/*
	Timer-Daemon  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	$Id: timerd.cpp,v 1.9 2002/05/17 19:50:41 dirch Exp $

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

	CTimerEvent_NextProgram::EventMap::iterator it = NULL;
	CTimerEventMap events;
	switch (rmessage->cmd)
	{

		case CTimerd::CMD_REGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->registerEvent( connfd );
		break;

		case CTimerd::CMD_UNREGISTEREVENT :
			CTimerManager::getInstance()->getEventServer()->unRegisterEvent( connfd );
		break;

		case CTimerd::CMD_GETTIMER:
			CTimerd::commandGetTimer msgGetTimer;
			read(connfd,&msgGetTimer, sizeof(msgGetTimer));
			CTimerd::responseGetTimer rspGetTimer;

		break;

		case CTimerd::CMD_GETTIMERLIST:
			CTimerManager::getInstance()->listEvents(events);
			for(CTimerEventMap::iterator pos = events.begin();pos != events.end();pos++)
			{
				CTimerd::responseGetTimer event;
				event.month = pos->second->alarmtime.tm_mon;
				event.day = pos->second->alarmtime.tm_mday;
				event.hour = pos->second->alarmtime.tm_hour;
				event.min = pos->second->alarmtime.tm_min;
				event.eventType = pos->second->eventType;
				event.eventID = pos->second->eventID;
				write( connfd, &event, sizeof(CTimerd::responseGetTimer));
			}
		break;

		case CTimerd::CMD_ADDTIMER:
			CTimerd::commandAddTimer msgAddTimer;
			read(connfd,&msgAddTimer, sizeof(msgAddTimer));

			CTimerd::responseAddTimer rspAddTimer;
			CTimerEvent* event;
			CTimerd::EventInfo evInfo;
			switch (msgAddTimer.evType)
			{

				case CTimerdClient::TIMER_STANDBY :
					CTimerd::commandSetStandby standby;
					read( connfd, &standby, sizeof(CTimerd::commandSetStandby));
					event = new CTimerEvent_Standby(
						msgAddTimer.month, msgAddTimer.day,
						msgAddTimer.hour, msgAddTimer.min);

					static_cast<CTimerEvent_Standby*>(event)->standby_on = standby.standby_on;
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
				break;

				case CTimerdClient::TIMER_SHUTDOWN :
					event = new CTimerEvent_Shutdown(
						msgAddTimer.month, msgAddTimer.day,
						msgAddTimer.hour, msgAddTimer.min);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
				break;

				case CTimerdClient::TIMER_RECORD :
					event = new CTimerEvent_Record(
						msgAddTimer.month, msgAddTimer.day,
						msgAddTimer.hour, msgAddTimer.min);
					rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
				break;

				case CTimerdClient::TIMER_ZAPTO :
					read( connfd, &evInfo, sizeof(CTimerd::EventInfo));
					if(evInfo.onidSid > 0)
					{
						event = new CTimerEvent_Zapto(
							msgAddTimer.month, msgAddTimer.day,
							msgAddTimer.hour, msgAddTimer.min);
						static_cast<CTimerEvent_NextProgram*>(event)->eventInfo.onidSid = evInfo.onidSid;
						rspAddTimer.eventID = CTimerManager::getInstance()->addEvent( event);
					}
				break;

				case CTimerdClient::TIMER_NEXTPROGRAM :
//					CTimerd::EventInfo evInfo;
					read( connfd, &evInfo, sizeof(CTimerd::EventInfo));

					it = CTimerEvent_NextProgram::events.find( evInfo.uniqueKey);
					if (it == CTimerEvent_NextProgram::events.end())
					{
						event = new CTimerEvent_NextProgram(
							msgAddTimer.month, msgAddTimer.day,
							msgAddTimer.hour, msgAddTimer.min);
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

				break;
				default:
					event = new CTimerEvent(
						msgAddTimer.month, msgAddTimer.day,
						msgAddTimer.hour, msgAddTimer.min,
						msgAddTimer.evType);
					rspAddTimer.eventID = event->eventID;
			}

			write( connfd, &rspAddTimer, sizeof(rspAddTimer));

			break;
		case CTimerd::CMD_REMOVETIMER:
			CTimerd::commandRemoveTimer msgRemoveTimer;
			read(connfd,&msgRemoveTimer, sizeof(msgRemoveTimer));
			CTimerManager::getInstance()->removeEvent( msgRemoveTimer.eventID);
			break;

		case CTimerd::CMD_TIMERDAVAILABLE:
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

			if (strncmp(argv[i], "-d", 2) == 0)
			{
				debug = 1;
			}
			else if (strncmp(argv[i], "-f", 2) == 0)
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
		while(1)
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
