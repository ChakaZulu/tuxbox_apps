/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

   $Id: timermanager.cpp,v 1.68 2004/02/20 22:21:21 thegoodguy Exp $

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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sstream>

#include <dbox/fp.h>

#include <timermanager.h>
#include <timerdclient/timerdclient.h>
#include <debug.h>
#include <sectionsdclient/sectionsdclient.h>

#include <vector>

//CTimerEvent_NextProgram::EventMap CTimerEvent_NextProgram::events;


//------------------------------------------------------------
CTimerManager::CTimerManager()
{
	eventID = 0;
	eventServer = new CEventServer;
	m_saveEvents = false;
	m_isTimeSet = false;
	loadRecordingSafety();

	//thread starten
	if(pthread_create (&thrTimer, NULL, timerThread, (void *) this) != 0 )
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
	pthread_mutex_t dummy_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t dummy_cond = PTHREAD_COND_INITIALIZER;
	struct timespec wait;

	CTimerManager *timerManager = (CTimerManager*) arg;
	int sleeptime=(timerd_debug)?10:20;
	while(1)
	{
		if(!timerManager->m_isTimeSet)
		{ // time not set yet
			//Ceck if time is set now
			CSectionsdClient sectionsd;
			if (sectionsd.getIsTimeSet())
			{
				timerManager->m_isTimeSet=true;
				timerManager->loadEventsFromConfig();
			}
			else
			{
				dprintf("waiting for time to be set\n");
				wait.tv_sec = time(NULL) + 5 ;
				wait.tv_nsec = 0;
				pthread_cond_timedwait(&dummy_cond, &dummy_mutex, &wait);
			}
		}
		else
		{
			time_t now = time(NULL);
			dprintf("Timer Thread time: %u\n", (uint) now);

			// fire events who's time has come
			CTimerEvent *event;
			CTimerEventMap::iterator pos = timerManager->events.begin();
			for(;pos != timerManager->events.end();pos++)
			{
				event = pos->second;
				if(timerd_debug) event->printEvent();					// print all events (debug)

				if(event->announceTime > 0 && event->eventState == CTimerd::TIMERSTATE_SCHEDULED ) // if event wants to be announced
					if( event->announceTime <= now )	// check if event announcetime has come
					{
						event->setState(CTimerd::TIMERSTATE_PREANNOUNCE);
						event->announceEvent();							// event specific announce handler
						timerManager->m_saveEvents = true;
					}

				if(event->alarmTime > 0 && (event->eventState == CTimerd::TIMERSTATE_SCHEDULED || event->eventState == CTimerd::TIMERSTATE_PREANNOUNCE) )	// if event wants to be fired
					if( event->alarmTime <= now )	// check if event alarmtime has come
					{
						event->setState(CTimerd::TIMERSTATE_ISRUNNING);
						event->fireEvent();										// fire event specific handler
						if(event->stopTime == 0)					// if event needs no stop event
							event->setState(CTimerd::TIMERSTATE_HASFINISHED);
						timerManager->m_saveEvents = true;
					}

				if(event->stopTime > 0 && event->eventState == CTimerd::TIMERSTATE_ISRUNNING  )		// check if stopevent is wanted
					if( event->stopTime <= now ) // check if event stoptime has come
					{
						event->stopEvent();							//  event specific stop handler
						event->setState(CTimerd::TIMERSTATE_HASFINISHED); 
						timerManager->m_saveEvents = true;
					}

				if(event->eventState == CTimerd::TIMERSTATE_HASFINISHED)
				{
					if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						event->Reschedule();
					else
						event->setState(CTimerd::TIMERSTATE_TERMINATED);
					timerManager->m_saveEvents = true;
				}

				if(event->eventState == CTimerd::TIMERSTATE_TERMINATED)				// event is terminated, so delete it
				{
					delete pos->second;										// delete event
					timerManager->events.erase(pos);				// remove from list
					timerManager->m_saveEvents = true;
				}
			}
			// save events if requested
			if(timerManager->m_saveEvents)
			{
				timerManager->saveEventsToConfig();
				timerManager->m_saveEvents=false;
			}
/*			int wait = sleeptime-(((int)time(NULL)) % sleeptime);
			if(wait==0) wait=sleeptime;
			usleep(wait*1000000);*/
			wait.tv_sec = (((time(NULL) / sleeptime) * sleeptime) + sleeptime);
			wait.tv_nsec = 0;
			pthread_cond_timedwait(&dummy_cond, &dummy_mutex, &wait);
		}
	}
	return 0;
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
int CTimerManager::addEvent(CTimerEvent* evt, bool save)
{
	eventID++;						// increase unique event id
	evt->eventID = eventID;
	if(evt->eventRepeat==CTimerd::TIMERREPEAT_WEEKDAYS)
		// Weekdays without weekday specified reduce to once
		evt->eventRepeat=CTimerd::TIMERREPEAT_ONCE;
	events[eventID] = evt;			// insert into events
	m_saveEvents=save;
	return eventID;					// return unique id
}

//------------------------------------------------------------
bool CTimerManager::removeEvent(int eventID)
{
	if(events.find(eventID)!=events.end())							 // if i have a event with this id
	{
		if( (events[eventID]->eventState == CTimerd::TIMERSTATE_ISRUNNING) && (events[eventID]->stopTime > 0) )
			events[eventID]->stopEvent();	// if event is running an has stopTime 

		events[eventID]->eventState = CTimerd::TIMERSTATE_TERMINATED;		// set the state to terminated
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

	for (CTimerEventMap::iterator pos = events.begin(); pos != events.end(); pos++)
	{
		pos->second->Refresh();
		Events[pos->second->eventID] = pos->second;
	}

	return true;
}

int CTimerManager::modifyEvent(int eventID, time_t announceTime, time_t alarmTime, time_t stopTime, CTimerd::CTimerEventRepeat evrepeat)
{
	if(events.find(eventID)!=events.end())
	{
		CTimerEvent *event = events[eventID];
		event->announceTime = announceTime;
		event->alarmTime = alarmTime;
		event->stopTime = stopTime;
		if(event->eventState==CTimerd::TIMERSTATE_PREANNOUNCE)
			event->eventState = CTimerd::TIMERSTATE_SCHEDULED;
		event->eventRepeat = evrepeat;
		if(event->eventRepeat==CTimerd::TIMERREPEAT_WEEKDAYS)
			// Weekdays without weekday specified reduce to once
			event->eventRepeat=CTimerd::TIMERREPEAT_ONCE;
		m_saveEvents=true;
		return eventID;
	}
	else
		return 0;
}

int CTimerManager::modifyEvent(int eventID, const std::string apids)
{
	dprintf("Modify Event %d apid %s\n",eventID,apids.c_str());
	if(events.find(eventID)!=events.end())
	{
		CTimerEvent *event = events[eventID];
		if(event->eventType == CTimerd::TIMER_RECORD)
		{
			((CTimerEvent_Record*) (event))->eventInfo.apids = apids;
			m_saveEvents=true;
			return eventID;
		}
		else if(event->eventType == CTimerd::TIMER_ZAPTO)
		{
			((CTimerEvent_Zapto*) (event))->eventInfo.apids = apids;
			m_saveEvents=true;
			return eventID;
		}
	}
	return 0;
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
		event->eventState = CTimerd::TIMERSTATE_SCHEDULED;
		m_saveEvents=true;
		return eventID;
	}
	else
		return 0;
}
// ---------------------------------------------------------------------------------
void CTimerManager::loadEventsFromConfig()
{
	CConfigFile config(',');

	if(!config.loadConfig(CONFIGFILE))
	{
		/* set defaults if no configuration file exists */
		dprintf("%s not found\n", CONFIGFILE);
	}
	else
	{
		std::vector<int> savedIDs;
		savedIDs = config.getInt32Vector ("IDS");
		dprintf("%d timer(s) in config\n",savedIDs.size());
		for(unsigned int i=0; i < savedIDs.size(); i++)
		{
			std::stringstream ostr;
			ostr << savedIDs[i];
			std::string id=ostr.str();
			CTimerd::CTimerEventTypes type=(CTimerd::CTimerEventTypes)config.getInt32 ("EVENT_TYPE_"+id,0);
			time_t now = time(NULL);
			switch(type)
			{
				case CTimerd::TIMER_SHUTDOWN :
					{
						CTimerEvent_Shutdown *event=
						new CTimerEvent_Shutdown(&config, savedIDs[i]);
						if((event->alarmTime >= now) || (event->stopTime > now))
						{
							addEvent(event,false);
						}
						else if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						{
							// old periodic timers need to be rescheduled
							event->eventState = CTimerd::TIMERSTATE_HASFINISHED;
							addEvent(event,false);
						}
						else
						{
							dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
							delete event;
						}
						break;
					}       
				case CTimerd::TIMER_NEXTPROGRAM :
					{
						CTimerEvent_NextProgram *event=
						new CTimerEvent_NextProgram(&config, savedIDs[i]);
						if((event->alarmTime >= now) || (event->stopTime > now))
						{
							addEvent(event,false);
						}
						else if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						{
							// old periodic timers need to be rescheduled
							event->eventState = CTimerd::TIMERSTATE_HASFINISHED;
							addEvent(event,false);
						}
						else
						{
							dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
							delete event;
						}
						break;
					}       
				case CTimerd::TIMER_ZAPTO :
					{
						CTimerEvent_Zapto *event=
						new CTimerEvent_Zapto(&config, savedIDs[i]);
						if((event->alarmTime >= now) || (event->stopTime > now))
						{
							addEvent(event,false);
						}
						else if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						{
							// old periodic timers need to be rescheduled
							event->eventState = CTimerd::TIMERSTATE_HASFINISHED;
							addEvent(event,false);
						}
						else
						{
							dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
							delete event;
						}
						break;
					}          
				case CTimerd::TIMER_STANDBY :
					{
						CTimerEvent_Standby *event=
						new CTimerEvent_Standby(&config, savedIDs[i]);
						if((event->alarmTime >= now) || (event->stopTime > now))
						{
							addEvent(event,false);
						}
						else if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						{
							// old periodic timers need to be rescheduled
							event->eventState = CTimerd::TIMERSTATE_HASFINISHED;
							addEvent(event,false);
						}
						else
						{
							dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
							delete event;
						}
						break;
					}           
				case CTimerd::TIMER_RECORD :
					{
						CTimerEvent_Record *event=
						new CTimerEvent_Record(&config, savedIDs[i]);
						if((event->alarmTime >= now) || (event->stopTime > now))
						{
							addEvent(event,false);
						}
						else if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						{
							// old periodic timers need to be rescheduled
							event->eventState = CTimerd::TIMERSTATE_HASFINISHED;
							addEvent(event,false);
						}
						else
						{
							dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
							delete event;
						}
						break;
					}          
				case CTimerd::TIMER_SLEEPTIMER :
					{
						CTimerEvent_Sleeptimer *event=
						new CTimerEvent_Sleeptimer(&config, savedIDs[i]);
						if((event->alarmTime >= now) || (event->stopTime > now))
						{
							addEvent(event,false);
						}
						else if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						{
							// old periodic timers need to be rescheduled
							event->eventState = CTimerd::TIMERSTATE_HASFINISHED;
							addEvent(event,false);
						}
						else
						{
							dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
							delete event;
						}
						break;
					}
				case CTimerd::TIMER_REMIND :
					{
						CTimerEvent_Remind *event=
						new CTimerEvent_Remind(&config, savedIDs[i]);
						if((event->alarmTime >= now) || (event->stopTime > now))
						{
							addEvent(event,false);
						}
						else if(event->eventRepeat != CTimerd::TIMERREPEAT_ONCE)
						{
							// old periodic timers need to be rescheduled
							event->eventState = CTimerd::TIMERSTATE_HASFINISHED;
							addEvent(event,false);
						}
						else
						{
							dprintf("Timer too old %d/%d\n",(int)now,(int) event->alarmTime);
							delete event;
						}
						break;
					}
				default:
					dprintf("Unknown timer on load %d\n",type);
			}
		}
	}
	saveEventsToConfig();
}
// -------------------------------------------------------------------------------------
void CTimerManager::loadRecordingSafety()
{
	CConfigFile config(',');

	if(!config.loadConfig(CONFIGFILE))
	{
		/* set defaults if no configuration file exists */
		dprintf("%s not found\n", CONFIGFILE);
	}
	else
	{
		m_extraTimeStart = config.getInt32 ("EXTRA_TIME_START",0);
		m_extraTimeEnd  = config.getInt32 ("EXTRA_TIME_END",0);
	}
}
// -------------------------------------------------------------------------------------
void CTimerManager::saveEventsToConfig()
{
	// Sperren !!!
	CConfigFile config(',');
	config.clear();
	dprintf("[Timerd] save %d events to config ... saving ", events.size());
	CTimerEventMap::iterator pos = events.begin();
	for(;pos != events.end();pos++)
	{
		CTimerEvent *event = pos->second;
		dprintf("%d ",event->eventID);
		event->saveToConfig(&config);
	}
	dprintf("\n");
	config.setInt32 ("EXTRA_TIME_START", m_extraTimeStart);
	config.setInt32 ("EXTRA_TIME_END", m_extraTimeEnd);
	config.saveConfig(CONFIGFILE);
	// Freigeben !!!
}
//------------------------------------------------------------
bool CTimerManager::shutdown()
{

	time_t nextAnnounceTime=0;
	bool status=false;
	CTimerEventMap::iterator pos = events.begin();
	dprintf("Waiting for timermanager thread to terminate ...\n");
	pthread_cancel(thrTimer);
	dprintf("Timermanager thread terimanted\n");
	if(m_saveEvents)
      saveEventsToConfig();
   for(;pos != events.end();pos++)
	{
		CTimerEvent *event = pos->second;
		if((event->eventType == CTimerd::TIMER_RECORD ||
			 event->eventType == CTimerd::TIMER_ZAPTO ) &&
			event->eventState == CTimerd::TIMERSTATE_SCHEDULED)
		{
			// Wir wachen nur für Records und Zaptos wieder auf
			if(event->announceTime < nextAnnounceTime || nextAnnounceTime==0)
			{
				nextAnnounceTime=event->announceTime;
			}
		}
	}
	int erg;

	if(nextAnnounceTime!=0)
	{
		int minutes=((nextAnnounceTime-time(NULL))/60)-3; //Wakeup 3 min befor next announce
		if(minutes<1)
			minutes=1; //1 minute is minimum

		int fd = open("/dev/dbox/fp0", O_RDWR);
		if((erg=ioctl(fd, FP_IOCTL_SET_WAKEUP_TIMER, &minutes))<0)
		{
			if(erg==-1)	// Wakeup not supported
			{
				dprintf("Wakeup not supported (%d min.)\n",minutes);
			}
			else
			{
				dprintf("Error setting wakeup (%d)\n",erg);
			}
		}
		else
		{
			dprintf("wakeup in %d min. programmed\n",minutes);
			status=true;
		}
	}
	return status;
}
//------------------------------------------------------------
void CTimerManager::shutdownOnWakeup()
{
	time_t nextAnnounceTime=0;
	CTimerEventMap::iterator pos = events.begin();
	for(;pos != events.end();pos++)
	{
		CTimerEvent *event = pos->second;
		if((event->eventType == CTimerd::TIMER_RECORD ||
			 event->eventType == CTimerd::TIMER_ZAPTO ) &&
			event->eventState == CTimerd::TIMERSTATE_SCHEDULED)
		{
			// Wir wachen nur für Records und Zaptos wieder auf
			if(event->announceTime < nextAnnounceTime || nextAnnounceTime==0)
			{
				nextAnnounceTime=event->announceTime;
			}
		}
	}
	time_t now = time(NULL);
	if((nextAnnounceTime-now) > 600 || nextAnnounceTime==0)
	{ // in den naechsten 10 min steht nix an
		//teste auf wakeup
		char wakeup;
		int fd = open("/dev/dbox/fp0", O_RDWR);
		int ret=ioctl(fd, FP_IOCTL_IS_WAKEUP, &wakeup);
		if(wakeup!=0 && !(ret<0))
		{
			dprintf("Programming shutdown event\n");
			CTimerEvent_Shutdown* event = new CTimerEvent_Shutdown(now+120, now+180);
			addEvent(event);
		}
		close(fd);
	}
}
void CTimerManager::setRecordingSafety(int pre, int post)
{
	m_extraTimeStart=pre;
	m_extraTimeEnd=post;
   m_saveEvents=true; // also saves extra times
}
//------------------------------------------------------------
//=============================================================
// event functions
//=============================================================
//------------------------------------------------------------
CTimerEvent::CTimerEvent( CTimerd::CTimerEventTypes evtype, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerd::CTimerEventRepeat evrepeat)
{
	eventRepeat = evrepeat;
	eventState = CTimerd::TIMERSTATE_SCHEDULED; 
	eventType = evtype;
	announceTime = announcetime;
	alarmTime = alarmtime;
	stopTime = stoptime;
}

//------------------------------------------------------------
CTimerEvent::CTimerEvent( CTimerd::CTimerEventTypes evtype, int mon, int day, int hour, int min, CTimerd::CTimerEventRepeat evrepeat)
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
CTimerEvent::CTimerEvent(CTimerd::CTimerEventTypes evtype,CConfigFile *config, int iId)
{
	std::stringstream ostr;
	ostr << iId;
	std::string id=ostr.str();
	time_t announcetime=config->getInt32("ANNOUNCE_TIME_"+id);
	time_t alarmtime=config->getInt32("ALARM_TIME_"+id);
	time_t stoptime=config->getInt32("STOP_TIME_"+id);
	CTimerd::CTimerEventRepeat evrepeat=(CTimerd::CTimerEventRepeat)config->getInt32("EVENT_REPEAT_"+id);
	eventRepeat = evrepeat;
	eventState = CTimerd::TIMERSTATE_SCHEDULED; 
	eventType = evtype;
	announceTime = announcetime;
	alarmTime = alarmtime;
	stopTime = stoptime;
	eventState = (CTimerd::CTimerEventStates ) config->getInt32 ("EVENT_STATE_"+id);
	previousState = (CTimerd::CTimerEventStates) config->getInt32("PREVIOUS_STATE_"+id);
}
//------------------------------------------------------------
void CTimerEvent::Reschedule()
{
	if(eventRepeat == CTimerd::TIMERREPEAT_ONCE)
	{
		eventState = CTimerd::TIMERSTATE_TERMINATED;
		dprintf("event %d not rescheduled, event will be terminated\n",eventID);
	}
	else
	{
		time_t now = time(NULL);
		while(alarmTime < now)
		{
			time_t diff = 0;
			struct tm *t= localtime(&alarmTime);
			int isdst1=t->tm_isdst;
			switch(eventRepeat)
			{
				case CTimerd::TIMERREPEAT_ONCE :
					break;
				case CTimerd::TIMERREPEAT_DAILY: 
					t->tm_mday++;
				break;
				case CTimerd::TIMERREPEAT_WEEKLY: 
					t->tm_mday+=7;
				break;
				case CTimerd::TIMERREPEAT_BIWEEKLY: 
					t->tm_mday+=14;
				break;
				case CTimerd::TIMERREPEAT_FOURWEEKLY: 
					t->tm_mday+=28;
				break;
				case CTimerd::TIMERREPEAT_MONTHLY: 
					t->tm_mon++;
				break;
				case CTimerd::TIMERREPEAT_BYEVENTDESCRIPTION :
					// todo !!
					break;
				default:
					if(eventRepeat >= CTimerd::TIMERREPEAT_WEEKDAYS)
					{
						int weekdays = ((int)eventRepeat) >> 9;
						if(weekdays > 0)
						{
							bool weekday_arr[7];
							weekday_arr[0]=((weekdays & 0x40) > 0); //So
							weekday_arr[1]=((weekdays & 0x1) > 0); //Mo
							weekday_arr[2]=((weekdays & 0x2) > 0); //Di
							weekday_arr[3]=((weekdays & 0x4) > 0); //Mi
							weekday_arr[4]=((weekdays & 0x8) > 0); //Do
							weekday_arr[5]=((weekdays & 0x10) > 0); //Fr
							weekday_arr[6]=((weekdays & 0x20) > 0); //Sa
							struct tm *t= localtime(&alarmTime);
							int day;
							for(day=1 ; !weekday_arr[(t->tm_wday+day)%7] ; day++){}
							t->tm_mday+=day;
						}
					}
					else
						dprintf("unknown repeat type %d\n",eventRepeat);
			}
			diff = mktime(t)-alarmTime;
			alarmTime += diff;
			t = localtime(&alarmTime);
			int isdst2 = t->tm_isdst;
			if(isdst2 > isdst1) //change from winter to summer
			{
				diff-=3600;
				alarmTime-=3600;
			}
			else if(isdst1 > isdst2) //change from summer to winter
			{
				diff+=3600;
				alarmTime+=3600;
			}
			if(announceTime > 0)
				announceTime += diff;
			if(stopTime > 0)
				stopTime += diff;
		}
		eventState = CTimerd::TIMERSTATE_SCHEDULED;
		dprintf("event %d rescheduled\n",eventID);
	}
}


//------------------------------------------------------------
void CTimerEvent::printEvent(void)
{
	struct tm *alarmtime, *announcetime;
	dprintf("eventID: %03d type: %d state: %d repeat: %d ",eventID,eventType,eventState,((int)eventRepeat)&0x1FF);
	announcetime = localtime(&announceTime);
	dprintf("announce: %u %02d.%02d. %02d:%02d:%02d ",(uint) announceTime,announcetime->tm_mday,announcetime->tm_mon+1,announcetime->tm_hour,announcetime->tm_min,announcetime->tm_sec);
	alarmtime = localtime(&alarmTime);
	dprintf("alarm: %u %02d.%02d. %02d:%02d:%02d ",(uint) alarmTime,alarmtime->tm_mday,alarmtime->tm_mon+1,alarmtime->tm_hour,alarmtime->tm_min,alarmtime->tm_sec);
	switch(eventType)
	{
		case CTimerd::TIMER_ZAPTO :
			dprintf("Zapto: %x epg: %llx\n",static_cast<CTimerEvent_Zapto*>(this)->eventInfo.channel_id,static_cast<CTimerEvent_Zapto*>(this)->eventInfo.epgID);
			break;

		case CTimerd::TIMER_RECORD :
			dprintf("Record: %x epg: %llx apids: %s\n",static_cast<CTimerEvent_Record*>(this)->eventInfo.channel_id,static_cast<CTimerEvent_Record*>(this)->eventInfo.epgID,
					  static_cast<CTimerEvent_Record*>(this)->eventInfo.apids.c_str());
			break;

		case CTimerd::TIMER_STANDBY :
			dprintf("standby: %s\n",(static_cast<CTimerEvent_Standby*>(this)->standby_on == 1)?"on":"off");
			break;

		default:
			dprintf("(no extra data)\n");
	}
}
//------------------------------------------------------------
void CTimerEvent::saveToConfig(CConfigFile *config)
{
	std::vector<int> allIDs;
	allIDs.clear();
	if (!(config->getString("IDS").empty()))
	{
		// sonst bekommen wir den bloeden 0er
		allIDs=config->getInt32Vector("IDS");
	}

	allIDs.push_back(eventID);
	//SetInt-Vector haengt komischerweise nur an, deswegen erst loeschen
	config->setString("IDS","");
	config->setInt32Vector ("IDS",allIDs);

	std::stringstream ostr;
	ostr << eventID;
	std::string id=ostr.str();
	config->setInt32("EVENT_TYPE_"+id, eventType);
	config->setInt32("EVENT_STATE_"+id, eventState);
	config->setInt32("PREVIOUS_STATE_"+id, previousState);
	config->setInt32("EVENT_REPEAT_"+id, eventRepeat);
	config->setInt32("ANNOUNCE_TIME_"+id, announceTime);
	config->setInt32("ALARM_TIME_"+id, alarmTime);
	config->setInt32("STOP_TIME_"+id, stopTime);

}

//=============================================================
// Shutdown Event
//=============================================================
void CTimerEvent_Shutdown::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_ANNOUNCE_SHUTDOWN,
								  CEventServer::INITID_TIMERD);
}
//------------------------------------------------------------
void CTimerEvent_Shutdown::fireEvent()
{
	dprintf("Shutdown Timer fired\n");
	//event in neutrinos remoteq. schreiben
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_SHUTDOWN,
								  CEventServer::INITID_TIMERD);
}
//=============================================================
// Sleeptimer Event
//=============================================================
void CTimerEvent_Sleeptimer::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER,
								  CEventServer::INITID_TIMERD);
}
//------------------------------------------------------------
void CTimerEvent_Sleeptimer::fireEvent()
{
	dprintf("Sleeptimer Timer fired\n");
	//event in neutrinos remoteq. schreiben
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_SLEEPTIMER,
								  CEventServer::INITID_TIMERD);
}
//=============================================================
// Standby Event
//=============================================================
CTimerEvent_Standby::CTimerEvent_Standby( time_t announceTime, time_t alarmTime, 
														bool sb_on, 
														CTimerd::CTimerEventRepeat evrepeat): 
