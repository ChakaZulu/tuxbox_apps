/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerdclient.h,v 1.6 2002/05/14 23:07:25 dirch Exp $

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

		enum timerTypes
		{
			TIMER_SHUTDOWN = 1,
			TIMER_NEXTPROGRAM,
			TIMER_STANDBY,
			TIMER_RECORD
		};

		enum events
		{
			EVT_SHUTDOWN = 1,
			EVT_NEXTPROGRAM,
			EVT_STANDBY,
			EVT_RECORD
		};

		CTimerdClient::CTimerdClient();

		void registerEvent(unsigned int eventID, unsigned int clientID, string udsName);
		void unRegisterEvent(unsigned int eventID, unsigned int clientID);

		bool isTimerdAvailable();

		int addTimerEvent( timerTypes evType, void* data = 0, int min = 0, int hour = 0, int day = 0, int month = 0);
		void removeTimerEvent( int evId);

		void getTimerList( CTimerd::TimerList &timerlist);
		void getTimer( CTimerd::responseGetTimer &timer, unsigned timerID);


		int addShutdownTimerEvent(int min = 0, int hour = 0, int day = 0, int month = 0)
		{
			addTimerEvent(TIMER_SHUTDOWN,NULL,month, day, hour, min);
		};

		int addRecordTimerEvent(int min = 0, int hour = 0, int day = 0, int month = 0)
		{
			addTimerEvent(TIMER_RECORD,NULL,month, day, hour, min);
		};

		int addNextProgramTimerEvent(unsigned onidSid,int min = 0, int hour = 0, int day = 0, int month = 0)
		{
			CTimerd::EventInfo eventInfo;
			eventInfo.onidSid = onidSid;
			addTimerEvent(TIMER_NEXTPROGRAM,&eventInfo,month, day, hour, min);
		};

		int addStandbyTimerEvent(bool standby_on,int min = 0, int hour = 0, int day = 0, int month = 0)
		{
			CTimerd::commandSetStandby standby;
			standby.standby_on =standby_on;
			addTimerEvent(TIMER_STANDBY,&standby,month, day, hour, min);
		};
};

#endif
