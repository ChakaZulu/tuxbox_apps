/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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

/*
 $Id: rcinput.cpp,v 1.2 2001/11/15 22:42:45 obi Exp $

 Module for Remote Control Handling

History:
 $Log: rcinput.cpp,v $
 Revision 1.2  2001/11/15 22:42:45  obi
 add gpl

 Revision 1.13  2001/11/15 11:42:41  McClean
 gpl-headers added

 Revision 1.12  2001/10/29 16:49:00  field
 Kleinere Bug-Fixes (key-input usw.)

 Revision 1.11  2001/10/27 11:54:08  field
 Tastenwiederholblocker entruempelt

 Revision 1.10  2001/10/11 21:00:56  rasc
 clearbuffer() fuer RC-Input bei Start,
 Klassen etwas erweitert...

 Revision 1.9  2001/10/01 20:41:08  McClean
 plugin interface for games - beta but nice.. :)

 Revision 1.8  2001/09/23 21:34:07  rasc
 - LIFObuffer Module, pushbackKey fuer RCInput,
 - In einige Helper und widget-Module eingebracht
   ==> harmonischeres Menuehandling
 - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)


*/



#include "rcinput.h"
//#include "../global.h"


/**************************************************************************
*	Constructor - opens rc-input device and starts threads
*
**************************************************************************/
CRCInput::CRCInput()
{
	fd=open("/dev/dbox/rc0", O_RDONLY);
	if (fd<0)
	{
		perror("/dev/dbox/rc0");
		exit(-1);
	}
	ioctl(fd, RC_IOCTL_BCODES, 1);
	prevrccode = 0xffff;
	start();

    tv_prev.tv_sec = 0;
    repeat_block = 0;
}

/**************************************************************************
*	Destructor - close the input-device
*
**************************************************************************/
CRCInput::~CRCInput()
{
	if (fd>=0)
		close(fd);
}

/**************************************************************************
*	stopInput - stop reading rcin for plugins
*
**************************************************************************/
void CRCInput::stopInput()
{
	printf("rcstop requested....\n");
	pthread_cancel(thrInput);
    sem_close(&waitforkey);
}


/**************************************************************************
*	restartInput - restart reading rcin after calling plugins
*
**************************************************************************/
void CRCInput::restartInput()
{
	if (fd>=0)
		close(fd);

	fd=open("/dev/dbox/rc0", O_RDONLY);
	if (fd<0)
	{
		perror("/dev/dbox/rc0");
		exit(-1);
	}
	ioctl(fd, RC_IOCTL_BCODES, 1);
	start();
}

/**************************************************************************
*	get rc-key - timeout can be specified
*
**************************************************************************/
int CRCInput::getKey(int Timeout)
{
      // -- something pushed back ?
	if (LIFObuffer.available()) {
		return LIFObuffer.pop();
	}

      // a key already pressed?
	if (ringbuffer.available()) {
		return ringbuffer.read();
	}

    if(Timeout> 0)
    {
        struct timespec abs_wait;
        struct timeval now;

        sem_init (&waitforkey, 0, 0);
        gettimeofday(&now, NULL);
        TIMEVAL_TO_TIMESPEC(&now, &abs_wait);

        abs_wait.tv_nsec += (Timeout % 10)* 100000000;
        abs_wait.tv_sec += ((Timeout/ 10)+ (abs_wait.tv_nsec/ 1000000000));
        abs_wait.tv_nsec %= 1000000000;

    	sem_timedwait (&waitforkey, &abs_wait);

    	if (ringbuffer.available())
    	{
    		return ringbuffer.read();
    	}
    }
    return RC_timeout;

}



//
// -- push back a key into the buffer
// -- 2001-09-21  rasc
//

int CRCInput::pushbackKey (int key)
{
	return LIFObuffer.push(key);
} 


//
// -- clear
// -- get rid of all buffered keys (Hard/Software buffer)...
// -- 2001-10-11  rasc
//

void CRCInput::clear (void)
{
    int key;

	ringbuffer.clear();
	LIFObuffer.clear();

    do
    {
        key = getKey(1);
        //printf ("DBG: clear: Eat key: %d\n",key);
	} while (key != RC_timeout);

	ringbuffer.clear();
	LIFObuffer.clear();

}



