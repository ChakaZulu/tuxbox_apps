/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/
	
	$Id: timerdclient.cpp,v 1.9 2002/05/21 13:07:01 dirch Exp $

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

#include "timerdclient.h"
#include "timerdMsg.h"
#include "../timermanager.h"
#include "debug.h"


CTimerdClient::CTimerdClient()
{
	sock_fd = 0;
}

bool CTimerdClient::timerd_connect()
{
	timerd_close();

	struct sockaddr_un servaddr;
	int clilen;
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, TIMERD_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("timerdclient: socket");
		return false;
	}

	if(connect(sock_fd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
  		perror("timerdclient: connect");
		return false;
	}
	return true;
}

bool CTimerdClient::timerd_close()
{
	if(sock_fd!=0)
	{
		close(sock_fd);
		sock_fd=0;
	}
}

bool CTimerdClient::send(char* data, int size)
{
	return( write(sock_fd, data, size) == size);
}

bool CTimerdClient::receive(char* data, int size)
{
	return( read(sock_fd, data, size) == size);
}

void CTimerdClient::registerEvent(unsigned int eventID, unsigned int clientID, string udsName)
{
	CTimerd::commandHead msg;
	CEventServer::commandRegisterEvent msg2;

	msg.version = CTimerd::ACTVERSION;
	msg.cmd = CTimerd::CMD_REGISTEREVENT;

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	strcpy(msg2.udsName, udsName.c_str());
	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	timerd_close();
}

void CTimerdClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	CTimerd::commandHead msg;
	CEventServer::commandUnRegisterEvent msg2;

	msg.version = CTimerd::ACTVERSION;
	msg.cmd = CTimerd::CMD_UNREGISTEREVENT;

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	timerd_close();
}

int CTimerdClient::setSleeptimer(time_t announcetime, time_t alarmtime, int timerid)
{
int timerID;

	if(timerid == 0)
		timerID = getSleeptimerID();
	else
		timerID = timerid;

	if(timerID != 0)
		modifyTimerEvent(timerID, announcetime, alarmtime, 0);
	else
		timerID = addTimerEvent(CTimerEvent::TIMER_SLEEPTIMER,true,NULL,announcetime,alarmtime,0);

	return timerID;	
}

int CTimerdClient::getSleeptimerID()
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_GETSLEEPTIMER;
	timerd_connect();
	send((char*)&msg, sizeof(msg));
	CTimerd::responseGetSleeptimer response;
	receive((char*)&response, sizeof(CTimerd::responseGetSleeptimer));
	dprintf("sleeptimer ID : %d\n",response.eventID);
	return response.eventID;
}

int CTimerdClient::getSleepTimerRemaining()
{
	int timerID;
	if((timerID = getSleeptimerID()) != 0)
	{
		CTimerd::responseGetTimer timer;
		getTimer( timer, timerID);
		dprintf("Remaining sleeptimer ID: %d\n",timerID);
		return (timer.alarmTime - time(NULL)) / 60;
	}
	else
		return -1;
}

void CTimerdClient::getTimerList( CTimerd::TimerList &timerlist)
{
	dprintf("getTimerList\n");
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_GETTIMERLIST;

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	timerlist.clear();
	CTimerd::responseGetTimer response;
	while ( receive((char*)&response, sizeof(CTimerd::responseGetTimer)))
	{
		dprintf("received one event\n");
		timerlist.insert( timerlist.end(), response);
	}
	timerd_close();
}

void CTimerdClient::getTimer( CTimerd::responseGetTimer &timer, unsigned timerID)
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_GETTIMER;

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&timerID, sizeof(timerID));

	CTimerd::responseGetTimer response;
	receive((char*)&response, sizeof(CTimerd::responseGetTimer));
	timer = response;
	timerd_close();
}


bool CTimerdClient::modifyTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime)
{
	// set new time values for event eventid
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_MODIFYTIMER;

	CTimerd::commandModifyTimer msgModifyTimer;
	msgModifyTimer.eventID = eventid;
	msgModifyTimer.announceTime = announcetime;
	msgModifyTimer.alarmTime = alarmtime;
	msgModifyTimer.stopTime = stoptime;

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*) &msgModifyTimer, sizeof(msgModifyTimer));

	timerd_close();

}

