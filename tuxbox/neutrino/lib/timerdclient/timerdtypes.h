/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerdtypes.h,v 1.8 2002/12/03 11:15:11 thegoodguy Exp $

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


#ifndef __timerdtypes__
#define __timerdtypes__

#include <vector>

#include <zapit/client/zapittypes.h>


#define REMINDER_MESSAGE_MAXLEN 31

class CTimerd
{
	public:
		enum CTimerEventRepeat 
		{ 
			TIMERREPEAT_ONCE = 0,
			TIMERREPEAT_DAILY, 
			TIMERREPEAT_WEEKLY, 
			TIMERREPEAT_BIWEEKLY, 
			TIMERREPEAT_FOURWEEKLY, 
			TIMERREPEAT_MONTHLY, 
			TIMERREPEAT_BYEVENTDESCRIPTION,
			TIMERREPEAT_WEEKDAYS = 0x100 // Bits 9-15 specify weekdays (9=mo,10=di,...)
		};

		enum CTimerEventTypes
		{
			TIMER_SHUTDOWN = 1,
			TIMER_NEXTPROGRAM,
			TIMER_ZAPTO,
			TIMER_STANDBY,
			TIMER_RECORD,
			TIMER_REMIND,
			TIMER_SLEEPTIMER
		};
		
		enum CTimerEventStates 
		{ 
			TIMERSTATE_SCHEDULED, 
			TIMERSTATE_PREANNOUNCE, 
			TIMERSTATE_ISRUNNING, 
			TIMERSTATE_HASFINISHED, 
			TIMERSTATE_TERMINATED 
		};
		enum CChannelMode
		{
			MODE_TV=1,
			MODE_RADIO
		};

		struct EventInfo
		{
			unsigned long long epgID;
			t_channel_id       channel_id;
			uint               apid;
			CChannelMode       mode;
		};
		
		class RecordingInfo : public EventInfo
		{
		   public:
				RecordingInfo(EventInfo& e)
				{
					apid = e.apid;
					channel_id = e.channel_id;
					epgID = e.epgID;
					mode = e.mode;
				};
				RecordingInfo& operator = (EventInfo& e)
				{
					apid = e.apid;
					channel_id = e.channel_id;
					epgID = e.epgID;
					mode = e.mode;
					return *this;
				}

			int eventID;
		};

		struct RecordingStopInfo
		{
			int eventID;
		};

		struct responseGetTimer
		{		
			int								eventID;
			CTimerEventTypes	eventType;
			CTimerEventStates	eventState;
			CTimerEventRepeat	eventRepeat;
			time_t							alarmTime;
			time_t							announceTime;
			time_t							stopTime;
			t_channel_id channel_id; //only filled if applicable
			unsigned long long epgID; //only filled if applicable
			uint apid; //only filled if applicable
			CChannelMode mode; //only filled if applicable
			bool standby_on; //only filled if applicable
			char message[REMINDER_MESSAGE_MAXLEN]; //only filled if applicable
			bool operator< (const responseGetTimer& a) const
			{
				return this->alarmTime < a.alarmTime ;
			}
		};
		
		typedef std::vector<responseGetTimer> TimerList;
};
#endif
