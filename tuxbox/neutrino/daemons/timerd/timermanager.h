/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timermanager.h,v 1.7 2002/05/14 23:10:36 dirch Exp $

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

#include "timerdclient.h"
#include "clientlib/zapitclient.h"
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
		CZapitClient		*zapitClient;	
		CTimerEventMap		events;
		pthread_t			thrTimer;

		CTimerManager();
		static void* timerThread(void *arg);
		CTimerEvent			*nextEvent();

	public:
		static CTimerManager* getInstance();

		CEventServer* getEventServer() {return eventServer;};
		CZapitClient* getZapitClient() {return zapitClient;};
		int addEvent(CTimerEvent*);
		void removeEvent(int eventID);
		CTimerEvent* getNextEvent();
		CTimerEventMap getEvents(){return events;};
		void listEvents(CTimerEventMap &Events);


};


class CTimerEvent
{
	public:
		int				eventID;
		int				eventType;
		struct tm		alarmtime;

		CTimerEvent( int mon = 0, int day = 0, int hour = 0, int min = 0, int evType = 0) :
			eventType( evType) { alarmtime.tm_mon = mon; alarmtime.tm_mday = day; alarmtime.tm_hour = hour; alarmtime.tm_min = min;};

		inline int time();
		bool operator <= ( CTimerEvent&);
		bool operator >= ( CTimerEvent&);
		void printEvent(void);
		static CTimerEvent now();

		virtual void fireEvent(){};
};

class CTimerEvent_Shutdown : public CTimerEvent
{
	public:
		CTimerEvent_Shutdown( int mon = 0, int day = 0, int hour = 0, int min = 0) :
			CTimerEvent(mon, day, hour, min, CTimerdClient::TIMER_SHUTDOWN){};
		virtual void fireEvent();
};


class CTimerEvent_Standby : public CTimerEvent
{
	public:
		bool standby_on;

		CTimerEvent_Standby( int mon = 0, int day = 0, int hour = 0, int min = 0) :
			CTimerEvent(mon, day, hour, min, CTimerdClient::TIMER_STANDBY){};
		virtual void fireEvent();
};

class CTimerEvent_Record : public CTimerEvent
{
	public:

		CTimerEvent_Record( int mon = 0, int day = 0, int hour = 0, int min = 0) :
			CTimerEvent(mon, day, hour, min, CTimerdClient::TIMER_RECORD){};
		virtual void fireEvent();
};

class CTimerEvent_NextProgram : public CTimerEvent
{
	public:

		CTimerd::EventInfo eventInfo;

		typedef map< int, CTimerEvent_NextProgram*> EventMap;
		static EventMap events;

		CTimerEvent_NextProgram( int mon = 0, int day = 0, int hour = 0, int min = 0) :
			CTimerEvent(mon, day, hour, min, CTimerdClient::TIMER_NEXTPROGRAM){};
		virtual void fireEvent();
};


#endif