bool CTimerdClient::rescheduleTimerEvent(int eventid, time_t diff)
{
	rescheduleTimerEvent(eventid,diff,diff,diff);
}

bool CTimerdClient::rescheduleTimerEvent(int eventid, time_t announcediff, time_t alarmdiff, time_t stopdiff)
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_RESCHEDULETIMER;
	
	CTimerd::commandModifyTimer msgModifyTimer;
	msgModifyTimer.eventID = eventid;
	msgModifyTimer.announceTime = announcediff;
	msgModifyTimer.alarmTime = alarmdiff;
	msgModifyTimer.stopTime = stopdiff;

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*) &msgModifyTimer, sizeof(msgModifyTimer));

	timerd_close();

}

int CTimerdClient::addTimerEvent( CTimerEvent::CTimerEventTypes evType, void* data , int min, int hour, int day, int month, CTimerEvent::CTimerEventRepeat evrepeat)
{
	dprintf("addTimerEvent(Typ:%d,min: %d, hour: %d, day: %d, month: %d repeat:%d\n",evType,min,hour,day,month,evrepeat);
	time_t actTime_t;
	time(&actTime_t);
	struct tm* actTime = localtime(&actTime_t);

	actTime->tm_min = min;
	actTime->tm_hour = hour;

	if (day > 0)
		actTime->tm_mday = day;
	if (month > 0)
		actTime->tm_mon = month -1; 
	
	addTimerEvent(evType,true,data,0,mktime(actTime),0);
}

int CTimerdClient::addTimerEvent( CTimerEvent::CTimerEventTypes evType, bool _new, void* data, time_t announcetime, time_t alarmtime,time_t stoptime, CTimerEvent::CTimerEventRepeat evrepeat)
{
	dprintf("addTimerEvent(Type: %d,data: %x announce: %ld, alarm: %ld, stop: %ld repeat: %d\n",evType,data,announcetime,alarmtime,stoptime,evrepeat);
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_ADDTIMER;


	CTimerd::commandAddTimer msgAddTimer;
	msgAddTimer.alarmTime  = alarmtime;
	msgAddTimer.announceTime = announcetime;
	msgAddTimer.stopTime   = stoptime;
	msgAddTimer.eventType = evType ;
	msgAddTimer.eventRepeat = evrepeat;

	int length;
	if ( evType == CTimerEvent::TIMER_SHUTDOWN || evType == CTimerEvent::TIMER_RECORD || evType == CTimerEvent::TIMER_SLEEPTIMER )
	{
		length = 0;
	}
	else if (evType == CTimerEvent::TIMER_NEXTPROGRAM || evType == CTimerEvent::TIMER_ZAPTO)
	{
		length = sizeof( CTimerEvent::EventInfo);
	}
	else if(evType == CTimerEvent::TIMER_STANDBY)
	{
		length = sizeof(CTimerd::commandSetStandby);
	}
	else
	{
		length = 0;
	}

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msgAddTimer, sizeof(msgAddTimer));
	if((data != NULL) && (length > 0))
		send((char*)data, length);

	CTimerd::responseAddTimer response;
	receive((char*)&response, sizeof(response));
	timerd_close();

	return( response.eventID);
}

void CTimerdClient::removeTimerEvent( int evId)
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_REMOVETIMER;
	CTimerd::commandRemoveTimer msgRemoveTimer;
	msgRemoveTimer.eventID  = evId;
 
	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*) &msgRemoveTimer, sizeof(msgRemoveTimer));

	timerd_close();
	dprintf("removed event %d\n",evId);
	
}

bool CTimerdClient::isTimerdAvailable()
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_TIMERDAVAILABLE;
	timerd_connect();
	try
	{
		send((char*)&msg, sizeof(msg));
		CTimerd::responseAvailable response;
		return (receive((char*)&response, sizeof(response)));
	}
	catch (...)
	{
		printf("[timerdclient] isTimerdAvailable() caught exception");
		return false;
	}
	timerd_close();
}

