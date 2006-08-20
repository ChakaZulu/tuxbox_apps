/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
                      2003 thegoodguy

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rcinput.h>
#include <stream2file.h>

#include <dbox/fp.h>
#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
#include <termio.h>
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
#include <unistd.h>
#include <fcntl.h>

#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <eventserver.h>

#ifdef OLD_RC_API
const char * const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {"/dev/dbox/rc0"};
#define RC_standby_release (KEY_MAX + 1)
typedef struct { __u16 code; } t_input_event;
#else /* OLD_RC_API */
const char * const RC_EVENT_DEVICE[NUMBER_OF_EVENT_DEVICES] = {"/dev/input/event0", "/dev/input/event1"};
typedef struct input_event t_input_event;
#endif /* OLD_RC_API */


#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
static struct termio orig_termio;
static bool          saved_orig_termio = false;
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */


/**************************************************************************
*	Constructor - opens rc-input device and starts threads
*
**************************************************************************/
CRCInput::CRCInput()
{
	timerid= 1;

	// pipe for internal event-queue
	// -----------------------------
	if (pipe(fd_pipe_high_priority) < 0)
	{
		perror("fd_pipe_high_priority");
		exit(-1);
	}

	fcntl(fd_pipe_high_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_high_priority[1], F_SETFL, O_NONBLOCK );

	if (pipe(fd_pipe_low_priority) < 0)
	{
		perror("fd_pipe_low_priority");
		exit(-1);
	}

	fcntl(fd_pipe_low_priority[0], F_SETFL, O_NONBLOCK );
	fcntl(fd_pipe_low_priority[1], F_SETFL, O_NONBLOCK );


	// open event-library
	// -----------------------------
	fd_event = 0;

	//network-setup
    struct sockaddr_un servaddr;
    int    clilen;
    memset(&servaddr, 0, sizeof(struct sockaddr_un));
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, NEUTRINO_UDS_NAME);
    clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
    unlink(NEUTRINO_UDS_NAME);

    //network-setup
    if ((fd_event = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
    {
        perror("[neutrino] socket\n");
    }

	if ( bind(fd_event, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		perror("[neutrino] bind failed...\n");
		exit(-1);
	}


	if (listen(fd_event, 5) !=0)
	{
		perror("[neutrino] listen failed...\n");
		exit( -1 );
	}

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		fd_rc[i] = -1;
	}

	open();
}

void CRCInput::open()
{
	close();

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if ((fd_rc[i] = ::open(RC_EVENT_DEVICE[i], O_RDONLY)) == -1)
			perror(RC_EVENT_DEVICE[i]);
		else
		{
			fcntl(fd_rc[i], F_SETFL, O_NONBLOCK);
		}
	}

	//+++++++++++++++++++++++++++++++++++++++
	fd_keyb = 0;

	calculateMaxFd();
}

void CRCInput::close()
{
	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			::close(fd_rc[i]);
			fd_rc[i] = -1;
		}
	}

	calculateMaxFd();
}

void CRCInput::calculateMaxFd()
{
	fd_max = fd_event;

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		if (fd_rc[i] > fd_max)
			fd_max = fd_rc[i];
	
	if(fd_pipe_high_priority[0] > fd_max)
		fd_max = fd_pipe_high_priority[0];
	if(fd_pipe_low_priority[0] > fd_max)
		fd_max = fd_pipe_low_priority[0];
}


/**************************************************************************
*	Destructor - close the input-device
*
**************************************************************************/
CRCInput::~CRCInput()
{
	close();

	if(fd_pipe_high_priority[0])
		::close(fd_pipe_high_priority[0]);
	if(fd_pipe_high_priority[1])
		::close(fd_pipe_high_priority[1]);

	if(fd_pipe_low_priority[0])
		::close(fd_pipe_low_priority[0]);
	if(fd_pipe_low_priority[1])
		::close(fd_pipe_low_priority[1]);

	if(fd_event)
		::close(fd_event);
}

/**************************************************************************
*	stopInput - stop reading rcin for plugins
*
**************************************************************************/
void CRCInput::stopInput()
{
	close();
}


/**************************************************************************
*	restartInput - restart reading rcin after calling plugins
*
**************************************************************************/
void CRCInput::restartInput()
{
	open();
}

