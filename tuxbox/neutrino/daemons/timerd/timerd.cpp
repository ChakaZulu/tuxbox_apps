/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerd.cpp,v 1.12 2002/05/31 20:27:38 dirch Exp $

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

//#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include "timermanager.h"
#include "timerdMsg.h"
#include "debug.h"

void parse_command(int connfd, CTimerd::commandHead* rmessage)
{

	if(rmessage->version!=CTimerd::ACTVERSION)
	{
		dperror("command with unknown version\n");
		return;
	}

//	CTimerEvent_NextProgram::EventMap::iterator it = NULL;
	CTimerEventMap events;
	CTimerd::commandModifyTimer msgModifyTimer;
	CTimerd::responseGetSleeptimer rspGetSleeptimer;
	CTimerEventMap::iterator pos;
	switch (rmessage->cmd)
	{

		case CTimerd::CMD_REGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->registerEvent( connfd );
		break;

		case CTimerd::CMD_UNREGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->unRegisterEvent( connfd );
		break;

		case CTimerd::CMD_GETSLEEPTIMER:
			printf("CMD_SLEEPTIMER\n");
			rspGetSleeptimer.eventID = 0;
			printf("anzahl events: %d\n",events.size());
			if(CTimerManager::getInstance()->listEvents(events))
			{
				if(events.size() > 0)
				{
					for(pos = events.begin();(pos != events.end()) && (pos->second->eventType != CTimerEvent::TIMER_SLEEPTIMER) ;pos++)
						printf("ID: %u type: %u\n",pos->second->eventID,pos->second->eventType);
					if(pos->second->eventType == CTimerEvent::TIMER_SLEEPTIMER)
						rspGetSleeptimer.eventID = pos->second->eventID;
				}
			}
			write( connfd, &rspGetSleeptimer, sizeof(rspGetSleeptimer));
		break;

		case CTimerd::CMD_GETTIMER:						// timer daten abfragen
			CTimerd::commandGetTimer msgGetTimer;
			CTimerd::responseGetTimer resp;
			read(connfd,&msgGetTimer, sizeof(msgGetTimer));
			if(CTimerManager::getInstance()->listEvents(events))
			{
				if(events[msgGetTimer.eventID])
				{
					CTimerEvent *event = events[msgGetTimer.eventID];
					resp.eventID = event->eventID;
					resp.eventState = event->eventState;
					resp.eventType = event->eventType;
					resp.eventRepeat = event->eventRepeat;
					resp.announceTime = event->announceTime;
					resp.alarmTime = event->alarmTime;
					resp.stopTime = event->stopTime;
				}
			}
			write( connfd, &resp, sizeof(CTimerd::responseGetTimer));
		break;

		case CTimerd::CMD_GETTIMERLIST:				// liste aller timer 
			if(CTimerManager::getInstance()->listEvents(events))
			{
				for(CTimerEventMap::iterator pos = events.begin();pos != events.end();pos++)
				{
					CTimerd::responseGetTimer resp;
					CTimerEvent *event = pos->second;

					resp.eventID = event->eventID;
					resp.eventState = event->eventState;
					resp.eventType = event->eventType;
					resp.eventRepeat = event->eventRepeat;
					resp.announceTime = event->announceTime;
					resp.alarmTime = event->alarmTime;
					resp.stopTime = event->stopTime;
					write( connfd, &resp, sizeof(CTimerd::responseGetTimer));
				}
			}
		break;

		case CTimerd::CMD_RESCHEDULETIMER:			// event nach vorne oder hinten schieben
			read(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
			CTimerManager::getInstance()->rescheduleEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime, msgModifyTimer.stopTime);
		break;

		case CTimerd::CMD_MODIFYTIMER:				// neue zeiten setzen
			read(connfd,&msgModifyTimer, sizeof(msgModifyTimer));
			CTimerManager::getInstance()->modifyEvent(msgModifyTimer.eventID,msgModifyTimer.announceTime,msgModifyTimer.alarmTime, msgModifyTimer.stopTime);
		break;

		case CTimerd::CMD_ADDTIMER:						// neuen timer hinzufügen
			CTimerd::commandAddTimer msgAddTimer;
			read(connfd,&msgAddTimer, sizeof(msgAddTimer));

			CTimerd::responseAddTimer rspAddTimer;
			CTimerEvent* event;
			CTimerEvent::EventInfo evInfo;
			switch (msgAddTimer.eventType)
			{
				case CTimerEvent::TIMER_STANDBY :
					CTimerd::commandSetStandby standby;
					read( connfd, &standby, sizeof(CTimerd::commandSetStandby));

					event = new CTimerEvent_Standby(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.stopTime,
						msgAddTimer.eventRepeat);

					static_cast<CTimerEvent_Standby*>(event)->standby_on = standby.standby_on;
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
				break;

				case CTimerEvent::TIMER_SHUTDOWN :
					event = new CTimerEvent_Shutdown(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.stopTime,
						msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
				break;

				case CTimerEvent::TIMER_SLEEPTIMER :
					event = new CTimerEvent_Sleeptimer(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.stopTime,
						msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
				break;

				case CTimerEvent::TIMER_RECORD :
					event = new CTimerEvent_Record(
						msgAddTimer.announceTime,
						msgAddTimer.alarmTime,
						msgAddTimer.stopTime,
						msgAddTimer.eventRepeat);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
				break;

				case CTimerEvent::TIMER_ZAPTO :
					read( connfd, &evInfo, sizeof(CTimerEvent::EventInfo));
					if(evInfo.onidSid > 0)
					{
						event = new CTimerEvent_Zapto(
							msgAddTimer.announceTime,
							msgAddTimer.alarmTime,
							msgAddTimer.stopTime,
							msgAddTimer.eventRepeat);
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.onidSid = evInfo.onidSid;
						rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					}
				break;

				case CTimerEvent::TIMER_NEXTPROGRAM :
//					CTimerd::EventInfo evInfo;
					read( connfd, &evInfo, sizeof(CTimerEvent::EventInfo));
/*
					it = CTimerEvent_NextProgram::events.find( evInfo.uniqueKey);
					if (it == CTimerEvent_NextProgram::events.end())
					{
						event = new CTimerEvent_NextProgram(
							msgAddTimer.announceTime,
							msgAddTimer.alarmTime,
							msgAddTimer.stopTime,
							msgAddTimer.eventRepeat);
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo = evInfo;
						CTimerEvent_NextProgram::events.insert(make_pair(static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.uniqueKey, static_cast<CTimerEvent_NextProgram*>(event)));
						rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					}
					else
					{
						event = it->second;
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo = evInfo;
						event->alarmtime.tm_mon  = msgAddTimer.month;
						event->alarmtime.tm_mday = msgAddTimer.day;
						event->alarmtime.tm_hour = msgAddTimer.hour;
						event->alarmtime.tm_min  = msgAddTimer.min;
						rspAddTimer.eventID = event->eventID;
					}
*/
				break;
				default:
					printf("[timerd] Unknown TimerType\n");
			}

			write( connfd, &rspAddTimer, sizeof(rspAddTimer));

			break;
		case CTimerd::CMD_REMOVETIMER:						//	timer entfernen
			CTimerd::commandRemoveTimer msgRemoveTimer;
			read(connfd,&msgRemoveTimer, sizeof(msgRemoveTimer));
			CTimerManager::getInstance()->removeEvent( msgRemoveTimer.eventID);
			break;

		case CTimerd::CMD_TIMERDAVAILABLE:					// testen ob server läuft ;)
			CTimerd::responseAvailable rspAvailable;
			rspAvailable.available = true;
			write( connfd, &rspAvailable, sizeof(rspAvailable));
			break;
		default:
			dprintf("unknown command\n");
	}
}

int main(int argc, char **argv)
{
	int listenfd, connfd;
	struct sockaddr_un servaddr;
	int clilen;
	bool do_fork = true;

	dprintf("startup\n\n");
	if (argc > 1)
	{
		for(int i = 1; i < argc; i++)
		{

			if (strncmp(argv[i], "-f", 2) == 0)
			{
				do_fork = false;
			}
		}
	}

	if(do_fork)
	{
		switch (fork())
		{
		case -1:
			perror("[timerd] fork");
			return -1;
		case 0:
			break;
		default:
			return 0;
		}
		if (setsid() == -1)
		{
			perror("[timerd] setsid");
			return -1;
		}
	}

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, TIMERD_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	unlink(TIMERD_UDS_NAME);

	//network-setup
	if ((listenfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		dperror("error while socket create");
	}

	if ( bind(listenfd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
		dperror("bind failed...");
		exit(-1);
	}

	if (listen(listenfd, 5) !=0)
	{
		perror("listen failed...");
		exit( -1 );
	}

	//startup Timer
	try
	{
		struct CTimerd::commandHead rmessage;
		while(1)								// wait for incomming messages
		{
			connfd = accept(listenfd, (struct sockaddr*) &servaddr, (socklen_t*) &clilen);
			memset(&rmessage, 0, sizeof(rmessage));
			read(connfd,&rmessage,sizeof(rmessage));
			parse_command(connfd, &rmessage);
			close(connfd);
		}
	}
	catch (std::exception& e)
	{
		dprintf("caught std-exception in main-thread %s!\n", e.what());
	}
	catch (...)
	{
		dprintf("caught exception in main-thread!\n");
	}
}
