/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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
	write(sock_fd, data, size);
}

bool CTimerdClient::receive(char* data, int size)
{
	read(sock_fd, data, size);
}

int CTimerdClient::addTimerEvent( timerTypes evType, void* data = 0, int min = 0, int hour = 0, int day = 0, int month = 0)
{
	CTimerd::commandHead msg;
	msg.version=CTimerd::ACTVERSION;
	msg.cmd=CTimerd::CMD_ADDTIMER;

	time_t actTime_t;
	::time(&actTime_t);
	struct tm* actTime = localtime(&actTime_t);
	actTime->tm_mon += 1;

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
	switch( evType)
	{
		TIMER_SHUTDOWN :
			length = 0;
		break;
		TIMER_NEXTPROGRAM :
			length = sizeof( CTimerEvent_NextProgram::EventInfo);
		break;
		default:
			length = 0;
	}

	timerd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msgAddTimer, sizeof(msgAddTimer));
	send((char*)data, length);

	CTimerd::responseAddTimer response;
	receive((char*)&response, sizeof(response));
	timerd_close();

	return( response.eventID);
}

void CTimerdClient::removeTimerEvent( int evId)
{
}
