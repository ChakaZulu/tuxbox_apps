/*
	Timer-Daemon  -   DBoxII-Project

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


#ifndef __timerd__
#define __timerd__

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>

using namespace std;


#define TIMERD_UDS_NAME "/tmp/timerd.sock"


class CTimerd
{

	public:

		static const char ACTVERSION = 1;

		enum commands
		{
			CMD_ADDTIMER = 1,
			CMD_REMOVETIMER,
			CMD_GETTIMER,
			CMD_GETTIMERLIST,

			CMD_REGISTEREVENT,
			CMD_UNREGISTEREVENT,
		};


		//command structures
		struct commandHead
		{
			unsigned char messageType;
			unsigned char version;
			unsigned char cmd;
		};

		struct commandAddTimer
		{
			int   month;
			int   day;
			int   hour;
			int   min;
			int   evType;
		};

		struct responseAddTimer
		{
			int   eventID;
		};
};

#endif