CTimerEvent(CTimerd::TIMER_STANDBY, announceTime, alarmTime, (time_t) 0, evrepeat)
{
	standby_on = sb_on;
}
CTimerEvent_Standby::CTimerEvent_Standby(CConfigFile *config, int iId):
CTimerEvent(CTimerd::TIMER_STANDBY, config, iId)
{
	std::stringstream ostr;
	ostr << iId;
	std::string id=ostr.str();
	standby_on = config->getBool("STANDBY_ON_"+id);
}
//------------------------------------------------------------
void CTimerEvent_Standby::fireEvent()
{
	dprintf("Standby Timer fired: %s\n",standby_on?"on":"off");
	CTimerManager::getInstance()->getEventServer()->sendEvent(
		(standby_on)?CTimerdClient::EVT_STANDBY_ON:CTimerdClient::EVT_STANDBY_OFF,
		CEventServer::INITID_TIMERD);
}

//------------------------------------------------------------
void CTimerEvent_Standby::saveToConfig(CConfigFile *config)
{
	CTimerEvent::saveToConfig(config);
	std::stringstream ostr;
	ostr << eventID;
	std::string id=ostr.str();
	config->setBool("STANDBY_ON_"+id,standby_on);
}
//=============================================================
// Record Event
//=============================================================
CTimerEvent_Record::CTimerEvent_Record(time_t announceTime, time_t alarmTime, time_t stopTime, 
				       t_channel_id channel_id,
				       event_id_t epgID, 
				       time_t epg_starttime, const std::string apids, CTimerd::CChannelMode mode,
				       CTimerd::CTimerEventRepeat evrepeat) :
