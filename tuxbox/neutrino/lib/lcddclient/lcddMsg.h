/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/


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

#ifndef __lcddmsg__
#define __lcddmsg__

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>

using namespace std;


#define LCDD_UDS_NAME "/tmp/lcdd.sock"


class CLcddMsg
{

	public:

		static const char ACTVERSION = 4;

		enum commands
		{
			CMD_SETMODE = 1,
			CMD_SETSERVICENAME,
			CMD_SETMENUTEXT,
			CMD_SETVOLUME,
			CMD_SETMUTE,
			CMD_SETLCDBRIGHTNESS,
			CMD_GETLCDBRIGHTNESS,
			CMD_SETSTANDBYLCDBRIGHTNESS,
			CMD_GETSTANDBYLCDBRIGHTNESS

		};

		//command structures
		struct commandHead
		{
			unsigned char messageType;
			unsigned char version;
			unsigned char cmd;
		};

		struct commandMode
		{
			unsigned char mode;
			char text[30];
		};

		struct commandServiceName
		{
			char servicename[40];
		};

		struct commandMenuText
		{
			char position;
			char highlight;
			char text[30];
		};

		struct commandMute
		{
			bool mute;
		};

		struct commandVolume
		{
			char volume;
		};

		struct commandSetBrightness
		{
			int brightness;
		};


		struct responseGetBrightness
		{
			int brightness;
		};

};


#endif
