/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timermanager.cpp,v 1.20 2002/08/27 15:27:39 dirch Exp $

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
#include <unistd.h>

#include "timermanager.h"
#include "debug.h"
#include "clientlib/timerdclient.h"


//CTimerEvent_NextProgram::EventMap CTimerEvent_NextProgram::events;


//------------------------------------------------------------
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
		time_t now = time(NULL);
		dprintf("Timer Thread time: %u\n", (uint) now);

		// fire events who's time has come
		CTimerEvent *event;
		CTimerEventMap::iterator pos = timerManager->events.begin();
		for(;pos != timerManager->events.end();pos++)
		{
			event = pos->second;
			if(debug) event->printEvent();					// print all events (debug)

			if(event->announceTime > 0 && event->eventState == CTimerEvent::TIMERSTATE_SCHEDULED ) // if event wants to be announced
				if( event->announceTime <= now ) // check if event announcetime has come
				{
					event->setState(CTimerEvent::TIMERSTATE_PREANNOUNCE);
					event->announceEvent();							// event specific announce handler
				}
			
			if(event->alarmTime > 0 && (event->eventState == CTimerEvent::TIMERSTATE_SCHEDULED || event->eventState == CTimerEvent::TIMERSTATE_PREANNOUNCE) ) // if event wants to be fired
				if( event->alarmTime <= now ) // check if event alarmtime has come
				{
					event->setState(CTimerEvent::TIMERSTATE_ISRUNNING);
					event->fireEvent();										// fire event specific handler
					if(event->stopTime == 0)					// if event needs no stop event
						event->setState(CTimerEvent::TIMERSTATE_HASFINISHED); 
				}
			
			if(event->stopTime > 0 && event->eventState == CTimerEvent::TIMERSTATE_ISRUNNING  )		// check if stopevent is wanted
				if( event->stopTime <= now ) // check if event stoptime has come
				{
					event->stopEvent();							//  event specific stop handler
					event->setState(CTimerEvent::TIMERSTATE_HASFINISHED); 
				}

			if(event->eventState == CTimerEvent::TIMERSTATE_HASFINISHED)
			{
				if(event->eventRepeat != CTimerEvent::TIMERREPEAT_ONCE)
					event->Reschedule();
				else
					event->setState(CTimerEvent::TIMERSTATE_TERMINATED);
			}

			if(event->eventState == CTimerEvent::TIMERSTATE_TERMINATED)				// event is terminated, so delete it
			{
				delete pos->second;										// delete event
				timerManager->events.erase( pos->first);				// remove from list
			}
		}

		(debug)?usleep(10 * 1000000):usleep(20 * 1000000);		// sleep for 10 / 20 seconds
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
	eventID++;						// increase unique event id
	evt->eventID = eventID;
	events[eventID] = evt;			// insert into events
	return eventID;					// return unique id
}

//------------------------------------------------------------
bool CTimerManager::removeEvent(int eventID)
{
	if(events.find(eventID)!=events.end())	// if i have a event with this id
	{
		if( (events[eventID]->eventState == CTimerEvent::TIMERSTATE_ISRUNNING) && (events[eventID]->stopTime > 0) )	
			events[eventID]->stopEvent();	// if event is running an has stopTime 

		events[eventID]->eventState = CTimerEvent::TIMERSTATE_TERMINATED;		// set the state to terminated
		return true;															// so timerthread will do the rest for us
//		delete events[eventID];
	}
	else
		return false;
//	events.erase(eventID);
}

//------------------------------------------------------------
bool CTimerManager::listEvents(CTimerEventMap &Events)
{
	if(!&Events)
		return false;
	Events.clear();
	if(getInstance()->events.size() > 0)
	{
		CTimerEventMap::iterator pos = getInstance()->events.begin();
		for(int i = 0;pos != getInstance()->events.end();pos++,i++)
			Events[pos->second->eventID] = pos->second;
		return true;
	}
	else
		return false;
}

int CTimerManager::modifyEvent(int eventID, time_t announceTime, time_t alarmTime, time_t stopTime)
{
	if(events.find(eventID)!=events.end())
	{
		CTimerEvent *event = events[eventID];
		event->announceTime = announceTime;
		event->alarmTime = alarmTime;
		event->stopTime = stopTime;
		event->eventState = CTimerEvent::TIMERSTATE_SCHEDULED;
	}
	return eventID;
}