CTimerEvent(getEventType(), announceTime, alarmTime, stopTime, evrepeat)
{
	eventInfo.epgID = epgID;
	eventInfo.epg_starttime = epg_starttime;
	eventInfo.channel_id = channel_id;
	eventInfo.apids = apids;
	eventInfo.mode = mode;
}
//------------------------------------------------------------
CTimerEvent_Record::CTimerEvent_Record(CConfigFile *config, int iId):
	CTimerEvent(getEventType(), config, iId)
{
	std::stringstream ostr;
	ostr << iId;
	std::string id=ostr.str();
	eventInfo.epgID = config->getInt64("EVENT_INFO_EPG_ID_"+id);
	eventInfo.epg_starttime = config->getInt64("EVENT_INFO_EPG_STARTTIME_"+id);
	eventInfo.channel_id = config->getInt32("EVENT_INFO_ONID_SID_"+id);
	eventInfo.apids = config->getString("EVENT_INFO_APIDS_"+id);
	eventInfo.mode = (CTimerd::CChannelMode) config->getInt32("EVENT_INFO_CHANNEL_MODE_"+id);
}
//------------------------------------------------------------
void CTimerEvent_Record::fireEvent()
{
	CTimerd::RecordingInfo ri=eventInfo;
	ri.eventID=eventID;
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_RECORD_START,
								  CEventServer::INITID_TIMERD,
								  &ri,
								  sizeof(CTimerd::RecordingInfo));
	dprintf("Record Timer fired\n"); 
}
//------------------------------------------------------------
void CTimerEvent_Record::announceEvent()
{
	Refresh();
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_ANNOUNCE_RECORD, CEventServer::INITID_TIMERD);
	dprintf("Record announcement\n"); 
}
//------------------------------------------------------------
void CTimerEvent_Record::stopEvent()
{
	CTimerd::RecordingStopInfo stopinfo;
	// Set EPG-ID if not set
	stopinfo.eventID = eventID;
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_RECORD_STOP,
								  CEventServer::INITID_TIMERD,
								  &stopinfo,
								  sizeof(CTimerd::RecordingStopInfo));
	// Programmiere shutdown timer, wenn in wakeup state und kein record/zapto timer in 10 min
	CTimerManager::getInstance()->shutdownOnWakeup();
	dprintf("Recording stopped\n"); 
}
//------------------------------------------------------------
void CTimerEvent_Record::saveToConfig(CConfigFile *config)
{
	CTimerEvent::saveToConfig(config);
	std::stringstream ostr;
	ostr << eventID;
	std::string id=ostr.str();
	config->setInt64("EVENT_INFO_EPG_ID_"+id, eventInfo.epgID);
	config->setInt64("EVENT_INFO_EPG_STARTTIME_"+id, eventInfo.epg_starttime);
	config->setInt32("EVENT_INFO_ONID_SID_"+id, eventInfo.channel_id);
	config->setInt32("EVENT_INFO_CHANNEL_MODE_"+id, (int) eventInfo.mode);
	config->setString("EVENT_INFO_APIDS_"+id, eventInfo.apids);
}
//------------------------------------------------------------
void CTimerEvent_Record::Reschedule()
{
	// clear epgId on reschedule
	eventInfo.epgID = 0;
	eventInfo.epg_starttime = 0;
	CTimerEvent::Reschedule();
}
//------------------------------------------------------------
void CTimerEvent_Record::getEpgId()
{
	CSectionsdClient sdc;
	CChannelEventList evtlist = sdc.getEventsServiceKey(eventInfo.channel_id);
	// we check for a time in the middle of the recording
	time_t check_time=alarmTime/2 + stopTime/2;
	for ( CChannelEventList::iterator e= evtlist.begin(); e != evtlist.end(); ++e )
	{
	    	if ( e->startTime <= check_time && (e->startTime + (int)e->duration) >= check_time)
		{
			eventInfo.epgID = e->eventID;
			eventInfo.epg_starttime = e->startTime;
			break;
		}
	}
}
//------------------------------------------------------------
void CTimerEvent_Record::Refresh()
{
	if (eventInfo.epgID == 0)
		getEpgId();
}
//=============================================================
// Zapto Event
//=============================================================
void CTimerEvent_Zapto::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_ANNOUNCE_ZAPTO,
								  CEventServer::INITID_TIMERD);
}
//------------------------------------------------------------
void CTimerEvent_Zapto::fireEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_ZAPTO,
								  CEventServer::INITID_TIMERD,
								  &eventInfo,
								  sizeof(CTimerd::EventInfo));
}


