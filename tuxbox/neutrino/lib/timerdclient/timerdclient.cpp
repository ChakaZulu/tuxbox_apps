/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/
	
	$Id: timerdclient.cpp,v 1.7 2002/05/17 13:06:51 woglinde Exp $

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
		timerlist.insert( timerlist.end(), response);
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
	timer = response;
	receive((char*)&response, sizeof(CTimerd::responseGetTimer));
	timerd_close();
}


int CTimerdClient::addTimerEvent( timerTypes evType, void* data, int min, int hour, int day, int month)
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_ADDTIMER;

	time_t actTime_t;
	::time(&actTime_t);
	struct tm* actTime = localtime(&actTime_t);

	if (min==0)
		min = actTime->tm_min;
	if (hour==0)
		hour = actTime->tm_hour;
	if (day==0)
		day = actTime->tm_mday;
	if (month==0)
		month = actTime->tm_mon;

	CTimerd::commandAddTimer msgAddTimer;
	msgAddTimer.month  = month  ;
	msgAddTimer.day    = day    ;
	msgAddTimer.hour   = hour   ;
	msgAddTimer.min    = min    ;
	msgAddTimer.evType = evType ;

	int length;
	if ( evType == TIMER_SHUTDOWN || evType == TIMER_RECORD)
	{
		length = 0;
	}
	else if (evType == TIMER_NEXTPROGRAM)
	{
		length = sizeof( CTimerd::EventInfo);
	}
	else if(evType == TIMER_STANDBY)
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

