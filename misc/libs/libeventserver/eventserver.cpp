/*
	Event-Server  -   DBoxII-Project

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

#include "eventserver.h"

void CEventServer::registerEvent(unsigned int eventID, unsigned int ClientID, string udsName)
{
	strcpy( eventData[eventID][ClientID].udsName, udsName.c_str());
	eventData[eventID][ClientID].clientID=ClientID;
}

void CEventServer::registerEvent(int fd)
{
	commandRegisterEvent msg;
	read(fd, &msg, sizeof(msg));
	registerEvent(msg.eventID, msg.clientID, msg.udsName);
}

void CEventServer::unRegisterEvent(unsigned int eventID, unsigned int ClientID)
{
	eventData[eventID].erase( ClientID );
}

void CEventServer::unRegisterEvent(int fd)
{
	commandUnRegisterEvent msg;
	read(fd, &msg, sizeof(msg));
	unRegisterEvent(msg.eventID, msg.clientID);
}

void CEventServer::sendEvent(unsigned int eventID, unsigned int initiatorID, void* eventbody, unsigned int eventbodysize)
{
	eventClientMap notifyClients = eventData[eventID];

	eventClientMap::iterator pos = notifyClients.begin();
	for(;pos!=notifyClients.end();pos++)
	{
		//allen clients ein event schicken
		eventClient client = pos->second;
		//printf("[eventserver]: send event (%d) to: %d - %s\n", eventID, client.clientID, client.udsName);
		sendEvent2Client(eventID, initiatorID, &client, eventbody, eventbodysize);
	}
}

bool CEventServer::sendEvent2Client(unsigned int eventID, unsigned int initiatorID, eventClient* ClientData, void* eventbody, unsigned int eventbodysize)
{
	struct sockaddr_un servaddr;
	int clilen, sock_fd;

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, ClientData->udsName);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("[eventserver]: socket");
		return false;
	}

	if(connect(sock_fd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
  		perror("[eventserver]: connect");
		return false;
	}

	eventHead head;
	head.eventID = eventID;
	head.initiatorID = initiatorID;
	head.dataSize = eventbodysize;
	int written = write(sock_fd, &head, sizeof(head));
	//printf ("[eventserver]: sent 0x%x - following eventbody= %d\n", written, eventbodysize );

	if(eventbodysize!=0)
	{
		written = write(sock_fd, eventbody, eventbodysize);
	}
	close(sock_fd);
}
