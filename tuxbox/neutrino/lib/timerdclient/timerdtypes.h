/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerdtypes.h,v 1.14 2004/03/12 22:01:02 zwen Exp $

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

#include <zapit/client/zapittypes.h>
#include <sectionsdclient/sectionsdtypes.h>

#include <vector>

#define REMINDER_MESSAGE_MAXLEN 31
#define TIMERD_APIDS_MAXLEN 50

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
			TIMER_SLEEPTIMER,
			TIMER_IMMEDIATE_RECORD
		};
		
		enum CTimerEventStates 
		{ 
			TIMERSTATE_SCHEDULED, 
			TIMERSTATE_PREANNOUNCE, 
			TIMERSTATE_ISRUNNING, 
			TIMERSTATE_HASFINISHED, 
			TIMERSTATE_TERMINATED 
		};

		struct EventInfo
		{
			event_id_t   epgID;
			time_t       epg_starttime;
			t_channel_id channel_id;
			std::string  apids;
			bool         recordingSafety;
		};
		
		struct TransferEventInfo
		{
			event_id_t   epgID;
			time_t       epg_starttime;
			t_channel_id channel_id;
			char         apids[TIMERD_APIDS_MAXLEN];
			bool         recordingSafety;
		};
		
		class RecordingInfo : public EventInfo
			{
			public:
				RecordingInfo(){};
				RecordingInfo(EventInfo& e)
					{
						strcpy(apids, e.apids.substr(0,TIMERD_APIDS_MAXLEN-1).c_str());
						channel_id = e.channel_id;
						epgID = e.epgID;
						epg_starttime = e.epg_starttime;
					};
				RecordingInfo& operator = (EventInfo& e)
					{
						strcpy(apids, e.apids.substr(0,TIMERD_APIDS_MAXLEN-1).c_str());
						channel_id = e.channel_id;
						epgID = e.epgID;
						epg_starttime = e.epg_starttime;
						return *this;
					}
				char apids[TIMERD_APIDS_MAXLEN];
				int eventID;
			};

		struct RecordingStopInfo
		{
			int eventID;
		};

		struct responseGetTimer
		{		
			int               eventID;
			CTimerEventTypes  eventType;
			CTimerEventStates eventState;
			CTimerEventRepeat eventRepeat;
			time_t            alarmTime;
			time_t            announceTime;
			time_t            stopTime;
			t_channel_id      channel_id;                       //only filled if applicable
			event_id_t        epgID;                            //only filled if applicable
			time_t            epg_starttime;                    //only filled if applicable
			char              apids[TIMERD_APIDS_MAXLEN];       //only filled if applicable
			bool              standby_on;                       //only filled if applicable
			char              message[REMINDER_MESSAGE_MAXLEN]; //only filled if applicable
			bool operator< (const responseGetTimer& a) const
			{
				return this->alarmTime < a.alarmTime ;
			}
		};
		
		typedef std::vector<responseGetTimer> TimerList;
};
#endif