//------------------------------------------------------------
void CTimerEvent_Zapto::getEpgId()
{
	CSectionsdClient sdc;
	CChannelEventList evtlist = sdc.getEventsServiceKey(eventInfo.channel_id);
	// we check for a time 5 min after zap
	time_t check_time=alarmTime + 300;
	for ( CChannelEventList::iterator e= evtlist.begin(); e != evtlist.end(); ++e )
	{
    	if ( e->startTime < check_time && (e->startTime + (int)e->duration) > check_time)
		{
			eventInfo.epgID = e->eventID;
			eventInfo.epg_starttime = e->startTime;
			break;
		}
	}
}
//=============================================================
// NextProgram Event
//=============================================================
CTimerEvent_NextProgram::CTimerEvent_NextProgram(time_t announceTime, time_t alarmTime, time_t stopTime, 
						 t_channel_id channel_id,
						 event_id_t epgID, 
						 time_t epg_starttime, CTimerd::CTimerEventRepeat evrepeat) :
CTimerEvent(CTimerd::TIMER_NEXTPROGRAM, announceTime, alarmTime, stopTime, evrepeat)
{
	eventInfo.epgID = epgID;
	eventInfo.epg_starttime = epg_starttime;
	eventInfo.channel_id = channel_id;
}
//------------------------------------------------------------
CTimerEvent_NextProgram::CTimerEvent_NextProgram(CConfigFile *config, int iId):
CTimerEvent(CTimerd::TIMER_NEXTPROGRAM, config, iId)
{
	std::stringstream ostr;
	ostr << iId;
	std::string id=ostr.str();
	eventInfo.epgID = config->getInt64("EVENT_INFO_EPG_ID_"+id);
	eventInfo.epg_starttime = config->getInt64("EVENT_INFO_EPG_STARTTIME_"+id);
	eventInfo.channel_id = config->getInt32("EVENT_INFO_ONID_SID_"+id);
	eventInfo.apids = config->getString("EVENT_INFO_APIDS_"+id);
	eventInfo.mode = (CTimerd::CChannelMode) config->getInt32("EVENT_INFO_CHANNEL_MODE_"+id);
}
//------------------------------------------------------------

