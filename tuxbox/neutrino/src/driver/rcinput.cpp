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

#include <driver/rcinput.h>
#include <driver/stream2file.h>

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

#include <global.h>
#include <neutrino.h>

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
#ifdef OLD_RC_API
			ioctl(fd_rc[i], RC_IOCTL_BCODES, 1);
#endif /* OLD_RC_API */
			fcntl(fd_rc[i], F_SETFL, O_NONBLOCK);
		}
	}

	//+++++++++++++++++++++++++++++++++++++++
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	fd_keyb = STDIN_FILENO;
#else
	fd_keyb = 0;
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
	/*
	::open("/dev/dbox/rc0", O_RDONLY);
	if (fd_keyb<0)
	{
		perror("/dev/stdin");
		exit(-1);
	}
	*/
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	::fcntl(fd_keyb, F_SETFL, O_NONBLOCK);

	struct termio new_termio;

	::ioctl(STDIN_FILENO, TCGETA, &orig_termio);

	saved_orig_termio      = true;

	new_termio             = orig_termio;
	new_termio.c_lflag    &= ~ICANON;
	//	new_termio.c_lflag    &= ~(ICANON|ECHO);
	new_termio.c_cc[VMIN ] = 1;
	new_termio.c_cc[VTIME] = 0;

	::ioctl(STDIN_FILENO, TCSETA, &new_termio);

#else
	//fcntl(fd_keyb, F_SETFL, O_NONBLOCK );

	//+++++++++++++++++++++++++++++++++++++++
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

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
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
	if (saved_orig_termio)
	{
		::ioctl(STDIN_FILENO, TCSETA, &orig_termio);
		printf("Original terminal settings restored.\n");
	}
#else
/*
	if(fd_keyb)
	{
		::close(fd_keyb);
	}
*/
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

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
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;

	bool doLoop = true;

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_MENU];

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	while (doLoop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

	if ( ( msg == CRCInput::RC_timeout ) ||
		( msg == CRCInput::RC_home ) ||
		( msg == CRCInput::RC_ok ) )
			doLoop = false;
		else
		{
			int mr = CNeutrinoApp::getInstance()->handleMsg( msg, data );

			if ( mr & messages_return::cancel_all )
			{
				res = menu_return::RETURN_EXIT_ALL;
				doLoop = false;
			}
			else if ( mr & messages_return::unhandled )
			{
				if ((msg <= CRCInput::RC_MaxRC) &&
				    (data == 0))                     /* <- button pressed */
				{
					if ( anyKeyCancels )
						doLoop = false;
					else
						timeoutEnd = CRCInput::calcTimeoutEnd( timeout );
				}
			}
		}


	}
	return res;
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

	return timeout_in_seconds > 0 ? (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec + (unsigned long long)timeout_in_seconds) * (unsigned long long) 1000000 : (unsigned long long) -1;
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

	if ( *msg == NeutrinoMessages::EVT_TIMESET )
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
       			*msg = NeutrinoMessages::EVT_TIMER;
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
		//printf("InitialTimeout= %lld:%lld\n", Timeout/1000000,Timeout%1000000);
        //printf("targetTimeout= %d:%d\n", tvselect.tv_sec,tvselect.tv_usec);

		FD_ZERO(&rfds);
		for (int i = 0; i < NUMBER_OF_EVENT_DEVICES; i++)
		{
			if (fd_rc[i] != -1)
				FD_SET(fd_rc[i], &rfds);
		}
#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
		if (true)
#else
		if (fd_keyb> 0)
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */
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
        			*msg = NeutrinoMessages::EVT_TIMER;
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


