/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timermanager.h,v 1.14 2002/06/24 23:17:18 dirch Exp $

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

//#include "timerdclient.h"
#include "eventserver.h"

using namespace std;

class CTimerEvent;
typedef map<int, CTimerEvent*> CTimerEventMap;


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
		int addEvent(CTimerEvent*);
		bool removeEvent(int eventID);
		CTimerEvent* getNextEvent();
		bool listEvents(CTimerEventMap &Events);
		int modifyEvent(int eventID, time_t announceTime, time_t alarmTime, time_t stopTime);
		int rescheduleEvent(int eventID, time_t announceTime, time_t alarmTime, time_t stopTime);

};


class CTimerEvent
{
	public:
		struct EventInfo
		{
			unsigned long long epgID;
			int      onidSid;
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
		
		void setState(CTimerEventStates newstate){previousState = eventState; eventState = newstate;};

		int remain_min(time_t t){return (t - time(NULL)) / 60;};
		void printEvent(void);
		void Reschedule();

		virtual void fireEvent(){};
		virtual void stopEvent(){};
		virtual void announceEvent(){};

};

class CTimerEvent_Shutdown : public CTimerEvent
{
	public:
		CTimerEvent_Shutdown( time_t announceTime, time_t alarmTime, time_t stopTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
			CTimerEvent(TIMER_SHUTDOWN, announceTime, alarmTime, stopTime, evrepeat ){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
};

class CTimerEvent_Sleeptimer : public CTimerEvent
{
	public:
		CTimerEvent_Sleeptimer( time_t announceTime, time_t alarmTime, time_t stopTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
			CTimerEvent(TIMER_SLEEPTIMER, announceTime, alarmTime, stopTime,evrepeat ){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
};


class CTimerEvent_Standby : public CTimerEvent
{
	public:
		bool standby_on;

		CTimerEvent_Standby( time_t announceTime, time_t alarmTime, time_t stopTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
			CTimerEvent(TIMER_STANDBY, announceTime, alarmTime, stopTime, evrepeat){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
};

class CTimerEvent_Record : public CTimerEvent
{
	public:
		CTimerEvent::EventInfo eventInfo;

		CTimerEvent_Record( time_t announceTime, time_t alarmTime, time_t stopTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
			CTimerEvent(TIMER_RECORD, announceTime, alarmTime, stopTime, evrepeat){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
};

class CTimerEvent_NextProgram : public CTimerEvent
{
	public:
		CTimerEvent::EventInfo eventInfo;

//		typedef map< int, CTimerEvent_NextProgram*> EventMap;
//		static EventMap events;

		CTimerEvent_NextProgram( time_t announceTime, time_t alarmTime, time_t stopTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
			CTimerEvent(TIMER_NEXTPROGRAM, announceTime, alarmTime, stopTime, evrepeat){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
};

class CTimerEvent_Zapto : public CTimerEvent
{
	public:

		CTimerEvent::EventInfo eventInfo;

		CTimerEvent_Zapto( time_t announceTime, time_t alarmTime, time_t stopTime, CTimerEventRepeat evrepeat = TIMERREPEAT_ONCE) :
			CTimerEvent(TIMER_ZAPTO, announceTime, alarmTime, stopTime, evrepeat){};
		virtual void fireEvent();
		virtual void announceEvent();
		virtual void stopEvent();
};


#endif