int CTimerManager::rescheduleEvent(int eventID, time_t announceTime, time_t alarmTime, time_t stopTime)
{
	if(events.find(eventID)!=events.end())
	{
		CTimerEvent *event = events[eventID];
		if(event->announceTime > 0)
			event->announceTime += announceTime;
		if(event->alarmTime > 0)
			event->alarmTime += alarmTime;
		if(event->stopTime > 0)
			event->stopTime += stopTime;
		event->eventState = CTimerEvent::TIMERSTATE_SCHEDULED;
	}
	return eventID;
}


//------------------------------------------------------------
//=============================================================
// event functions
//=============================================================
//------------------------------------------------------------
CTimerEvent::CTimerEvent( CTimerEventTypes evtype, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerEventRepeat evrepeat)
{
	eventRepeat = evrepeat;
	eventState = TIMERSTATE_SCHEDULED; 
	eventType = evtype;
	announceTime = announcetime;
	alarmTime = alarmtime;
	stopTime = stoptime;
}

//------------------------------------------------------------
CTimerEvent::CTimerEvent( CTimerEventTypes evtype, int mon, int day, int hour, int min, CTimerEventRepeat evrepeat)
{ 
	
	time_t mtime = time(NULL);
	struct tm *tmtime = localtime(&mtime);

	if(mon > 0)
		tmtime->tm_mon = mon -1;	
	if(day > 0)
		tmtime->tm_mday = day;
	tmtime->tm_hour = hour;
	tmtime->tm_min = min;
	
	CTimerEvent(evtype, (time_t) 0, mktime(tmtime), (time_t)0, evrepeat);
}
//------------------------------------------------------------
void CTimerEvent::Reschedule()
{
	int diff = 0;
	int TAG = 60 * 60 * 24;	// sek * min * std
	printf("Reschedule\n");
	switch(eventRepeat)
	{
		case TIMERREPEAT_ONCE :
			break;
		case TIMERREPEAT_DAILY: 
				diff = TAG;
			break;
		case TIMERREPEAT_WEEKLY: 
				diff = TAG * 7;
			break;
		case TIMERREPEAT_BIWEEKLY: 
				diff = TAG * 14;
			break;
		case TIMERREPEAT_FOURWEEKLY: 
				diff = TAG * 28;
			break;
		case TIMERREPEAT_MONTHLY: 
				// hehe, son mist, todo
				diff = TAG * 28;
			break;
		case TIMERREPEAT_BYEVENTDESCRIPTION :
				// todo !!
			break;
		default:
			dprintf("unknown repeat type %d\n",eventRepeat);
	}
	if (diff != 0)
	{
		if (announceTime > 0)
			announceTime += diff;
		if (alarmTime > 0)
			alarmTime += diff;
		if (stopTime > 0)
			stopTime += diff;
		eventState = CTimerEvent::TIMERSTATE_SCHEDULED;
		dprintf("event %d rescheduled\n",eventID);
	}
	else
	{
		eventState = CTimerEvent::TIMERSTATE_TERMINATED;
		dprintf("event %d not rescheduled, event will be terminated\n",eventID);
	}
}


//------------------------------------------------------------
void CTimerEvent::printEvent(void)
{
	struct tm *alarmtime, *announcetime;
	dprintf("eventID: %03d type: %d state: %d repeat: %d ",eventID,eventType,eventState,eventRepeat);
	announcetime = localtime(&announceTime);
	dprintf("announce: %u %02d.%02d. %02d:%02d:%02d ",(uint) announceTime,announcetime->tm_mday,announcetime->tm_mon+1,announcetime->tm_hour,announcetime->tm_min,announcetime->tm_sec);
	alarmtime = localtime(&alarmTime);
	dprintf("alarm: %u %02d.%02d. %02d:%02d:%02d ",(uint) alarmTime,alarmtime->tm_mday,alarmtime->tm_mon+1,alarmtime->tm_hour,alarmtime->tm_min,alarmtime->tm_sec);
	switch(eventType)
	{		
		case CTimerEvent::TIMER_ZAPTO :
			dprintf("Zapto: %x epg: %llx\n",static_cast<CTimerEvent_NextProgram*>(this)->eventInfo.onidSid,static_cast<CTimerEvent_NextProgram*>(this)->eventInfo.epgID);
		break;

		case CTimerEvent::TIMER_RECORD :
			dprintf("Record: %x epg: %llx\n",static_cast<CTimerEvent_NextProgram*>(this)->eventInfo.onidSid,static_cast<CTimerEvent_NextProgram*>(this)->eventInfo.epgID);
		break;

		case CTimerEvent::TIMER_STANDBY :
			dprintf("standby: %s\n",(static_cast<CTimerEvent_Standby*>(this)->standby_on == 1)?"on":"off");
		break;

		default:
			dprintf("(no extra data)\n");
	}
}
//------------------------------------------------------------