#ifdef KEYBOARD_INSTEAD_OF_REMOTE_CONTROL
		if (FD_ISSET(fd_keyb, &rfds))
		{
			int trkey;
			char key = 0;
			read(fd_keyb, &key, sizeof(key));

			switch(key)
			{
			case 27: // <- Esc
				trkey = KEY_HOME;
				break;
			case 10: // <- Return
			case 'o':
				trkey = KEY_OK;
				break;
			case 'p':
				trkey = KEY_POWER;
				break;
			case 's':
				trkey = KEY_SETUP;
				break;
			case 'h':
				trkey = KEY_HELP;
				break;
			case 'i':
				trkey = KEY_UP;
				break;
			case 'm':
				trkey = KEY_DOWN;
				break;
			case 'j':
				trkey = KEY_LEFT;
				break;
			case 'k':
				trkey = KEY_RIGHT;
				break;
			case 'r':
				trkey = KEY_RED;
				break;
			case 'g':
				trkey = KEY_GREEN;
				break;
			case 'y':
				trkey = KEY_YELLOW;
				break;
			case 'b':
				trkey = KEY_BLUE;
				break;
			case '0':
				trkey = RC_0;
				break;
			case '1':
				trkey = RC_1;
				break;
			case '2':
				trkey = RC_2;
				break;
			case '3':
				trkey = RC_3;
				break;
			case '4':
				trkey = RC_4;
				break;
			case '5':
				trkey = RC_5;
				break;
			case '6':
				trkey = RC_6;
				break;
			case '7':
				trkey = RC_7;
				break;
			case '8':
				trkey = RC_8;
				break;
			case '9':
				trkey = RC_9;
				break;
			case '+':
				trkey = RC_plus;
				break;
			case '-':
				trkey = RC_minus;
				break;
			case 'a':
				trkey = KEY_A;
				break;
			case 'u':
				trkey = KEY_U;
				break;
			case '/':
				trkey = KEY_SLASH;
				break;
			case '\\':
				trkey = KEY_BACKSLASH;
				break;
			default:
				trkey = RC_nokey;
			}
			if (trkey != RC_nokey)
			{
				*msg = trkey;
				*data = 0; /* <- button pressed */
				return;
			}
		}
