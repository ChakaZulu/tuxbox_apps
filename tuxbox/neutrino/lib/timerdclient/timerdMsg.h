/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerdMsg.h,v 1.6 2002/05/14 23:08:00 dirch Exp $

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
#include <vector>

using namespace std;


#define TIMERD_UDS_NAME "/tmp/timerd.sock"


class CTimerd
{

	public:

		struct EventInfo
		{
			int      uniqueKey;
			int      onidSid;
			char     name[50];
			int      fsk;
		};

		static const char ACTVERSION = 1;

		enum commands
		{
			CMD_ADDTIMER = 1,
			CMD_REMOVETIMER,
			CMD_GETTIMER,
			CMD_GETTIMERLIST,

			CMD_REGISTEREVENT,
			CMD_UNREGISTEREVENT,
			CMD_TIMERDAVAILABLE
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
			int   evType;
			int   month;
			int   day;
			int   hour;
			int   min;
		};


		struct responseAddTimer
		{
			int   eventID;
		};

		struct commandRemoveTimer
		{
			int   eventID;
		};

		struct responseAvailable
		{
			bool available;
		};
		
		struct commandGetTimer
		{
			int   eventID;
		};

		struct commandSetStandby
		{
			bool standby_on;
		};

		struct responseGetTimer
		{			
			int	  eventID;
			int   month;
			int   day;
			int   hour;
			int   min;
			int   eventType;
			unsigned data;
		};
		typedef std::vector<responseGetTimer> TimerList;


};

#endif
