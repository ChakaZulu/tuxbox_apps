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

#ifndef __libevent__
#define __libevent__

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <string>
#include <map>

using namespace std;


class CEventServer
{
		struct eventClient
		{
			unsigned int clientID;//doppelt..
			char udsName[50];
		};

		//key: clientid 
		typedef std::map<unsigned int, eventClient> eventClientMap;

		//key: eventid
		std::map<unsigned int, eventClientMap> eventData;

		bool sendEvent2Client(unsigned int eventID, unsigned int initiatorID, eventClient* ClientData, void* eventbody=NULL, unsigned int eventbodysize=0);

	public:

		struct commandRegisterEvent
		{
			unsigned int eventID;
			unsigned int clientID;
			char udsName[50];
		};

		struct commandUnRegisterEvent
		{
			unsigned int eventID;
			unsigned int clientID;
		};

		struct eventHead
		{
			unsigned int eventID;
			unsigned int initiatorID;
		};

		void registerEvent(unsigned int eventID, unsigned int ClientID, string udsName);
		void registerEvent(int fd);
		void unRegisterEvent(unsigned int eventID, unsigned int ClientID);
		void unRegisterEvent(int fd);

		void sendEvent(unsigned int eventID, unsigned int initiatorID, void* eventbody=NULL, unsigned int eventbodysize=0);

};


#endif
