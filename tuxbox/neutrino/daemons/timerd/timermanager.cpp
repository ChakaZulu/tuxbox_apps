/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/



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

#include "timermanager.h"
#include "timerdclient.h"
#include "debug.h"



CTimerManager::CTimerManager()
{
	eventID = 0;
	//thread starten
	if (pthread_create (&thrTimer, NULL, timerThread, (void *) this) != 0 )
	{
		dprintf("CTimerManager::CTimerManager create timerThread failed\n");
	}
	dprintf("timermanager created\n");
}

CTimerManager* CTimerManager::getInstance()
{
	static CTimerManager *instance=NULL;
	if(!instance)
	{
		instance = new CTimerManager;
	}
	return instance;
}


void* CTimerManager::timerThread(void *arg)
{
	CTimerManager *timerManager = (CTimerManager*) arg;
	while (1)
	{
		//tooooooooooooooooooooooooooooooooodoooo	
	}
}

CTimerEvent* CTimerManager::getNextEvent()
{
	int lngTime = (events[0]->alarmtime.tm_mon+ 1)* 1000000+ (events[0]->alarmtime.tm_mday)* 10000+
		(events[0]->alarmtime.tm_hour)* 100+ events[0]->alarmtime.tm_min;	
	CTimerEvent *erg = events[0];
	std::map<int, CTimerEvent*>::iterator pos = events.begin();
	for(;pos!=events.end();pos++)
	{
		CTimerEvent *tmp = pos->second;
		int lngTime2 = (tmp->alarmtime.tm_mon+ 1)* 1000000+ (tmp->alarmtime.tm_mday)* 10000+
			(tmp->alarmtime.tm_hour)* 100+ tmp->alarmtime.tm_min;	

		if(lngTime2<lngTime)
		{
			erg = events[0];
			lngTime=lngTime2;
		}
	}
	return erg;
}

int CTimerManager::addEvent(CTimerEvent* evt)
{
	eventID++;
	evt->eventID = eventID;
	events[eventID] = evt;
	return eventID;
}

void CTimerManager::removeEvent(int eventID)
{
	if(events[eventID])
	{
		delete events[eventID];
	}
	events.erase(eventID);
}


//------------------------------------------------------------

CTimerEvent_Shutdown::CTimerEvent_Shutdown()
{
	eventType = CTimerdClient::TIMER_SHUTDOWN;
}

void CTimerEvent_Shutdown::fireEvent()
{
	//event in neutrinos remoteq. schreiben
}