#else
/*
                if(FD_ISSET(fd_keyb, &rfds))
                {
                        char key = 0;
                        read(fd_keyb, &key, sizeof(key));
                        printf("keyboard: %d\n", rc_key);
                }
*/
#endif /* KEYBOARD_INSTEAD_OF_REMOTE_CONTROL */

		if(FD_ISSET(fd_event, &rfds))
		{
			//printf("[neutrino] event - accept!\n");
			socklen_t          clilen;
			struct sockaddr_in cliaddr;
			clilen = sizeof(cliaddr);
			int fd_eventclient = accept(fd_event, (struct sockaddr *) &cliaddr, &clilen);

			*msg = RC_nokey;
			//printf("[neutrino] network event - read!\n");
			CEventServer::eventHead emsg;
			int read_bytes= recv(fd_eventclient, &emsg, sizeof(emsg), MSG_WAITALL);
			//printf("[neutrino] event read %d bytes - following %d bytes\n", read_bytes, emsg.dataSize );
			if ( read_bytes == sizeof(emsg) )
			{
				bool dont_delete_p = false;

				unsigned char* p;
				p= new unsigned char[ emsg.dataSize + 1 ];
				if ( p!=NULL )
			 	{
			 		read_bytes= recv(fd_eventclient, p, emsg.dataSize, MSG_WAITALL);
			 		//printf("[neutrino] eventbody read %d bytes - initiator %x\n", read_bytes, emsg.initiatorID );

					if ( emsg.initiatorID == CEventServer::INITID_CONTROLD )
					{
						switch(emsg.eventID)
						{
							case CControldClient::EVT_VOLUMECHANGED :
									*msg = NeutrinoMessages::EVT_VOLCHANGED;
									*data = 0;
								break;
							case CControldClient::EVT_MUTECHANGED :
									*msg = NeutrinoMessages::EVT_MUTECHANGED;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case CControldClient::EVT_VCRCHANGED :
									*msg = NeutrinoMessages::EVT_VCRCHANGED;
									*data = *(int*) p;
								break;
							case CControldClient::EVT_MODECHANGED :
									*msg = NeutrinoMessages::EVT_MODECHANGED;
									*data = *(int*) p;
								break;
							default:
								printf("[neutrino] event INITID_CONTROLD - unknown eventID 0x%x\n",  emsg.eventID );
						}
					}
					else if ( emsg.initiatorID == CEventServer::INITID_HTTPD )
					{
						switch(emsg.eventID)
						{
							case NeutrinoMessages::SHUTDOWN :
									*msg = NeutrinoMessages::SHUTDOWN;
									*data = 0;
								break;
							case NeutrinoMessages::EVT_POPUP :
									*msg = NeutrinoMessages::EVT_POPUP;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case NeutrinoMessages::EVT_EXTMSG :
									*msg = NeutrinoMessages::EVT_EXTMSG;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case NeutrinoMessages::CHANGEMODE :	// Change
									*msg = NeutrinoMessages::CHANGEMODE;
									*data = *(unsigned*) p;
								break;
							case NeutrinoMessages::STANDBY_TOGGLE :
									*msg = NeutrinoMessages::STANDBY_TOGGLE;
									*data = 0;
								break;
							case NeutrinoMessages::STANDBY_ON :
									*msg = NeutrinoMessages::STANDBY_ON;
									*data = 0;
								break;
							case NeutrinoMessages::STANDBY_OFF :
									*msg = NeutrinoMessages::STANDBY_OFF;
									*data = 0;
								break;
							case NeutrinoMessages::EVT_START_PLUGIN :
									*msg = NeutrinoMessages::EVT_START_PLUGIN;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case NeutrinoMessages::LOCK_RC :
									*msg = NeutrinoMessages::LOCK_RC;
									*data = 0;
								break;
							case NeutrinoMessages::UNLOCK_RC :
									*msg = NeutrinoMessages::UNLOCK_RC;
									*data = 0;
								break;
							default:
								printf("[neutrino] event INITID_HTTPD - unknown eventID 0x%x\n",  emsg.eventID );
						}
					}
					else if ( emsg.initiatorID == CEventServer::INITID_SECTIONSD )
			 		{
			 			//printf("[neutrino] event - from SECTIONSD %x %x\n", emsg.eventID, *(unsigned*) p);
						switch(emsg.eventID)
						{
							case CSectionsdClient::EVT_TIMESET:
								{
									struct timeval tv;
									gettimeofday( &tv, NULL );
									long long timeOld = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);

									stime((time_t*) p);

									gettimeofday( &tv, NULL );
									long long timeNew = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);

									delete p;
									p= new unsigned char[ sizeof(long long) ];
									*(long long*) p = timeNew - timeOld;

									if ((long long)last_keypress > *(long long*)p)
										last_keypress += *(long long *)p;

									// Timer anpassen
									for(std::vector<timer>::iterator e = timers.begin(); e != timers.end(); ++e)
										if (e->correct_time)
											e->times_out+= *(long long*) p;

									*msg          = NeutrinoMessages::EVT_TIMESET;
									*data         = (neutrino_msg_data_t) p;
									dont_delete_p = true;
								}
								break;
							case CSectionsdClient::EVT_GOT_CN_EPG:
									*msg          = NeutrinoMessages::EVT_CURRENTNEXT_EPG;
									*data         = (neutrino_msg_data_t) p;
									dont_delete_p = true;
								break;
							case CSectionsdClient::EVT_SERVICES_UPDATE:
									*msg          = NeutrinoMessages::EVT_SERVICES_UPD;
									*data         = 0;
								break;
							case CSectionsdClient::EVT_BOUQUETS_UPDATE:
								break;
							default:
								printf("[neutrino] event INITID_SECTIONSD - unknown eventID 0x%x\n",  emsg.eventID );
						}
			 		}
			 		else if ( emsg.initiatorID == CEventServer::INITID_ZAPIT )
			 		{
			 			//printf("[neutrino] event - from ZAPIT %x %x\n", emsg.eventID, *(unsigned*) p);
						switch(emsg.eventID)
						{
						case CZapitClient::EVT_RECORDMODE_ACTIVATED:
							*msg  = NeutrinoMessages::EVT_RECORDMODE;
							*data = true;
							break;
						case CZapitClient::EVT_RECORDMODE_DEACTIVATED:
							*msg  = NeutrinoMessages::EVT_RECORDMODE;
							*data = false;
							break;
						case CZapitClient::EVT_ZAP_COMPLETE:
							*msg = NeutrinoMessages::EVT_ZAP_COMPLETE;
							break;
						case CZapitClient::EVT_ZAP_FAILED:
							*msg = NeutrinoMessages::EVT_ZAP_FAILED;
							break;
						case CZapitClient::EVT_ZAP_SUB_FAILED:
							*msg = NeutrinoMessages::EVT_ZAP_SUB_FAILED;
							break;
						case CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD:
							*msg = NeutrinoMessages::EVT_ZAP_ISNVOD;
							break;
						case CZapitClient::EVT_ZAP_SUB_COMPLETE:
							*msg = NeutrinoMessages::EVT_ZAP_SUB_COMPLETE;
							break;
						case CZapitClient::EVT_SCAN_COMPLETE:
							*msg  = NeutrinoMessages::EVT_SCAN_COMPLETE;
							*data = 0;
							break;
						case CZapitClient::EVT_SCAN_NUM_TRANSPONDERS:
							*msg  = NeutrinoMessages::EVT_SCAN_NUM_TRANSPONDERS;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS:
							*msg  = NeutrinoMessages::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_REPORT_FREQUENCY:
							*msg = NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCY;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_FOUND_A_CHAN:
							*msg = NeutrinoMessages::EVT_SCAN_FOUND_A_CHAN;
							break;
						case CZapitClient::EVT_SCAN_SERVICENAME:
							*msg = NeutrinoMessages::EVT_SCAN_SERVICENAME;
							break;
						case CZapitClient::EVT_SCAN_FOUND_TV_CHAN:
							*msg  = NeutrinoMessages::EVT_SCAN_FOUND_TV_CHAN;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_FOUND_RADIO_CHAN:
							*msg  = NeutrinoMessages::EVT_SCAN_FOUND_RADIO_CHAN;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_FOUND_DATA_CHAN:
							*msg  = NeutrinoMessages::EVT_SCAN_FOUND_DATA_CHAN;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_REPORT_FREQUENCYP:
							*msg  = NeutrinoMessages::EVT_SCAN_REPORT_FREQUENCYP;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_NUM_CHANNELS:
							*msg = NeutrinoMessages::EVT_SCAN_NUM_CHANNELS;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_SCAN_PROVIDER:
							*msg = NeutrinoMessages::EVT_SCAN_PROVIDER;
							break;
						case CZapitClient::EVT_SCAN_SATELLITE:
							*msg = NeutrinoMessages::EVT_SCAN_SATELLITE;
							break;
						case CZapitClient::EVT_BOUQUETS_CHANGED:
							*msg  = NeutrinoMessages::EVT_BOUQUETSCHANGED;
							*data = 0;
							break;
#ifndef SKIP_CA_STATUS
						case CZapitClient::EVT_ZAP_CA_CLEAR:
							*msg  = NeutrinoMessages::EVT_ZAP_CA_CLEAR;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_ZAP_CA_LOCK:
							*msg  = NeutrinoMessages::EVT_ZAP_CA_LOCK;
							*data = *(unsigned*) p;
							break;
						case CZapitClient::EVT_ZAP_CA_FTA:
							*msg  = NeutrinoMessages::EVT_ZAP_CA_FTA;
							*data = *(unsigned*) p;
							break;
#endif
						case CZapitClient::EVT_SCAN_FAILED:
							*msg  = NeutrinoMessages::EVT_SCAN_FAILED;
							*data = 0;
							break;
						case CZapitClient::EVT_ZAP_MOTOR:
							*msg  = NeutrinoMessages::EVT_ZAP_MOTOR;
							*data = *(unsigned*) p;
							break;
						default:
							printf("[neutrino] event INITID_ZAPIT - unknown eventID 0x%x\n",  emsg.eventID );
						}
						if (((*msg) >= CRCInput::RC_WithData) && ((*msg) < CRCInput::RC_WithData + 0x10000000))
						{
							*data         = (neutrino_msg_data_t) p;
							dont_delete_p = true;
						}
			 		}
			 		else if ( emsg.initiatorID == CEventServer::INITID_TIMERD )
			 		{
/*
						if (emsg.eventID==CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM)
			 			{
						}

						if (emsg.eventID==CTimerdClient::EVT_NEXTPROGRAM)
			 			{
			 				*msg = NeutrinoMessages::EVT_NEXTPROGRAM;
			 				*data = (neutrino_msg_data_t) p;
			 				dont_delete_p = true;
			 			}
*/
						switch(emsg.eventID)
						{
							case CTimerdClient::EVT_ANNOUNCE_RECORD :
									*msg = NeutrinoMessages::ANNOUNCE_RECORD;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_ANNOUNCE_ZAPTO :
									*msg = NeutrinoMessages::ANNOUNCE_ZAPTO;
									*data = 0;
								break;
							case CTimerdClient::EVT_ANNOUNCE_SHUTDOWN :
									*msg = NeutrinoMessages::ANNOUNCE_SHUTDOWN;
									*data = 0;
								break;
							case CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER :
									*msg = NeutrinoMessages::ANNOUNCE_SLEEPTIMER;
									*data = 0;
								break;
							case CTimerdClient::EVT_SLEEPTIMER :
									*msg = NeutrinoMessages::SLEEPTIMER;
									*data = 0;
								break;
							case CTimerdClient::EVT_RECORD_START :
									*msg = NeutrinoMessages::RECORD_START;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_RECORD_STOP :
									*msg = NeutrinoMessages::RECORD_STOP;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_ZAPTO :
									*msg = NeutrinoMessages::ZAPTO;
									*data = (unsigned)  p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_SHUTDOWN :
									*msg = NeutrinoMessages::SHUTDOWN;
									*data = 0;
								break;
							case CTimerdClient::EVT_STANDBY_ON :
									*msg = NeutrinoMessages::STANDBY_ON;
									*data = 0;
								break;
							case CTimerdClient::EVT_STANDBY_OFF :
									*msg = NeutrinoMessages::STANDBY_OFF;
									*data = 0;
								break;
							case CTimerdClient::EVT_REMIND :
									*msg = NeutrinoMessages::REMIND;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;
							case CTimerdClient::EVT_EXEC_PLUGIN :
									*msg = NeutrinoMessages::EVT_START_PLUGIN;
									*data = (unsigned) p;
									dont_delete_p = true;
								break;

							default :
								printf("[neutrino] event INITID_TIMERD - unknown eventID 0x%x\n",  emsg.eventID );

						}
					}
					else if (emsg.initiatorID == CEventServer::INITID_NEUTRINO)
					{
						if ((emsg.eventID == NeutrinoMessages::EVT_RECORDING_ENDED) &&
						    (read_bytes == sizeof(stream2file_status2_t)))
						{
							*msg  = NeutrinoMessages::EVT_RECORDING_ENDED;
							*data = (neutrino_msg_data_t) p;
							dont_delete_p = true;
						}
					}
					else if (emsg.initiatorID == CEventServer::INITID_GENERIC_INPUT_EVENT_PROVIDER)
					{
						if (read_bytes == sizeof(int))
						{
							*msg  = *(int *)p;
							*data = emsg.eventID;
						}
					}
					else
						printf("[neutrino] event - unknown initiatorID 0x%x\n",  emsg.initiatorID);
			 		if ( !dont_delete_p )
			 		{
			 			delete p;
			 			p= NULL;
			 		}
			 	}
			}
			else
			{
				printf("[neutrino] event - read failed!\n");
			}

			::close(fd_eventclient);

			if ( *msg != RC_nokey )
			{
				// raus hier :)
				//printf("[neutrino] event 0x%x\n", *msg);
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
#ifdef OLD_RC_API
						if (ev.code != 0x5cfe)
#else /* OLD_RC_API */
						if ((ev.value) && (ev.type==EV_KEY))
#endif /* OLD_RC_API */
						{
							unsigned long long now_pressed;
							bool keyok = true;

#ifdef OLD_RC_API
							gettimeofday( &tv, NULL );
#else /* OLD_RC_API */
							tv = ev.time;
#endif /* OLD_RC_API */
							now_pressed = (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
#ifdef OLD_RC_API
							//alter nokia-rc-code - lastkey löschen weil sonst z.b. nicht zweimal nacheinander ok gedrückt werden kann
							if ((ev.code & 0xff00) == 0x5c00)
								rc_last_key = 0;
#endif /* OLD_RC_API */

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

#ifdef OLD_RC_API
									*msg  = (trkey == RC_standby_release) ? RC_standby : trkey;
									*data = (trkey == RC_standby_release) ? 1 : 0; /* <- button released / pressed */
#else /* OLD_RC_API */
									*msg = trkey;
									*data = 0; /* <- button pressed */
#endif /* OLD_RC_API */
									return;
								}
							}
						}
