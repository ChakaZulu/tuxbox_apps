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

#ifndef __controldclient__
#define __controldclient__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <string>
#include <stdio.h>
#include <unistd.h>

#include <string>

using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in



class CControldClient
{
	struct ctrl_rmsg
	{
		unsigned char version;
		unsigned char cmd;
		unsigned char param;
		unsigned short param2;
		char param3[30];
	}	remotemsg;

	int send(bool closesock);

	public:

		CControldClient();
		void setVolume(char volume );
		char getVolume();
		void setVideoFormat(char);
		void setVideoOutput(char);
		void setBoxType(char);
		void setScartMode(char);

		void Mute();
		void UnMute();
		void setMute( bool );
		char getMute();

		void shutdown();
};

#endif