int CRCInput::messageLoop( bool anyKeyCancels, int timeout )
{
	return 0;
}


int CRCInput::addTimer(unsigned long long Interval, bool oneshot, bool correct_time )
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	timer _newtimer;
	if (!oneshot)
		_newtimer.interval = Interval;
	else
		_newtimer.interval = 0;

	_newtimer.id = timerid++;
	if ( correct_time )
		_newtimer.times_out = timeNow+ Interval;
	else
		_newtimer.times_out = Interval;

	_newtimer.correct_time = correct_time;

	//printf("adding timer %d (0x%llx, 0x%llx)\n", _newtimer.id, _newtimer.times_out, Interval);

	std::vector<timer>::iterator e;
	for ( e= timers.begin(); e!= timers.end(); ++e )
		if ( e->times_out> _newtimer.times_out )
			break;

	timers.insert(e, _newtimer);
	return _newtimer.id;
}

int CRCInput::addTimer(struct timeval Timeout)
{
	unsigned long long timesout = (unsigned long long) Timeout.tv_usec + (unsigned long long)((unsigned long long) Timeout.tv_sec * (unsigned long long) 1000000);
	return addTimer( timesout, true, false );
}

int CRCInput::addTimer(const time_t *Timeout)
{
	return addTimer( (unsigned long long)*Timeout* (unsigned long long) 1000000, true, false );
}

void CRCInput::killTimer(uint id)
{
	//printf("killing timer %d\n", id);
	std::vector<timer>::iterator e;
	for ( e= timers.begin(); e!= timers.end(); ++e )
		if ( e->id == id )
		{
			timers.erase(e);
			break;
		}
}

int CRCInput::checkTimers()
{
	struct timeval tv;
	int _id = 0;

	gettimeofday( &tv, NULL );
	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);


	std::vector<timer>::iterator e;
	for ( e= timers.begin(); e!= timers.end(); ++e )
		if ( e->times_out< timeNow+ 2000 )
		{
//			printf("timeout timer %d %llx %llx\n",e->id,e->times_out,timeNow );
			_id = e->id;
			if ( e->interval != 0 )
			{
				timer _newtimer;
				_newtimer.id= e->id;
				_newtimer.interval= e->interval;
				_newtimer.times_out= e->times_out+ e->interval;
				_newtimer.correct_time= e->correct_time;

            	timers.erase(e);
				for ( e= timers.begin(); e!= timers.end(); ++e )
					if ( e->times_out> _newtimer.times_out )
						break;

				timers.insert(e, _newtimer);
			}
			else
				timers.erase(e);

			break;
        }
//        else
//    		printf("skipped timer %d %llx %llx\n",e->id,e->times_out, timeNow );
	return _id;
}



long long CRCInput::calcTimeoutEnd(const int timeout_in_seconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	return (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec + (unsigned long long)timeout_in_seconds) * (unsigned long long) 1000000;
}

long long CRCInput::calcTimeoutEnd_MS(const int timeout_in_milliseconds)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	return ( timeNow + timeout_in_milliseconds * 1000 );
}


void CRCInput::getMsgAbsoluteTimeout(neutrino_msg_t * msg, neutrino_msg_data_t * data, unsigned long long *TimeoutEnd, bool bAllowRepeatLR)
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	unsigned long long timeNow = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	unsigned long long diff;

	if ( *TimeoutEnd < timeNow+ 100 )
		diff = 100;  // Minimum Differenz...
	else
		diff = ( *TimeoutEnd - timeNow );

	getMsg_us( msg, data, diff, bAllowRepeatLR );

	if ( 0 /* radiobox *msg == NeutrinoMessages::EVT_TIMESET */ )
	{
		// recalculate timeout....
		//unsigned long long ta= *TimeoutEnd;
		*TimeoutEnd= *TimeoutEnd + *(long long*) *data;

		//printf("[getMsgAbsoluteTimeout]: EVT_TIMESET - recalculate timeout\n%llx/%llx - %llx/%llx\n", timeNow, *(long long*) *data, *TimeoutEnd, ta );
	}
}

