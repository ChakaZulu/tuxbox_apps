/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerdMsg.h,v 1.9 2002/05/27 21:53:30 dirch Exp $

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

		enum externalcommands 
		{
			CMD_VCR_UNKNOWN =	0;
			CMD_VCR_START	=	1;
			CMD_VCR_STOP	=	2;
			CMD_VCR_PAUSE	=	3;
			CMD_VCR_RESUME	=	4;
		};


		struct externalCommand
		{
			unsigned char		messageType;		// maybe vcr or server ?
			unsigned char		version;			// ACTVERSION
			unsigned int		command;			// externalcommands
			unsigned long long	epgID;				// may be zero
			unsigned int		onidsid;			// may be zero
		};

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
			unsigned data;
		};
		typedef std::vector<responseGetTimer> TimerList;


};

#endif
