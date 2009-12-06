/*
 * $Id: io.c,v 1.1 2009/12/06 21:58:11 rhabarber1848 Exp $
 *
 * msgbox - d-box2 linux project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
*/

#include <stdio.h>
#include <time.h>
#include "msgbox.h"

int rc_last_key=-1;
int lastval=-1;
int RCTranslate(int code);
extern int rc;
time_t t1,t2;
clock_t tk1=0, tk2;

#if HAVE_DVB_API_VERSION == 3
__u16 rc_last_code = KEY_RESERVED;


void ClearKeys(void)
{
	if(rc>=0)
	{
		close(rc);
	}
	rc = open(RC_DEVICE, O_RDONLY);
	rc_last_key = KEY_RESERVED;
}


int RCKeyPressed(void)
{
int rval=0;

	if(read(rc, &ev, sizeof(ev)) == sizeof(ev))
	{
		if(ev.value==2)
		{
			tk2=clock()/(CLOCKS_PER_SEC/1000) ;
			if((tk2-tk1)<500)
			{
				usleep(250000L);
				if(read(rc, &ev, sizeof(ev)) == sizeof(ev))
				{
					if(ev.value)
					{
						lastval=ev.code;
						rval=ev.value;
						tk1=clock()/(CLOCKS_PER_SEC/1000);
					}
				}
			}
		}
		else
		{
			if(ev.value)
			{
				tk1=clock()/(CLOCKS_PER_SEC/1000);
			}
			lastval=ev.code;
			rval=ev.value;
		}
	}
	
	return rval;
}
#endif

int RCTranslate(int code)
{
	switch(code)
	{
		case KEY_UP:		rccode = RC_UP;
			break;

		case KEY_DOWN:		rccode = RC_DOWN;
			break;

		case KEY_LEFT:		rccode = RC_LEFT;
			break;

		case KEY_RIGHT:		rccode = RC_RIGHT;
			break;

		case KEY_OK:		rccode = RC_OK;
			break;

		case KEY_0:			rccode = RC_0;
			break;

		case KEY_1:			rccode = RC_1;
			break;

		case KEY_2:			rccode = RC_2;
			break;

		case KEY_3:			rccode = RC_3;
			break;

		case KEY_4:			rccode = RC_4;
			break;

		case KEY_5:			rccode = RC_5;
			break;

		case KEY_6:			rccode = RC_6;
			break;

		case KEY_7:			rccode = RC_7;
			break;

		case KEY_8:			rccode = RC_8;
			break;

		case KEY_9:			rccode = RC_9;
			break;

		case KEY_RED:		rccode = RC_RED;
			break;

		case KEY_GREEN:		rccode = RC_GREEN;
			break;

		case KEY_YELLOW:	rccode = RC_YELLOW;
			break;

		case KEY_BLUE:		rccode = RC_BLUE;
			break;

		case KEY_VOLUMEUP:	rccode = RC_PLUS;
			break;

		case KEY_VOLUMEDOWN:	rccode = RC_MINUS;
			break;

		case KEY_MUTE:		rccode = RC_MUTE;
			break;

		case KEY_HELP:		rccode = RC_HELP;
			break;

		case KEY_SETUP:		rccode = RC_DBOX;
			break;

		case KEY_HOME:		rccode = RC_HOME;
			break;

		case KEY_POWER:		rccode = RC_STANDBY;
			break;

		default:			rccode = -1;
	}

	return rccode;
	
}

#if HAVE_DVB_API_VERSION == 3
int GetCode(void)
{
	if(!RCKeyPressed() || (get_instance()>instance))
	{
		return -1;
	}

	do
	{
		rc_last_key = ev.code;
	}
	while(RCKeyPressed());
	
	return RCTranslate(rc_last_key);
}


int GetRCCode()
{
int b_key=-1,i=-1,debounce=200;
	
//	while(i==-1)
	{
		i=GetCode();
		if(i!=-1)
		{
			if(i==b_key)
			{
				usleep(debounce*1000);
				while((i=GetCode())!=-1);
			}
			b_key=i;
		}
	}
	return i;
}

int GetRCCodeNW()
{
int b_key=-1,i=-1,debounce=200;
	
	i=GetCode();
	if(i!=-1)
	{
		if(i==b_key)
		{
			usleep(debounce*1000);
			while((i=GetCode())!=-1);
		}
		b_key=i;
	}
	return i;
}

#else

char buf[32];
int x=0;

void ClearKeys(void)
{
  x=read(rc, &buf, 32);
}

int GetCode(void)
{
// Nix
}

int RCKeyPressed(void)
{
// Nix
}

int GetRCCode(void)
{

  x=read(rc, &rccode, 2);
  
  if (x < 2)
    return -1;
  
  if (lastkey == rccode)
      {
      key_count++;
      if (key_count < 3)
          return -1;
      } else
          key_count=0;

  lastkey=rccode;

  if((rccode & 0xFF00) == 0x5C00)
  {
      rccode=RCTranslate(rccode);
  }
  else 
       rccode = -1;

  return rccode;
}

int GetRCCodeNW(void)
{
  return GetRCCode();
}

#endif