void CRCInput::getMsg(neutrino_msg_t * msg, neutrino_msg_data_t * data, int Timeout, bool bAllowRepeatLR)
{
	getMsg_us(msg, data, (unsigned long long) Timeout * 100 * 1000, bAllowRepeatLR);
}

void CRCInput::getMsg_ms(neutrino_msg_t * msg, neutrino_msg_data_t * data, int Timeout, bool bAllowRepeatLR)
{
	getMsg_us(msg, data, (unsigned long long) Timeout * 1000, bAllowRepeatLR);
}

void CRCInput::getMsg_us(neutrino_msg_t * msg, neutrino_msg_data_t * data, unsigned long long Timeout, bool bAllowRepeatLR)
{
	static unsigned long long last_keypress = 0ULL;
	unsigned long long getKeyBegin;

	static __u16 rc_last_key = 0;
	static __u16 rc_last_repeat_key = 0;

	struct timeval tv, tvselect;
	unsigned long long InitialTimeout = Timeout;
	long long targetTimeout;

	int timer_id;
	fd_set rfds;
	t_input_event ev;

	//set 0
	*data = 0;

	// wiederholung reinmachen - dass wirklich die ganze zeit bis timeout gewartet wird!
	gettimeofday( &tv, NULL );
	getKeyBegin = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

	while(1)
	{
		timer_id = 0;
		if ( timers.size()> 0 )
		{
			gettimeofday( &tv, NULL );
			unsigned long long t_n= (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
			if ( timers[0].times_out< t_n )
			{
				timer_id = checkTimers();
	       			*msg = 0; // radiobox, replaced with 0 NeutrinoMessages::EVT_TIMER;
				*data = timer_id;
				return;
			}
			else
			{
             	targetTimeout = timers[0].times_out - t_n;
				if ( (unsigned long long) targetTimeout> Timeout)
					targetTimeout= Timeout;
				else
					timer_id = timers[0].id;
			}
		}
		else
			targetTimeout= Timeout;

	    tvselect.tv_sec = targetTimeout/1000000;
		tvselect.tv_usec = targetTimeout%1000000;

		FD_ZERO(&rfds);
		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		{
			if (fd_rc[i] != -1)
				FD_SET(fd_rc[i], &rfds);
		}
		if (fd_keyb> 0)
			FD_SET(fd_keyb, &rfds);

		FD_SET(fd_event, &rfds);
		FD_SET(fd_pipe_high_priority[0], &rfds);
		FD_SET(fd_pipe_low_priority[0], &rfds);

		int status =  select(fd_max+1, &rfds, NULL, NULL, &tvselect);

		if ( status == -1 )
		{
			perror("[neutrino - getMsg_us]: select returned ");
			// in case of an error return timeout...?!
			*msg = RC_timeout;
			*data = 0;
			return;
		}
		else if ( status == 0 ) // Timeout!
		{
			if ( timer_id != 0 )
			{
			    timer_id = checkTimers();
				if ( timer_id != 0 )
				{
        			*msg = 0; // radiobox, replaced with 0 NeutrinoMessages::EVT_TIMER;
					*data = timer_id;
					return;
				}
				else
					continue;
			}
			else
			{
				*msg = RC_timeout;
				*data = 0;
				return;
			}
		}

		if(FD_ISSET(fd_pipe_high_priority[0], &rfds))
		{
			struct event buf;

			read(fd_pipe_high_priority[0], &buf, sizeof(buf));

			*msg  = buf.msg;
			*data = buf.data;

			// printf("got event from high-pri pipe %x %x\n", *msg, *data );

			return;
		}

		if(FD_ISSET(fd_event, &rfds))
		{
			//printf("[neutrino] event - accept!\n");
			socklen_t          clilen;
			struct sockaddr_in cliaddr;
			clilen = sizeof(cliaddr);
			int fd_eventclient = accept(fd_event, (struct sockaddr *) &cliaddr, &clilen);

			*msg = RC_nokey;

			CEventServer::eventHead emsg;
			int read_bytes= recv(fd_eventclient, &emsg, sizeof(emsg), MSG_WAITALL);

			if ( read_bytes == sizeof(emsg) )
			{
				bool dont_delete_p = false;

				unsigned char* p;
				p= new unsigned char[ emsg.dataSize + 1 ];
				if ( p!=NULL )
			 	{
			 		read_bytes= recv(fd_eventclient, p, emsg.dataSize, MSG_WAITALL);

					if ( emsg.initiatorID == CEventServer::INITID_CONTROLD )
					{

					}
					else if ( emsg.initiatorID == CEventServer::INITID_HTTPD )
					{

					}
					else if ( emsg.initiatorID == CEventServer::INITID_SECTIONSD )
			 		{

			 		}
			 		else if ( emsg.initiatorID == CEventServer::INITID_ZAPIT )
			 		{

			 		}
			 		else if ( emsg.initiatorID == CEventServer::INITID_TIMERD )
			 		{

					}
					else if (emsg.initiatorID == CEventServer::INITID_NEUTRINO)
					{

					}
					else if (emsg.initiatorID ==
						CEventServer::INITID_GENERIC_INPUT_EVENT_PROVIDER)
					{
						if (read_bytes == sizeof(int))
						{
							*msg  = *(int *)p;
							*data = emsg.eventID;
						}
					}
					else
						printf("[radiobox] event - unknown initiatorID 0x%x\n",  emsg.initiatorID);
			 		if ( !dont_delete_p )
			 		{
			 			delete p;
			 			p= NULL;
			 		}
			 	}
			}
			else
			{
				printf("[radiobox] event - read failed!\n");
			}

			::close(fd_eventclient);

			if ( *msg != RC_nokey )
			{
				// raus hier :)
				return;
			}
		}

		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		{
			if ((fd_rc[i] != -1) &&
			    (FD_ISSET(fd_rc[i], &rfds)))
			{
				if (read(fd_rc[i], &ev, sizeof(t_input_event)) == sizeof(t_input_event))
				{
					uint trkey = translate(ev.code);

					if (trkey != RC_nokey)
					{
						if (ev.value)
						{
							unsigned long long now_pressed;
							bool keyok = true;

							tv = ev.time;

							now_pressed = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);

							if (ev.code == rc_last_key)
							{
								/* only allow selected keys to be repeated */
								/* (why?)                                  */
								if  ((trkey == RC_up     ) ||
								     (trkey == RC_down   ) ||
								     (trkey == RC_plus   ) ||
								     (trkey == RC_minus  ) ||
								     (trkey == RC_standby) ||
								     ((bAllowRepeatLR) && ((trkey == RC_left ) ||
											   (trkey == RC_right))))
								{
									if (rc_last_repeat_key != ev.code)
									{
										if ((now_pressed > last_keypress + repeat_block) ||
										    /* accept all keys after time discontinuity: */
										    (now_pressed < last_keypress)) 
											rc_last_repeat_key = ev.code;
										else
											keyok = false;
									}
								}
								else
									keyok = false;
							}
							else
								rc_last_repeat_key = 0;

							rc_last_key = ev.code;

							if (keyok)
							{
								if ((now_pressed > last_keypress + repeat_block_generic) ||
								    /* accept all keys after time discontinuity: */
								    (now_pressed < last_keypress)) 
								{
									last_keypress = now_pressed;

									*msg = trkey;
									*data = 0; /* <- button pressed */

									return;
								}
							}
						}
						else
						{
							*msg = trkey;
							*data = 1; /* <- button released */
							return;
						}
					}
				}
			}
		} 

		if(FD_ISSET(fd_pipe_low_priority[0], &rfds))
		{
			struct event buf;

			read(fd_pipe_low_priority[0], &buf, sizeof(buf));

			*msg  = buf.msg;
			*data = buf.data;

			// printf("got event from low-pri pipe %x %x\n", *msg, *data );

			return;
		}

        if ( InitialTimeout == 0 )
		{
			//nicht warten wenn kein key da ist
			*msg = RC_timeout;
			*data = 0;
			return;
		}
		else
		{
			//timeout neu kalkulieren
			gettimeofday( &tv, NULL );
			long long getKeyNow = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);
			long long diff = (getKeyNow - getKeyBegin);
			if( Timeout <= (unsigned long long) diff )
			{
				*msg = RC_timeout;
				*data = 0;
				return;
			}
			else
				Timeout -= diff;
		}
	}
}

