/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerdclient.h,v 1.16 2002/09/05 22:31:24 dirch Exp $

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


#ifndef __timerdclient__
#define __timerdclient__

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>

#include "timerdMsg.h"
#include "../timermanager.h"

using namespace std;

class CTimerdClient
{

	private:

		int sock_fd;

		bool timerd_connect();
		bool timerd_close();
		bool send(char* data, int size);
		bool receive(char* data, int size);

	public:
		enum events
		{
			EVT_SHUTDOWN = 1,
			EVT_ANNOUNCE_SHUTDOWN,
			EVT_ZAPTO,
			EVT_ANNOUNCE_ZAPTO,
			EVT_NEXTPROGRAM,
			EVT_ANNOUNCE_NEXTPROGRAM,
			EVT_STANDBY_ON,
			EVT_STANDBY_OFF,
			EVT_RECORD_START,
			EVT_RECORD_STOP,
			EVT_ANNOUNCE_RECORD,
			EVT_ANNOUNCE_SLEEPTIMER,
			EVT_SLEEPTIMER
		};


		CTimerdClient::CTimerdClient();

		void registerEvent(unsigned int eventID, unsigned int clientID, string udsName);
		void unRegisterEvent(unsigned int eventID, unsigned int clientID);

		bool isTimerdAvailable();			// check if timerd is running

		int addTimerEvent( CTimerEvent::CTimerEventTypes evType, void* data, time_t alarmtime,time_t announcetime = 0, time_t stoptime = 0, CTimerEvent::CTimerEventRepeat evrepeat = CTimerEvent::TIMERREPEAT_ONCE);
//		int addTimerEvent( CTimerEvent::CTimerEventTypes evType, void* data, int min, int hour, int day = 0, int month = 0, CTimerEvent::CTimerEventRepeat evrepeat = CTimerEvent::TIMERREPEAT_ONCE);

		void removeTimerEvent( int evId);	// remove timer event

		void getTimerList( CTimerd::TimerList &timerlist);		// returns the list of all timers
		void getTimer( CTimerd::responseGetTimer &timer, unsigned timerID);		// returns specified timer

		// modify existing timer event
		bool modifyTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerEvent::CTimerEventRepeat evrepeat = CTimerEvent::TIMERREPEAT_ONCE);

		// set existing sleeptimer to new times or create new sleeptimer with these times
		int setSleeptimer(time_t announcetime, time_t alarmtime, int timerid = 0);		

		// returns the id of sleeptimer, 0 of no sleeptimer exists
		int getSleeptimerID();
		// returns remaining mins, -1 if no sleeptimer exists
		int getSleepTimerRemaining();


		// add diff to existing timer event
		bool rescheduleTimerEvent(int eventid, time_t diff);

		// add diff to existing timer event
		bool rescheduleTimerEvent(int eventid, time_t announcediff, time_t alarmdiff, time_t stoptime);

		// adds new sleeptimer event
		int addSleepTimerEvent(time_t announcetime,time_t alarmtime)	// sleeptimer setzen
			{return addTimerEvent(CTimerEvent::TIMER_SLEEPTIMER, NULL, announcetime, alarmtime, 0);};

		// adds new shutdown timer event
		int addShutdownTimerEvent(time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0)
			{return addTimerEvent(CTimerEvent::TIMER_SHUTDOWN, NULL, announcetime, alarmtime, stoptime);};

		// adds new record timer event
		int addRecordTimerEvent(unsigned onidSid,unsigned long long epgID,time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0) 
			{
				CTimerEvent::EventInfo eventInfo;
				eventInfo.onidSid = onidSid;
				eventInfo.epgID = epgID;
				printf("[TIMERDCLIENTLIB] ONI-SID %d\n",eventInfo.onidSid );
				return addTimerEvent(CTimerEvent::TIMER_RECORD, &eventInfo, announcetime, alarmtime, stoptime);
			};

		// adds new standby timer event
		int addStandbyTimerEvent(bool standby_on,time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0) 
			{return addTimerEvent(CTimerEvent::TIMER_STANDBY, &standby_on,  announcetime, alarmtime, stoptime);};

		// adds new zapto timer event
		int addZaptoTimerEvent(unsigned onidSid,unsigned long long epgID,time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0) 
		{
			CTimerEvent::EventInfo eventInfo;
			eventInfo.onidSid = onidSid;
			eventInfo.epgID = epgID;
			return addTimerEvent(CTimerEvent::TIMER_ZAPTO, &eventInfo, announcetime, alarmtime, stoptime);
		};

		int addNextProgramTimerEvent(CTimerEvent::EventInfo eventInfo,time_t alarmtime, time_t announcetime = 0, time_t stoptime = 0)
		{
			// mal auf verdacht eingebaut
			// keine ahnung ob / was hier noch fehlt
			return addTimerEvent(CTimerEvent::TIMER_NEXTPROGRAM, &eventInfo, alarmtime, announcetime, stoptime);
		};
};

#endif
