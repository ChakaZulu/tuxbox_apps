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

#include "lcddclient.h"


CLcddClient::CLcddClient()
{
	sock_fd = 0;
}

bool CLcddClient::lcdd_connect()
{
	lcdd_close();
	//printf("[lcddclient] try connect\n");
	struct sockaddr_un servaddr;
	int clilen;

	std::string filename = LCDD_UDS_NAME;
	filename += ".";
	filename += getSystemId();

	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, filename.c_str());
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("lcddclient: socket");
		return false;
	}	

	if(connect(sock_fd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
  		perror("lcddclient: connect");
		return false;
	}
	return true;
}

bool CLcddClient::lcdd_close()
{
	if(sock_fd!=0)
	{
		//printf("[lcddclient] close\n");
		close(sock_fd);
		sock_fd=0;
	}
}

bool CLcddClient::send(char* data, int size)
{
	write(sock_fd, data, size);
}

bool CLcddClient::receive(char* data, int size)
{
	read(sock_fd, data, size);
}

void CLcddClient::setMode(char mode, string head="")
{
	commandHead msg;
	commandMode msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETMODE;
	msg2.mode = mode;
	strcpy( msg2.text, head.substr(0, sizeof(msg2.text)-2).c_str() );
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setMenuText(char pos, string text, char highlight)
{
	commandHead msg;
	commandMenuText msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETMENUTEXT;
	msg2.position = pos;
	msg2.highlight = highlight;
	strcpy( msg2.text, text.substr(0, sizeof(msg2.text)-2).c_str() );
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setServiceName(string name)
{
	commandHead msg;
	commandServiceName msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETSERVICENAME;
	strcpy( msg2.servicename, name.substr(0, sizeof(msg2.servicename)-2).c_str() );
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setMute(bool mute)
{
	commandHead msg;
	commandMute msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETMUTE;
	msg2.mute = mute;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setVolume(char volume)
{
	commandHead msg;
	commandVolume msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETVOLUME;
	msg2.volume = volume;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::shutdown()
{
	setMode(MODE_SHUTDOWN, "");
}

const std::string CLcddClient::getSystemId ()
{
	char *id = getenv("dsID");

	if (id == NULL)
	{
		return "noSystemId";
	}
	else
	{
		return id;
	}
}