void CRCInput::postMsg(const neutrino_msg_t msg, const neutrino_msg_data_t data, const bool Priority)
{
//	printf("postMsg %x %x %d\n", msg, data, Priority );

	struct event buf;
	buf.msg  = msg;
	buf.data = data;

	if (Priority)
		write(fd_pipe_high_priority[1], &buf, sizeof(buf));
	else
		write(fd_pipe_low_priority[1], &buf, sizeof(buf));
}


void CRCInput::clearRCMsg()
{
	t_input_event ev;

	for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
	{
		if (fd_rc[i] != -1)
		{
			while (read(fd_rc[i], &ev, sizeof(t_input_event)) == sizeof(t_input_event))
				;
		}
	}
}

/**************************************************************************
*       isNumeric - test if key is 0..9
*
**************************************************************************/
bool CRCInput::isNumeric(const neutrino_msg_t key)
{
	return ((key == RC_0) || ((key >= RC_1) && (key <= RC_9)));
}

/**************************************************************************
*       getNumericValue - return numeric value of the key or -1
*
**************************************************************************/
int CRCInput::getNumericValue(const neutrino_msg_t key)
{
	return ((key == RC_0) ? (int)0 : (((key >= RC_1) && (key <= RC_9)) ? (int)(key - RC_1 + 1) : (int)-1));
}

