/*
	LCD-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean', Georg Lukas
	Homepage: http://mcclean.cyberphoria.org/



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

#ifndef __lcdd__
#define __lcdd__

#define SA struct sockaddr
#define SAI struct sockaddr_in

#define LCDD_PORT 1510

#define LCDD_VERSION 2

#define LC_SET_SETUP_OFF 0
#define LC_SET_SETUP_ON 1

#define LC_MUTE_OFF 0
#define LC_MUTE_ON 1

enum lcdd_cmd {
	LC_CHANNEL = 1,
	LC_VOLUME = 2,
	LC_MUTE,
	LC_SET_SETUP,
	LC_SETUP_MSG,
	LC_POWEROFF
};

struct lcdd_msg {
  unsigned char version;
  unsigned char cmd;
  unsigned char param;
  unsigned short param2;
  char param3[30];
  char param4[10][30];
}__attribute((packed));


#endif // __lcdd__
