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
$Id: rcinput.h,v 1.16 2002/02/25 19:32:26 field Exp $

 Module  RemoteControle Handling

History:
 $Log: rcinput.h,v $
 Revision 1.16  2002/02/25 19:32:26  field
 Events <-> Key-Handling umgestellt! SEHR BETA!

 Revision 1.15  2002/02/17 15:55:56  McClean
 prepare for keyboard - useless at the moment

 Revision 1.14  2002/01/29 17:26:51  field
 Jede Menge Updates :)

 Revision 1.13  2002/01/08 12:34:28  McClean
 better rc-handling - add flat-standby

 Revision 1.12  2002/01/08 03:08:20  McClean
 improve input-handling

 Revision 1.11  2002/01/03 20:03:20  McClean
 cleanup

 Revision 1.10  2001/12/25 03:28:42  McClean
 better pushback-handling

 Revision 1.9  2001/11/26 02:34:04  McClean
 include (.../../stuff) changed - correct unix-formated files now

 Revision 1.8  2001/11/15 11:42:41  McClean
 gpl-headers added

 Revision 1.7  2001/10/29 16:49:00  field
 Kleinere Bug-Fixes (key-input usw.)

 Revision 1.6  2001/10/11 21:00:56  rasc
 clearbuffer() fuer RC-Input bei Start,
 Klassen etwas erweitert...

 Revision 1.5  2001/10/01 20:41:08  McClean
 plugin interface for games - beta but nice.. :)

 Revision 1.4  2001/09/23 21:34:07  rasc
 - LIFObuffer Module, pushbackKey fuer RCInput,
 - In einige Helper und widget-Module eingebracht
   ==> harmonischeres Menuehandling
 - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)


*/



#ifndef __MOD_rcinput__
#define __MOD_rcinput__

#include <dbox/fp.h>
#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "ringbuffer.h"

#include <string>

using namespace std;

class CRCInput
{
	private:
		int         fd_rc;
		int			fd_keyb;
		int			fd_max;
		CRingBuffer pb_keys;

		void open();
		void close();
		int translate(int code);

	public:
		//rc-code definitions
		enum
		{
		    RC_standby=0x10, RC_home=0x1F, RC_setup=0x18, RC_0=0x0, RC_1=0x1,
		    RC_2=0x2, RC_3=0x3, RC_4=0x4, RC_5=0x5, RC_6=0x6, RC_7=0x7,
		    RC_8=0x8, RC_9=0x9, RC_blue=0x14, RC_yellow=0x12, RC_green=0x11,
		    RC_red=0x13, RC_page_up=0x54, RC_page_down=0x53, RC_up=0xC, RC_down=0xD,
		    RC_left=0xB, RC_right=0xA, RC_ok=0xE, RC_plus=0x15, RC_minus=0x16,
		    RC_spkr=0xF, RC_help=0x17, RC_top_left=27, RC_top_right=28, RC_bottom_left=29, RC_bottom_right=30,
		    RC_timeout=-1, RC_nokey=-2
		};

		enum
		{
			MSG_ignore 	= 0x00,
			MSG_handled	= 0x01,
			MSG_unhandled= 0x02,
			MSG_cancel_all= 0x03
		};

		static const int RC_KeyBoard = 0x4000;

		//only used for plugins (games) !!
		int getFileHandle()
		{
			return fd_rc;
		}
		void stopInput();
		void restartInput();

		int repeat_block;
		int repeat_block_generic;
		CRCInput();      //constructor - opens rc-device and starts needed threads
		~CRCInput();     //destructor - closes rc-device


		static bool isNumeric(int key);
		int  _getKey(int Timeout=-1, bool bAllowRepeatLR= false);     //get key from the input-device
		int  _pushbackKey (int key);      // push key back in buffer (like ungetc)
		void _clear (void);
		static string getKeyName(int);
		void getMsg(int *msg, uint* data, int Timeout=-1, bool bAllowRepeatLR= false);     //get message :) (calls _getKey at the moment)
		void pushbackMsg(int msg, uint data);     // push message back into buffer - (calls _pushbackKey at the moment)
		void clearMsg(int type);	// bestimmte Msgs aus der Schleife löschen - löscht zZ ALLES :(
};

#endif