/**************************************************************************
*       convertDigitToKey - return key representing digit or RC_nokey
*
**************************************************************************/
static const unsigned int digit_to_key[10] = {CRCInput::RC_0, CRCInput::RC_1, CRCInput::RC_2, CRCInput::RC_3, CRCInput::RC_4, CRCInput::RC_5, CRCInput::RC_6, CRCInput::RC_7, CRCInput::RC_8, CRCInput::RC_9};

unsigned int CRCInput::convertDigitToKey(const unsigned int digit)
{
	return (digit < 10) ? digit_to_key[digit] : RC_nokey;
}

/**************************************************************************
*       getUnicodeValue - return unicode value of the key or -1
*
**************************************************************************/
#define UNICODE_VALUE_SIZE 58
static const int unicode_value[UNICODE_VALUE_SIZE] = {-1 , -1 , '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', -1 , -1 ,
						      'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', -1 , -1 , 'A', 'S',
						      'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', -1 /* FIXME */, -1 /* FIXME */, -1 , '\\', 'Z', 'X', 'C', 'V',
						      'B', 'N', 'M', ',', '.', '/', -1, -1, -1, ' '};

int CRCInput::getUnicodeValue(const neutrino_msg_t key)
{
	if (key < UNICODE_VALUE_SIZE)
		return unicode_value[key];
	else
		return -1;
}

/**************************************************************************
*       transforms the rc-key to const char *
*
**************************************************************************/
const char * CRCInput::getSpecialKeyName(const unsigned int key)
{
	switch(key)
	{
			case RC_standby:
			return "standby";
			case RC_home:
			return "home";
			case RC_setup:
			return "setup";
			case RC_red:
			return "red button";
			case RC_green:
			return "green button";
			case RC_yellow:
			return "yellow button";
			case RC_blue:
			return "blue button";
			case RC_page_up:
			return "page up";
			case RC_page_down:
			return "page down";
			case RC_up:
			return "cursor up";
			case RC_down:
			return "cursor down";
			case RC_left:
			return "cursor left";
			case RC_right:
			return "cursor right";
			case RC_ok:
			return "ok";
			case RC_plus:
			return "vol. inc";
			case RC_minus:
			return "vol. dec";
			case RC_spkr:
			return "mute";
			case RC_help:
			return "help";
			case RC_timeout:
			return "timeout";
			case RC_nokey:
			return "none";

			default:
			return "unknown";
	}
}

std::string CRCInput::getKeyName(const unsigned int key)
{
	int unicode_value = getUnicodeValue(key);
	if (unicode_value == -1)
		return getSpecialKeyName(key);
	else
	{
		char tmp[2];
		tmp[0] = unicode_value;
		tmp[1] = 0;
		return std::string(tmp);
	}
}

/**************************************************************************
*	transforms the rc-key to generic - internal use only!
*
**************************************************************************/
int CRCInput::translate(int code)
{
	if ((code >= 0) && (code <= KEY_MAX))
		return code;
	else
		return RC_nokey;
}

////////////////////////////////////////////////

