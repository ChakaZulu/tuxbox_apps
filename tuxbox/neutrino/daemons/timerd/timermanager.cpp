/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timermanager.cpp,v 1.13 2002/05/17 19:50:41 dirch Exp $

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

//------------------------------------------------------------
CTimerManager::CTimerManager()
{
	eventID = 0;
	eventServer = new CEventServer;
	zapitClient = new CZapitClient;

	//thread starten
	if (pthread_create (&thrTimer, NULL, timerThread, (void *) this) != 0 )
	{
		dprintf("CTimerManager::CTimerManager create timerThread failed\n");
	}
	dprintf("timermanager created\n");
}

//------------------------------------------------------------
CTimerManager* CTimerManager::getInstance()
{
	static CTimerManager *instance=NULL;
	if(!instance)
		instance = new CTimerManager;
	return instance;
}


//------------------------------------------------------------
void* CTimerManager::timerThread(void *arg)
{
	CTimerManager *timerManager = (CTimerManager*) arg;
	while (1)
	{
		CTimerEvent eNow = CTimerEvent::now();
		dprintf("our time: %d\n", eNow.time());

		// fire events who's time has come
		CTimerEventMap::iterator pos = timerManager->events.begin();
		for(;pos != timerManager->events.end();pos++)
		{
			if(debug) pos->second->printEvent();

			if( *(pos->second) <= eNow)
			{
				pos->second->fireEvent();							// fire event specific handler
			}
			else if (pos->second->time() <= eNow.time()+10)			// print events before their time has come
			{
				dprintf("soon starting: type: %d (%02d:%02d)\n", pos->second->eventType, pos->second->alarmtime.tm_hour, pos->second->alarmtime.tm_min);
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
		(debug)?usleep(20000000):usleep(60000000);
	}
}

//------------------------------------------------------------
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

//------------------------------------------------------------
int CTimerManager::addEvent(CTimerEvent* evt)
{
	eventID++;
	evt->eventID = eventID;
	events[eventID] = evt;
	return eventID;
}

//------------------------------------------------------------
void CTimerManager::removeEvent(int eventID)
{
	if(events[eventID])
	{
		delete events[eventID];
	}
	events.erase(eventID);
}

//------------------------------------------------------------
void CTimerManager::listEvents(CTimerEventMap &Events)
{
	Events.clear();
	CTimerEventMap::iterator pos = getInstance()->events.begin();
	for(int i = 0;pos != getInstance()->events.end();pos++,i++)
		Events[pos->second->eventID] = pos->second;
}
//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
void CTimerEvent::printEvent(void)
{
	dprintf("eventID: %03d type: %d time:%d (%02d.%02d. %02d:%02d) ",eventID,eventType,time(),alarmtime.tm_mday,alarmtime.tm_mon+1,alarmtime.tm_hour,alarmtime.tm_min)
	switch(eventType)
	{		
		case CTimerdClient::TIMER_ZAPTO :
			dprintf("Zapto: %u\n",static_cast<CTimerEvent_NextProgram*>(this)->eventInfo.onidSid);
		break;

		case CTimerdClient::TIMER_STANDBY :
			dprintf("standby: %s\n",(static_cast<CTimerEvent_Standby*>(this)->standby_on == 1)?"on":"off");
		break;

		default:
			dprintf("(no extra data)\n");
	}
}

//------------------------------------------------------------
int CTimerEvent::time()
{
	return( (alarmtime.tm_mon+ 1) * 1000000 +
			(alarmtime.tm_mday)   * 10000 +
			(alarmtime.tm_hour)   * 100 +
			 alarmtime.tm_min );
}

//------------------------------------------------------------
CTimerEvent CTimerEvent::now()
{
	CTimerEvent result = CTimerEvent( );

	time_t actTime_t;
	::time(&actTime_t);
	struct tm* actTime = localtime(&actTime_t);
	result.alarmtime = *actTime;

	return(result);
}

//------------------------------------------------------------
bool CTimerEvent::operator <= ( CTimerEvent& e)
{
	// todo: comparision over year borders: december < january!!
	return ( time() <= e.time());
}

//------------------------------------------------------------
bool CTimerEvent::operator >= ( CTimerEvent& e)
{
	return ( time() >= e.time());
}

//-----------------------------------------

void CTimerEvent_Shutdown::fireEvent()
{
	dprintf("Shutdown Timer fired\n");
	//event in neutrinos remoteq. schreiben
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_SHUTDOWN,
		CEventServer::INITID_TIMERD);
}

//-----------------------------------------
void CTimerEvent_Standby::fireEvent()
{
	dprintf("Standby Timer fired: %s\n",standby_on?"on":"off");
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		(standby_on)?CTimerdClient::EVT_STANDBY_ON:CTimerdClient::EVT_STANDBY_OFF,
		CEventServer::INITID_TIMERD);
}
//-----------------------------------------
void CTimerEvent_Record::fireEvent()
{
	dprintf("Record Timer fired\n"); 
}
//-----------------------------------------

void CTimerEvent_Zapto::fireEvent()
{
	dprintf("Zapto Timer fired, onidSid: %d\n",eventInfo.onidSid);
	CTimerManager::getInstance()->getZapitClient()->zapTo_serviceID(eventInfo.onidSid);
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
