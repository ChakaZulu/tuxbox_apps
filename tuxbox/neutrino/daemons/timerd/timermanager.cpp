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
#include "debug.h"
#include <unistd.h>

CTimerEvent_NextProgram::EventMap CTimerEvent_NextProgram::events;

CTimerManager::CTimerManager()
{
	eventID = 0;
	eventServer = new CEventServer;

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
		CTimerEvent eNow = CTimerEvent::now();
		printf("[timerd] our time: %d\n", eNow.time());
		// fire events who's time has come
		CTimerEventMap::iterator pos = timerManager->events.begin();
		for(;pos != timerManager->events.end();pos++)
		{
			if( *(pos->second) <= eNow)
			{
				pos->second->fireEvent();
			}
			else if (pos->second->time() <= eNow.time()+10)
			{
				dprintf("[timerd] soon starting: %s (%d:%d)\n", static_cast<CTimerEvent_NextProgram*>(pos->second)->eventInfo.name, pos->second->alarmtime.tm_hour, pos->second->alarmtime.tm_min);
			}
		}

		// delete events who's time has gone
		pos = timerManager->events.begin();
		for(;pos != timerManager->events.end();pos++)
		{
			if( *(pos->second) <= eNow)
			{
				delete pos->second;
				timerManager->events.erase( pos->first);
			}
		}
		usleep( 60000000);
	}
}

CTimerEvent* CTimerManager::getNextEvent()
{
	CTimerEvent *erg = events[0];
	CTimerEventMap::iterator pos = events.begin();
	for(;pos!=events.end();pos++)
	{
		if(pos->second <= erg)
		{
			erg = pos->second;
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
int CTimerEvent::time()
{
	return( (alarmtime.tm_mon+ 1) * 1000000 +
			(alarmtime.tm_mday)   * 10000 +
			(alarmtime.tm_hour)   * 100 +
			 alarmtime.tm_min );
}

CTimerEvent CTimerEvent::now()
{
	CTimerEvent result = CTimerEvent( );

	time_t actTime_t;
	::time(&actTime_t);
	struct tm* actTime = localtime(&actTime_t);
	result.alarmtime = *actTime;

	return(result);
}

bool CTimerEvent::operator <= ( CTimerEvent& e)
{
	// todo: comparision over year borders: december < january!!
	return ( time() <= e.time());
}

bool CTimerEvent::operator >= ( CTimerEvent& e)
{
	return ( time() >= e.time());
}

//-----------------------------------------

void CTimerEvent_Shutdown::fireEvent()
{
	//event in neutrinos remoteq. schreiben
}

//-----------------------------------------

void CTimerEvent_NextProgram::fireEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_NEXTPROGRAM,
		CEventServer::INITID_TIMERD,
		&eventInfo,
		sizeof(eventInfo));
}
