/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timermanager.h,v 1.1 2002/10/13 11:35:03 woglinde Exp $

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

#ifndef __neutrino_timermanager__
#define __neutrino_timermanager__

#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <map>
#include <string>

#include "eventserver.h"
#include "../../libconfigfile/configfile.h"
#include "config.h"


#include <stdint.h>
typedef uint32_t t_channel_id; // should be the same as in zapit/clientlib/zapittypes.h


#define CONFIGFILE CONFIGDIR "/timerd.conf"
#define REMINDER_MESSAGE_MAXLEN 31

using namespace std;


class CTimerEvent;
typedef map<int, CTimerEvent*> CTimerEventMap;




class CTimerEvent
{
	public:
		struct EventInfo
		{
			unsigned long long epgID;
			t_channel_id       channel_id;
		};
		
		enum CTimerEventRepeat 
		{ 
			TIMERREPEAT_ONCE = 0,
			TIMERREPEAT_DAILY, 
			TIMERREPEAT_WEEKLY, 
			TIMERREPEAT_BIWEEKLY, 
			TIMERREPEAT_FOURWEEKLY, 
			TIMERREPEAT_MONTHLY, 
			TIMERREPEAT_BYEVENTDESCRIPTION
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

		int					eventID;			// event identifier
		CTimerEventTypes	eventType;			// Type of event
		CTimerEventStates	eventState;			// actual event state
		CTimerEventStates	previousState;			// previous event state
		CTimerEventRepeat	eventRepeat;

	// time values
		time_t		alarmTime;					// event start time
		time_t		stopTime;					// 0 = one time shot
		time_t		announceTime;				// when should event be announced (0=none)

		CTimerEvent( CTimerEventTypes evtype, int mon = 0, int day = 0, int hour = 0, int min = 0, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE);
		CTimerEvent( CTimerEventTypes evtype, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE);
                CTimerEvent( CTimerEventTypes evtype, CConfigFile *config, int iId);
		
		void setState(CTimerEventStates newstate){previousState = eventState; eventState = newstate;};

		int remain_min(time_t t){return (t - time(NULL)) / 60;};
		void printEvent(void);
		virtual void Reschedule();

		virtual void fireEvent(){};
		virtual void stopEvent(){};
		virtual void announceEvent(){};
                virtual void saveToConfig(CConfigFile *config);
};

class CTimerEvent_Shutdown : public CTimerEvent
{
	public:
		CTimerEvent_Shutdown( time_t announceTime, time_t alarmTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
                   CTimerEvent(TIMER_SHUTDOWN, announceTime, alarmTime, (time_t) 0, evrepeat ){};
                CTimerEvent_Shutdown(CConfigFile *config, int iId):
                   CTimerEvent(TIMER_SHUTDOWN, config, iId){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
                virtual void saveToConfig(CConfigFile *config);
};

class CTimerEvent_Sleeptimer : public CTimerEvent
{
	public:
		CTimerEvent_Sleeptimer( time_t announceTime, time_t alarmTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
                   CTimerEvent(TIMER_SLEEPTIMER, announceTime, alarmTime, (time_t) 0,evrepeat ){};
                CTimerEvent_Sleeptimer(CConfigFile *config, int iId):
                   CTimerEvent(TIMER_SLEEPTIMER, config, iId){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
                virtual void saveToConfig(CConfigFile *config);
};


class CTimerEvent_Standby : public CTimerEvent
{
	public:
		bool standby_on;

		CTimerEvent_Standby( time_t announceTime, time_t alarmTime, 
									bool sb_on, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE);
		CTimerEvent_Standby(CConfigFile *config, int iId);
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
                virtual void saveToConfig(CConfigFile *config);
};

class CTimerEvent_Record : public CTimerEvent
{
	public:
		CTimerEvent::EventInfo eventInfo;

		CTimerEvent_Record( time_t announceTime, time_t alarmTime, time_t stopTime, 
                          t_channel_id channel_id, unsigned long long epgID=0, 
								  CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE);
		CTimerEvent_Record(CConfigFile *config, int iId);
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
		virtual void saveToConfig(CConfigFile *config);
		virtual void Reschedule();
};

class CTimerEvent_Zapto : public CTimerEvent
{
	public:

		CTimerEvent::EventInfo eventInfo;

		CTimerEvent_Zapto( time_t announceTime, time_t alarmTime, 
								 t_channel_id channel_id, unsigned long long epgID=0, 
								 CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE);
		CTimerEvent_Zapto(CConfigFile *config, int iId);
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
		virtual void saveToConfig(CConfigFile *config);
		virtual void Reschedule();
};

class CTimerEvent_NextProgram : public CTimerEvent
{
	public:
		CTimerEvent::EventInfo eventInfo;

		CTimerEvent_NextProgram( time_t announceTime, time_t alarmTime, time_t stopTime, 
										 t_channel_id channel_id, unsigned long long epgID=0, 
										 CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE);
		CTimerEvent_NextProgram(CConfigFile *config, int iId);
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
		virtual void saveToConfig(CConfigFile *config);
		virtual void Reschedule();
};

class CTimerEvent_Remind : public CTimerEvent
{
	public:
		char message[REMINDER_MESSAGE_MAXLEN];

		CTimerEvent_Remind( time_t announceTime, time_t alarmTime, 
								  char* msg, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE);
		CTimerEvent_Remind(CConfigFile *config, int iId);
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
		virtual void saveToConfig(CConfigFile *config);
};

class CTimerManager
{
	//singleton
	private:
		int					eventID;
		CEventServer		*eventServer;
		CTimerEventMap		events;
		pthread_t			thrTimer;

		CTimerManager();
		static void* timerThread(void *arg);
		CTimerEvent			*nextEvent();

	public:


		static CTimerManager* getInstance();

		CEventServer* getEventServer() {return eventServer;};
		int addEvent(CTimerEvent*,bool save = true);
		bool removeEvent(int eventID);
		CTimerEvent* getNextEvent();
		bool listEvents(CTimerEventMap &Events);
		int modifyEvent(int eventID, time_t announceTime, time_t alarmTime, time_t stopTime, CTimerEvent::CTimerEventRepeat evrepeat = CTimerEvent::TIMERREPEAT_ONCE);
		int rescheduleEvent(int eventID, time_t announceTime, time_t alarmTime, time_t stopTime);
		void saveEventsToConfig();
		bool shutdown();
};

#endif
