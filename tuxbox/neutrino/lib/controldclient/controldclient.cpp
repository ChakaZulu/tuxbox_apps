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

#include "controldclient.h"


CControldClient::CControldClient()
{
	sock_fd = 0;
}

bool CControldClient::controld_connect()
{
	controld_close();

	struct sockaddr_un servaddr;
	int clilen;
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, CONTROLD_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	
	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("controldclient: socket");
		return false;
	}	

	if(connect(sock_fd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
  		perror("controldclient: connect");
		return false;
	}
	return true;
}

bool CControldClient::controld_close()
{
	if(sock_fd!=0)
	{
		close(sock_fd);
		sock_fd=0;
	}
}

bool CControldClient::send(char* data, int size)
{
	write(sock_fd, data, size);
}

bool CControldClient::receive(char* data, int size)
{
	read(sock_fd, data, size);
}

void  CControldClient::shutdown()
{
	CControld::commandHead msg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_SHUTDOWN;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	controld_close();
}

void CControldClient::setBoxType(char type)
{
	CControld::commandHead msg;
	CControld::commandBoxType msg2;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_SETBOXTYPE;
	msg2.boxtype = type;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getBoxType()
{
	CControld::commandHead msg;
	CControld::responseBoxType rmsg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_GETBOXTYPE;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.boxtype;
}

void CControldClient::setScartMode(bool mode)
{
	CControld::commandHead msg;
	CControld::commandScartMode msg2;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_SETSCARTMODE;
	msg2.mode = mode;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

/*
char CControldClient::getScartMode()
{
	char scartmode = 0;
	int sockfd = -1;

	remotemsg.version=1;
	remotemsg.cmd=132;
	sockfd = send(false);
	read(sockfd, &scartmode, sizeof(scartmode));
	close(sockfd);
	return scartmode;
}
*/

void CControldClient::setVolume(char volume )
{
	CControld::commandHead msg;
	CControld::commandVolume msg2;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_SETVOLUME;
	msg2.volume = volume;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getVolume()
{
	CControld::commandHead msg;
	CControld::responseVolume rmsg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_GETVOLUME;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.volume;
}

void CControldClient::setVideoFormat(char format)
{
	CControld::commandHead msg;
	CControld::commandVideoFormat msg2;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_SETVIDEOFORMAT;
	msg2.format = format;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getVideoFormat()
{
	CControld::commandHead msg;
	CControld::responseVideoFormat rmsg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_GETVIDEOFORMAT;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.format;
}

void CControldClient::setVideoOutput(char output)
{
	CControld::commandHead msg;
	CControld::commandVideoOutput msg2;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_SETVIDEOOUTPUT;
	msg2.output = output;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getVideoOutput()
{
	CControld::commandHead msg;
	CControld::responseVideoOutput rmsg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_GETVIDEOOUTPUT;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.output;
}


void CControldClient::Mute()
{
	CControld::commandHead msg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_MUTE;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	controld_close();
}

void CControldClient::UnMute()
{
	CControld::commandHead msg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_UNMUTE;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	controld_close();
}

void CControldClient::setMute( bool mute)
{
	if (mute)
		Mute();
	else
		UnMute();
}

bool CControldClient::getMute()
{
	CControld::commandHead msg;
	CControld::responseMute rmsg;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_GETMUTESTATUS;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.mute;
}



void CControldClient::registerEvent(unsigned int eventID, unsigned int clientID, string udsName)
{
	CControld::commandHead msg;
	CEventServer::commandRegisterEvent msg2;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_REGISTEREVENT;
	msg2.eventID = eventID;
	msg2.clientID = clientID;
	strcpy(msg2.udsName, udsName.c_str());
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

void CControldClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	CControld::commandHead msg;
	CEventServer::commandUnRegisterEvent msg2;
	msg.version=CControld::ACTVERSION;
	msg.cmd=CControld::CMD_UNREGISTEREVENT;
	msg2.eventID = eventID;
	msg2.clientID = clientID;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

void CControldClient::videoPowerDown(bool powerdown)
{
        CControld::commandHead msg;
        CControld::commandVideoPowerSave msg2;
        msg.version=CControld::ACTVERSION;
        msg.cmd=CControld::CMD_SETVIDEOPOWERDOWN;
        msg2.powerdown = powerdown;
        controld_connect();
        send((char*)&msg, sizeof(msg));
        send((char*)&msg2, sizeof(msg2));
        controld_close();
}

void CControldClient::saveSettings()
{
        CControld::commandHead msg;
        msg.version=CControld::ACTVERSION;
        msg.cmd=CControld::CMD_SAVECONFIG;
        controld_connect();
        send((char*)&msg, sizeof(msg));
        controld_close();
}