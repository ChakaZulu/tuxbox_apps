/*
	Timer Daemon  -   DBoxII-Project

	Copyright (C) 2002 Dirk Szymanski 'Dirch'

	$Id: timerdMsg.h,v 1.17 2002/09/24 21:10:42 Zwen Exp $

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
#include "../timermanager.h"

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
			CMD_MODIFYTIMER,
			CMD_GETSLEEPTIMER,
			CMD_RESCHEDULETIMER,

			CMD_REGISTEREVENT,
			CMD_UNREGISTEREVENT,
			CMD_TIMERDAVAILABLE,
			CMD_SHUTDOWN
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
			CTimerEvent::CTimerEventTypes	eventType;
			CTimerEvent::CTimerEventRepeat	eventRepeat;
			time_t							alarmTime;
			time_t							announceTime;
			time_t							stopTime;			
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

		struct responseGetSleeptimer
		{
			int   eventID;
		};

		struct commandSetStandby
		{
			bool standby_on;
		};

		struct commandModifyTimer
		{
			int			eventID;
			time_t		announceTime;
			time_t		alarmTime;
			time_t		stopTime;
			CTimerEvent::CTimerEventRepeat	eventRepeat;
		};

		struct responseGetTimer
		{			
			int								eventID;
			CTimerEvent::CTimerEventTypes	eventType;
			CTimerEvent::CTimerEventStates	eventState;
			CTimerEvent::CTimerEventRepeat	eventRepeat;
			time_t							alarmTime;
			time_t							announceTime;
			time_t							stopTime;
			t_channel_id channel_id; //only filled if applicable
			unsigned long long epgID; //only filled if applicable
			bool standby_on; //only filled if applicable
		};
		typedef std::vector<responseGetTimer> TimerList;


		struct responseStatus
		{
			bool status;
		};
};

#endif
