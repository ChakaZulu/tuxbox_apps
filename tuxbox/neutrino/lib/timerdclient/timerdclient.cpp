/*
	Timer Daemon  -   DBoxII-Project

	Copyright (C) 2002 Dirk Szymanski 'Dirch'
	
	$Id: timerdclient.cpp,v 1.24 2002/10/13 05:42:52 woglinde Exp $

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

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sys/socket.h>
//#include <netinet/in.h>
//#include <netinet/in_systm.h>
//#include <netinet/ip.h>
//#include <netdb.h>
//#include <arpa/inet.h>

#include <timerdclient.h>


CTimerdClient::CTimerdClient()
{
	sock_fd = 0;
}
//-------------------------------------------------------------------------

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
//-------------------------------------------------------------------------

bool CTimerdClient::timerd_close()
{
	if(sock_fd!=0)
	{
		close(sock_fd);
		sock_fd=0;
		return true;
	}
	else
	{
		return false;
	}
}
//-------------------------------------------------------------------------

bool CTimerdClient::send(char* data, int size)
{
	return( write(sock_fd, data, size) == size);
}
//-------------------------------------------------------------------------

bool CTimerdClient::receive(char* data, int size)
{
	return( read(sock_fd, data, size) == size);
}
//-------------------------------------------------------------------------

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
//-------------------------------------------------------------------------

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
//-------------------------------------------------------------------------

int CTimerdClient::setSleeptimer(time_t announcetime, time_t alarmtime, int timerid)
{
int timerID;

	if(timerid == 0)
		timerID = getSleeptimerID();
	else
		timerID = timerid;

	if(timerID != 0)
	{
		dprintf("Modify Sleeptimer announce: %ld alarm: %ld\n",announcetime,alarmtime);
		modifyTimerEvent(timerID, announcetime, alarmtime, 0);
	}
	else
	{
		dprintf("Set New Sleeptimerannounce: %ld alarm: %ld\n",announcetime,alarmtime);
		timerID = addTimerEvent(CTimerEvent::TIMER_SLEEPTIMER,NULL,announcetime,alarmtime,0);
	}

	return timerID;	
}
//-------------------------------------------------------------------------

int CTimerdClient::getSleeptimerID()
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_GETSLEEPTIMER;
	timerd_connect();
	send((char*)&msg, sizeof(msg));
	CTimerd::responseGetSleeptimer response;
	if (!receive((char*)&response, sizeof(CTimerd::responseGetSleeptimer)))
		response.eventID =0;
	timerd_close();	
	return response.eventID;
}
//-------------------------------------------------------------------------

int CTimerdClient::getSleepTimerRemaining()
{
	int timerID;
	if((timerID = getSleeptimerID()) != 0)
	{
		CTimerd::responseGetTimer timer;
		getTimer( timer, timerID);
		int min=(((timer.alarmTime + 1 - time(NULL)) / 60)+1); //aufrunden auf nächst größerere Min.
		if(min <1)
			min=1;
		return min;
	}
	else
		return 0;
}
//-------------------------------------------------------------------------

void CTimerdClient::getTimerList( CTimerd::TimerList &timerlist)
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_GETTIMERLIST;

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	timerlist.clear();
	CTimerd::responseGetTimer response;
	while ( receive((char*)&response, sizeof(CTimerd::responseGetTimer)))
	{
		if(response.eventState != CTimerEvent::TIMERSTATE_TERMINATED)
			timerlist.insert( timerlist.end(), response);
	}
	timerd_close();
}
//-------------------------------------------------------------------------

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
//-------------------------------------------------------------------------


bool CTimerdClient::modifyTimerEvent(int eventid, time_t announcetime, time_t alarmtime, time_t stoptime, CTimerEvent::CTimerEventRepeat evrepeat)
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
	msgModifyTimer.eventRepeat = evrepeat;

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*) &msgModifyTimer, sizeof(msgModifyTimer));

	CTimerd::responseStatus response;
	receive((char*)&response, sizeof(response));

	timerd_close();
	return true;
}
//-------------------------------------------------------------------------

bool CTimerdClient::rescheduleTimerEvent(int eventid, time_t diff)
{
	rescheduleTimerEvent(eventid,diff,diff,diff);
	return true;
}
//-------------------------------------------------------------------------

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
	
	CTimerd::responseStatus response;
	receive((char*)&response, sizeof(response));

	timerd_close();
	return response.status;
}
//-------------------------------------------------------------------------

/*
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
*/
//-------------------------------------------------------------------------
int CTimerdClient::addTimerEvent( CTimerEvent::CTimerEventTypes evType, void* data, time_t announcetime, time_t alarmtime,time_t stoptime, CTimerEvent::CTimerEventRepeat evrepeat)
{
	dprintf("addTimerEvent(Type: %d, announce: %ld, alarm: %ld, stop: %ld repeat: %d\n",evType,announcetime,alarmtime,stoptime,evrepeat);
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
	if ( evType == CTimerEvent::TIMER_SHUTDOWN || evType == CTimerEvent::TIMER_SLEEPTIMER )
	{
		length = 0;
	}
	else if (evType == CTimerEvent::TIMER_NEXTPROGRAM || evType == CTimerEvent::TIMER_ZAPTO || evType == CTimerEvent::TIMER_RECORD )
	{
		length = sizeof( CTimerEvent::EventInfo);
	}
	else if(evType == CTimerEvent::TIMER_STANDBY)
	{
		length = sizeof(CTimerd::commandSetStandby);
	}
	else if(evType == CTimerEvent::TIMER_REMIND)
	{
		length = sizeof(CTimerd::commandRemind);
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
//-------------------------------------------------------------------------

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
}
//-------------------------------------------------------------------------

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
		bool ret=receive((char*)&response, sizeof(response));
		timerd_close();
		return ret;
	}
	catch (...)
	{
		printf("[timerdclient] isTimerdAvailable() caught exception");
		timerd_close();
		return false;
	}
}
//-------------------------------------------------------------------------
bool CTimerdClient::shutdown()
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_SHUTDOWN;
 
	timerd_connect();
	send((char*)&msg, sizeof(msg));
	
	CTimerd::responseStatus response;
	receive((char*)&response, sizeof(response));

	timerd_close();
	return response.status;
}
//-------------------------------------------------------------------------