/**************************************************************************
*	get rc-key from the rcdevice - internal use only!
*
**************************************************************************/
int CRCInput::getKeyInt()
{
    struct timeval tv;
    long long td;
	__u16 rccode;
	bool repeat = true;
	int	erg = RC_nokey;
	while (repeat)
	{
		if (read(fd, &rccode, 2)!=2)
		{
			printf("key: empty\n");
			//return -1; error!!!
		}
		else
		{
            gettimeofday( &tv, NULL );

            td = ( tv.tv_usec - tv_prev.tv_usec );
            td+= ( tv.tv_sec - tv_prev.tv_sec )* 1000000;

            if ( ( ( prevrccode&0x1F ) != ( rccode&0x1F ) ) ||
                 ( td > repeat_block ) )
            {
                tv_prev = tv;
//                printf("got key native key: %04x %d\n", rccode, tv.tv_sec );

    			if( prevrccode==rccode )
    			{
    				//key-repeat - cursors and volume are ok
				    int tkey = translate(rccode);
    				if ((tkey==RC_up) || (tkey==RC_down) || (tkey==RC_left) || (tkey==RC_right) ||
				        (tkey==RC_plus) || (tkey==RC_minus))
    				{//repeat is ok!
				    	erg = tkey;
    					repeat = false;
				    }
    			}
    			else
    			{
				    erg = translate(rccode);
    				prevrccode=rccode;
				    if(erg!=RC_nokey)
    				{
				    	repeat=false;
    					//printf("native key: %04x   tr: %04x   name: %s\n", rccode, erg, getKeyName(erg).c_str() );
				    }
    			}

            }
            else
            {
                //printf("key ignored %lld\n", td);
            }

		}

	}
	return erg;
}

/**************************************************************************
*       transforms the rc-key to string
*
**************************************************************************/
string CRCInput:: getKeyName(int code)
{
	switch(code)
	{
		case RC_standby: return "standby";
		case RC_home: return "home";
		case RC_setup: return "setup";
		case RC_0: return "0";
		case RC_1: return "1";
		case RC_2: return "2";
		case RC_3: return "3";
		case RC_4: return "4";
		case RC_5: return "5";
		case RC_6: return "6";
		case RC_7: return "7";
		case RC_8: return "8";
		case RC_9: return "9";
		case RC_red: return "red button";
		case RC_green: return "green button";
		case RC_yellow: return "yellow button";
		case RC_blue: return "blue button";
		case RC_page_up: return "page up";
		case RC_page_down: return "page down";
		case RC_up: return "cursor up";
		case RC_down: return "cursor down";
		case RC_left: return "cursor left";
		case RC_right: return "cursor right";
		case RC_ok: return "ok";
		case RC_plus: return "vol. inc";
		case RC_minus: return "vol. dec";
		case RC_spkr: return "mute";
		case RC_help: return "help";
		case RC_top_left: return "cursor top+left";
		case RC_top_right: return "cursor top+right";
		case RC_bottom_left: return "cursor bottom+left";
		case RC_bottom_right: return "cursor bottom+right";
		case RC_timeout: return "timeout";
		case RC_nokey: return "none";

		default: return "unknown";
	}
}


/**************************************************************************
*	transforms the rc-key to generic - internal use only!
*
**************************************************************************/
int CRCInput::translate(int code)
{
	if ((code&0xFF00)==0x5C00)
	{
		switch (code&0xFF)
		{
		case 0x0C: return RC_standby;
		case 0x20: return RC_home;
		case 0x27: return RC_setup;
		case 0x00: return RC_0;
		case 0x01: return RC_1;
		case 0x02: return RC_2;
		case 0x03: return RC_3;
		case 0x04: return RC_4;
		case 0x05: return RC_5;
		case 0x06: return RC_6;
		case 0x07: return RC_7;
		case 0x08: return RC_8;
		case 0x09: return RC_9;
		case 0x3B: return RC_blue;
		case 0x52: return RC_yellow;
		case 0x55: return RC_green;
		case 0x2D: return RC_red;
		case 0x54: return RC_page_up;
		case 0x53: return RC_page_down;
		case 0x0E: return RC_up;
 		case 0x0F: return RC_down;
		case 0x2F: return RC_left;
 		case 0x2E: return RC_right;
		case 0x30: return RC_ok;
 		case 0x16: return RC_plus;
 		case 0x17: return RC_minus;
 		case 0x28: return RC_spkr;
 		case 0x82: return RC_help;
		default:
			//perror("unknown old rc code");
			return RC_nokey;
		}
	} else if (!(code&0x00))
		return code&0x3F;
	//else
		//perror("unknown rc code");
	return RC_nokey;
}

/**************************************************************************
*	start the threads - internal use only!
*
**************************************************************************/
void CRCInput::start()
{
        if (pthread_create (&thrInput, NULL, InputThread, (void *) this) != 0 )
        {
                perror("create failed\n");
        }
        sem_init (&waitforkey, 0, 0);
        clear();
}


/**************************************************************************
*	Input Thread for key-input (blocking read) - internal use only!
*
**************************************************************************/
void * CRCInput::InputThread (void *arg)
{
        CRCInput* RCInput = (CRCInput*) arg;
        while(1)
        {
			int key = RCInput->getKeyInt();
			if(key!=-1)
			{
				RCInput->ringbuffer.add(key);
    			sem_post (&RCInput->waitforkey);
			}
        }
		printf("rcinput endend.....\n");
        return NULL;
}