#ifndef OLD_RC_API
						else
						{
							// clear rc_last_key on keyup event
							//printf("got keyup native key: %04x %04x, translate: %04x -%s-\n", ev.code, ev.code&0x1f, translate(ev.code), getKeyName(translate(ev.code)).c_str() );
							rc_last_key = 0;
							if (trkey == RC_standby)
							{
								*msg = RC_standby;
								*data = 1; /* <- button released */
								return;
							}
						}
#endif /* OLD_RC_API */
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

			// Keyboard keys
			case RC_esc:
			  return "escape";
			case RC_hyphen:
			  return "hyphen";
			case RC_equal:
			  return "equal";
			case RC_backspace:
			  return "backspace";
			case RC_tab:
			  return "tab";
			case RC_leftbrace:
			  return "leftbrace";
			case RC_rightbrace:
			  return "rightbrace";
			case RC_enter:
			  return "enter";
			case RC_leftctrl:
			  return "leftctrl";
			case RC_semicolon:
			  return "semicolon";
			case RC_apostrophe:
			  return "apostrophe";
			case RC_grave:
			  return "grave";
			case RC_leftshift:
			  return "leftshift";
			case RC_backslash:
			  return "backslash";
			case RC_comma:
			  return "comma";
			case RC_dot:
			  return "dot";
			case RC_slash:
			  return "slash";
			case RC_rightshift:
			  return "rightshift";
			case RC_kpasterisk:
			    return "kpasterisk";
			case RC_leftalt:
			  return "leftalt";
			case RC_space:
			  return "space";
			case RC_capslock:
			  return "capslock";
			case RC_f1:
			  return "f1";
			case RC_f2:
			  return "f2";
			case RC_f3:
			  return "f3";
			case RC_f4:
			  return "f4";
			case RC_f5:
			  return "f5";
			case RC_f6:
			  return "f6";
			case RC_f7:
			  return "f7";
			case RC_f8:
			  return "f8";
			case RC_f9:
			  return "f9";
			case RC_f10:
			  return "f10";
			case RC_numlock:
			  return "numlock";
			case RC_scrolllock:
			  return "scrolllock";
			case RC_kp7:
			  return "kp7";
			case RC_kp8:
			  return "kp8";
			case RC_kp9:
			  return "kp9";
			case RC_kpminus:
			  return "kpminus";
			case RC_kp4:
			  return "kp4";
			case RC_kp5:
			  return "kp5";
			case RC_kp6:
			  return "kp6";
			case RC_kpplus:
			  return "kpplus";
			case RC_kp1:
			  return "kp1";
			case RC_kp2:
			  return "kp2";
			case RC_kp3:
			  return "kp3";
			case RC_kp0:
			  return "kp0";
			case RC_kpdot:
			  return "kpdot";
			case RC_102nd:
			  return "102nd";
			case RC_kpenter:
			  return "kpenter";
			case RC_kpslash:
			  return "kpslash";
			case RC_rightalt:
			  return "rightalt";
			case RC_end:
			  return "end";
			case RC_insert:
			  return "insert";
			case RC_delete:
			  return "delete";
			case RC_pause:
			  return "pause";
			case RC_btn_left:
			  return "btn_left";
			case RC_btn_right:
			  return "btn_right";

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
#ifdef OLD_RC_API
	if ((code&0xFF00)==0x5C00)
	{
		switch (code&0xFF)
		{
		case 0x0C:
			return RC_standby;
		case 0x20:
			return RC_home;
		case 0x27:
			return RC_setup;
		case 0x00:
			return RC_0;
		case 0x01:
			return RC_1;
		case 0x02:
			return RC_2;
		case 0x03:
			return RC_3;
		case 0x04:
			return RC_4;
		case 0x05:
			return RC_5;
		case 0x06:
			return RC_6;
		case 0x07:
			return RC_7;
		case 0x08:
			return RC_8;
		case 0x09:
			return RC_9;
		case 0x3B:
			return RC_blue;
		case 0x52:
			return RC_yellow;
		case 0x55:
			return RC_green;
		case 0x2D:
			return RC_red;
		case 0x54:
			return RC_page_up;
		case 0x53:
			return RC_page_down;
		case 0x0E:
			return RC_up;
		case 0x0F:
			return RC_down;
		case 0x2F:
			return RC_left;
		case 0x2E:
			return RC_right;
		case 0x30:
			return RC_ok;
		case 0x16:
			return RC_plus;
		case 0x17:
			return RC_minus;
		case 0x28:
			return RC_spkr;
		case 0x82:
			return RC_help;
		default:
			return RC_nokey;
		}
	}
	else if ((code & 0xFF00) == 0xFF00)
	{
		switch (code & 0xFF)
		{
		case 0x12:
		case 0x9d:
			return RC_standby;
		case 0x48:
		case 0xab:
			return RC_down;
		case 0x24:
		case 0xc7:
			return RC_up;
		case 0x20:
		case 0x40:
		case 0xaf:
		case 0xcf:
			return RC_nokey;
		case 0x10:
		case 0x9f:
			return RC_standby_release;
		}
	}
	else if (!(code&0x00))
	{
		static const uint translation[0x21 + 1] = 
			{ RC_0           , RC_1   , RC_2      , RC_3        , RC_4    , RC_5    , RC_6      , RC_7       , RC_8        , RC_9          ,
			  RC_right       , RC_left, RC_up     , RC_down     , RC_ok   , RC_spkr , RC_standby, RC_green   , RC_yellow   , RC_red        ,
			  RC_blue        , RC_plus, RC_minus  , RC_help     , RC_setup, RC_nokey, RC_nokey  , RC_top_left, RC_top_right, RC_bottom_left,
			  RC_bottom_right, RC_home, RC_page_up, RC_page_down};
		if ((code & 0x3F) <= 0x21)
			return translation[code & 0x3F];
		else
			return RC_nokey;
	}
	
	return RC_nokey;
#else /* OLD_RC_API */
	if ((code >= 0) && (code <= KEY_MAX))
		return code;
	else
		return RC_nokey;
#endif /* OLD_RC_API */
}