void CTimerEvent_NextProgram::announceEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM,
								  CEventServer::INITID_TIMERD,
								  &eventInfo,
								  sizeof(eventInfo));
}
//------------------------------------------------------------
void CTimerEvent_NextProgram::fireEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(CTimerdClient::EVT_NEXTPROGRAM,
								  CEventServer::INITID_TIMERD,
								  &eventInfo,
								  sizeof(eventInfo));
}

//------------------------------------------------------------
void CTimerEvent_NextProgram::saveToConfig(CConfigFile *config)
{
	CTimerEvent::saveToConfig(config);
	std::stringstream ostr;
	ostr << eventID;
	std::string id=ostr.str();
	config->setInt64("EVENT_INFO_EPG_ID_"+id,eventInfo.epgID);
	config->setInt64("EVENT_INFO_EPG_STARTTIME_"+id,eventInfo.epg_starttime);
	config->setInt32("EVENT_INFO_ONID_SID_"+id,eventInfo.channel_id);
	config->setString("EVENT_INFO_APIDS_"+id,eventInfo.apids);
	config->setInt32("EVENT_INFO_CHANNEL_MODE_"+id, (int) eventInfo.mode);
}
//------------------------------------------------------------
void CTimerEvent_NextProgram::Reschedule()
{
	// clear eogId on reschedule
	eventInfo.epgID = 0;
	eventInfo.epg_starttime = 0;
	CTimerEvent::Reschedule();
}
//=============================================================
// Remind Event
//=============================================================
CTimerEvent_Remind::CTimerEvent_Remind(time_t announceTime,
				       time_t alarmTime, 
				       const char * const msg,
				       CTimerd::CTimerEventRepeat evrepeat) :
CTimerEvent(CTimerd::TIMER_REMIND, announceTime, alarmTime, (time_t) 0, evrepeat)
{
	memset(message, 0, sizeof(message));
	strncpy(message, msg, sizeof(message)-1);
}
//------------------------------------------------------------
CTimerEvent_Remind::CTimerEvent_Remind(CConfigFile *config, int iId):
CTimerEvent(CTimerd::TIMER_REMIND, config, iId)
{
	std::stringstream ostr;
	ostr << iId;
	std::string id=ostr.str();
	strcpy(message, config->getString("MESSAGE_"+id).c_str());
}
//------------------------------------------------------------
void CTimerEvent_Remind::fireEvent()
{
	CTimerManager::getInstance()->getEventServer()->sendEvent(
																				CTimerdClient::EVT_REMIND,
																				CEventServer::INITID_TIMERD,
																				message,REMINDER_MESSAGE_MAXLEN);
}

//------------------------------------------------------------
void CTimerEvent_Remind::saveToConfig(CConfigFile *config)
{
	CTimerEvent::saveToConfig(config);
	std::stringstream ostr;
	ostr << eventID;
	std::string id=ostr.str();
	config->setString("MESSAGE_"+id,message);
}
//=============================================================