//=============================================================
// Shutdown Event
//=============================================================
void CTimerEvent_Shutdown::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_ANNOUNCE_SHUTDOWN,
		CEventServer::INITID_TIMERD);
}
//------------------------------------------------------------
void CTimerEvent_Shutdown::stopEvent(){}

//------------------------------------------------------------
void CTimerEvent_Shutdown::fireEvent()
{
	dprintf("Shutdown Timer fired\n");
	//event in neutrinos remoteq. schreiben
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_SHUTDOWN,
		CEventServer::INITID_TIMERD);
}

//=============================================================
// Sleeptimer Event
//=============================================================
void CTimerEvent_Sleeptimer::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER,
		CEventServer::INITID_TIMERD);
}
//------------------------------------------------------------
void CTimerEvent_Sleeptimer::stopEvent(){}

//------------------------------------------------------------
void CTimerEvent_Sleeptimer::fireEvent()
{
	dprintf("Sleeptimer Timer fired\n");
	//event in neutrinos remoteq. schreiben
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_SLEEPTIMER,
		CEventServer::INITID_TIMERD);
}

//=============================================================
// Standby Event
//=============================================================

void CTimerEvent_Standby::announceEvent(){}
//------------------------------------------------------------
void CTimerEvent_Standby::stopEvent(){}
//------------------------------------------------------------

void CTimerEvent_Standby::fireEvent()
{
	dprintf("Standby Timer fired: %s\n",standby_on?"on":"off");
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		(standby_on)?CTimerdClient::EVT_STANDBY_ON:CTimerdClient::EVT_STANDBY_OFF,
		CEventServer::INITID_TIMERD);
}

//=============================================================
// Record Event
//=============================================================

void CTimerEvent_Record::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_ANNOUNCE_RECORD,
		CEventServer::INITID_TIMERD);
	dprintf("Record announcement\n"); 
}
//------------------------------------------------------------
void CTimerEvent_Record::stopEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_RECORD_STOP,
		CEventServer::INITID_TIMERD);
	dprintf("Recording stopped\n"); 
}
//------------------------------------------------------------

void CTimerEvent_Record::fireEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_RECORD_START,
		CEventServer::INITID_TIMERD,
		&eventInfo, sizeof(CTimerEvent::EventInfo));
	dprintf("Record Timer fired\n"); 
}

//=============================================================
// Zapto Event
//=============================================================

void CTimerEvent_Zapto::announceEvent(){}
//------------------------------------------------------------
void CTimerEvent_Zapto::stopEvent(){}
//------------------------------------------------------------

void CTimerEvent_Zapto::fireEvent()
{
	dprintf("Zapto Timer fired, onidSid: %d\n",eventInfo.onidSid);

//	CTimerManager::getInstance()->getZapitClient()->zapTo_serviceID(eventInfo.onidSid);
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_ZAPTO,
		CEventServer::INITID_TIMERD,
		&eventInfo,
		sizeof(CTimerEvent::EventInfo));
	dprintf("gefeuert\n");
}

//=============================================================
// NextProgram Event
//=============================================================

void CTimerEvent_NextProgram::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM,
		CEventServer::INITID_TIMERD,
		&eventInfo,
		sizeof(eventInfo));
}
//------------------------------------------------------------
void CTimerEvent_NextProgram::stopEvent(){}
//------------------------------------------------------------

void CTimerEvent_NextProgram::fireEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		CTimerdClient::EVT_NEXTPROGRAM,
		CEventServer::INITID_TIMERD,
		&eventInfo,
		sizeof(eventInfo));
}
//=============================================================
//=============================================================
