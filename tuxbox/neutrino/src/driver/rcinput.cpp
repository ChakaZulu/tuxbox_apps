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
 $Id: rcinput.cpp,v 1.22 2002/01/09 00:05:08 McClean Exp $
 
 Module for Remote Control Handling
 
History:
 $Log: rcinput.cpp,v $
 Revision 1.22  2002/01/09 00:05:08  McClean
 secure-...

 Revision 1.21  2002/01/08 23:22:08  McClean
 fix for old nokia-fb's

 Revision 1.20  2002/01/08 12:34:28  McClean
 better rc-handling - add flat-standby

 Revision 1.19  2002/01/08 03:08:20  McClean
 improve input-handling

 Revision 1.18  2002/01/06 03:04:04  McClean
 busybox 0.60 workarround

 Revision 1.17  2002/01/03 20:03:20  McClean
 cleanup

 Revision 1.16  2001/12/25 11:40:30  McClean
 better pushback handling
 
 Revision 1.15  2001/12/25 03:28:42  McClean
 better pushback-handling
 
 Revision 1.14  2001/11/26 02:34:04  McClean
 include (.../../stuff) changed - correct unix-formated files now
 
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
#include "../global.h"


void printbin( int a)
{
	for(int pos = 15;pos>=0;pos--)
	{
		printf("%d", a&(1<<pos)?1:0);
	}
	printf("\n");
}



/**************************************************************************
*	Constructor - opens rc-input device and starts threads
*
**************************************************************************/
CRCInput::CRCInput()
{
	open();
}

void CRCInput::open()
{
	close();
	fd=::open("/dev/dbox/rc0", O_RDONLY);
	if (fd<0)
	{
		perror("/dev/dbox/rc0");
		exit(-1);
	}
	ioctl(fd, RC_IOCTL_BCODES, 1);
	fcntl(fd, F_SETFL, O_NONBLOCK );
}

void CRCInput::close()
{
	if(fd)
	{
		::close(fd);
	}
}

/**************************************************************************
*	Destructor - close the input-device
*
**************************************************************************/
CRCInput::~CRCInput()
{
	close();
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

/**************************************************************************
*	get rc-key - timeout can be specified
*
**************************************************************************/
int CRCInput::getKey(int Timeout)
{
	static long long last_keypress=0;
	static __u16 rc_last_key = 0;
	static __u16 rc_last_repeat_key = 0;
	__u16 rc_key;
	bool exit=false;

	//es ist ein key im pushback-Buffer - diesen zurückgeben
	if(pb_keys.available())
	{
		//printf("resonse pb-key\n");
		return pb_keys.read();
	}

	while(!exit)
	{
		int status = read(fd, &rc_key, sizeof(rc_key));
		if (status==2)
		{	//loslassen bei alten nokia fb's
			if(rc_key!=0x5cfe)
			{
				//printf("got key native key: %04x %04x\n", rc_key, rc_key&0x1f );
				struct timeval tv;
				long long now_pressed;
				bool keyok = true;

				gettimeofday( &tv, NULL );
				now_pressed = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);
				//printf("diff: %lld - %lld = %lld should: %d\n", now_pressed, last_keypress, now_pressed-last_keypress, repeat_block);
				
				//alter nokia-rc-code - lastkey löschen weil sonst z.b. nicht zweimal nacheinander ok gedrückt werden kann
				if((rc_key&0xff00)==0x5c00)
				{
					rc_last_key = 0;
				}
				//test auf wiederholenden key (gedrückt gehalten)
				if (rc_key == rc_last_key)
				{
					keyok = false;
					//nur diese tasten sind wiederholbar 
					int trkey = translate(rc_key);
					if ((trkey==RC_up) || (trkey==RC_down) || (trkey==RC_plus) || (trkey==RC_minus) || (trkey==RC_standby))
					{
						if( rc_last_repeat_key!=rc_key)
						{
							if(abs(now_pressed-last_keypress)>repeat_block)
							{
								keyok = true;
								rc_last_repeat_key = rc_key;
							}
						}
						else
						{
								keyok = true;
						}
					}
				}
				else
				{
					rc_last_repeat_key = 0;
				}
				rc_last_key = rc_key;

				if(abs(now_pressed-last_keypress)>repeat_block_generic)
				{
					if(keyok)
					{
						last_keypress = now_pressed;
						return translate(rc_key);
					}
				}
				//printf("!!!!!!!eat  native key: %04x %04x\n", rc_key, rc_key&0x1f );
			}
		}
		Timeout--;
		if (Timeout==-1)
		{
			return RC_timeout;
		}
		usleep(100000);
	}
	return rc_key;
}



int CRCInput::pushbackKey (int key)
{
	pb_keys.add( key );
	return 0;
}



void CRCInput::clear (void)
{
	while (getKey(0)!=RC_timeout)
	{}
}

/**************************************************************************
*       isNumeric - test if key is 0..9
*
**************************************************************************/
bool CRCInput::isNumeric(int key)
{
	if( (key>=RC_0) && (key<=RC_9))
		return true;
	else
		return false;
}

/**************************************************************************
*       transforms the rc-key to string
*
**************************************************************************/
string CRCInput:: getKeyName(int code)
{
	switch(code)
	{
			case RC_standby:
			return "standby";
			case RC_home:
			return "home";
			case RC_setup:
			return "setup";
			case RC_0:
			return "0";
			case RC_1:
			return "1";
			case RC_2:
			return "2";
			case RC_3:
			return "3";
			case RC_4:
			return "4";
			case RC_5:
			return "5";
			case RC_6:
			return "6";
			case RC_7:
			return "7";
			case RC_8:
			return "8";
			case RC_9:
			return "9";
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
			case RC_top_left:
			return "cursor top+left";
			case RC_top_right:
			return "cursor top+right";
			case RC_bottom_left:
			return "cursor bottom+left";
			case RC_bottom_right:
			return "cursor bottom+right";
			case RC_timeout:
			return "timeout";
			case RC_nokey:
			return "none";

			default:
			return "unknown";
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
				//perror("unknown old rc code");
				return RC_nokey;
		}
	}
	else if (!(code&0x00))
		return code&0x3F;
	//else
	//perror("unknown rc code");
	return RC_nokey;
}
