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

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "controldclient.h"
#include "controldMsg.h"


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
		return true;
	}
	else
	{
		return false;
	}
}

bool CControldClient::send_data(char* data, int size)
{
	write(sock_fd, data, size);
	return true;
}

bool CControldClient::receive(char* data, int size)
{
	read(sock_fd, data, size);
	return true;
}

void CControldClient::send(const unsigned char command, char* data = NULL, const unsigned int size = 0)
{
	CControld::commandHead msgHead;
	msgHead.version = CControld::ACTVERSION;
	msgHead.cmd     = command;
	controld_connect();
	send_data((char*)&msgHead, sizeof(msgHead));
	if (size != 0)
	    send_data(data, size);
}

void  CControldClient::shutdown()
{
	send(CControld::CMD_SHUTDOWN);
	controld_close();
}

void CControldClient::setBoxType(char type)
{
	CControld::commandBoxType msg2;

	msg2.boxtype = type;

	send(CControld::CMD_SETBOXTYPE, (char*)&msg2, sizeof(msg2));

	controld_close();
}

char CControldClient::getBoxType()
{
	CControld::responseBoxType rmsg;

	send(CControld::CMD_GETBOXTYPE);

	receive((char*)&rmsg, sizeof(rmsg));

	controld_close();

	return rmsg.boxtype;
}

void CControldClient::setScartMode(bool mode)
{
	CControld::commandScartMode msg2;

	msg2.mode = mode;

	send(CControld::CMD_SETSCARTMODE, (char*)&msg2, sizeof(msg2));

	controld_close();
}

void CControldClient::setVolume(char volume )
{
	CControld::commandVolume msg2;

	msg2.volume = volume;

	send(CControld::CMD_SETVOLUME_AVS, (char*)&msg2, sizeof(msg2));

	controld_close();
}

char CControldClient::getVolume()
{
	CControld::responseVolume rmsg;

	send(CControld::CMD_GETVOLUME_AVS);

	receive((char*)&rmsg, sizeof(rmsg));

	controld_close();

	return rmsg.volume;
}

void CControldClient::setVideoFormat(char format)
{
	CControld::commandVideoFormat msg2;

	msg2.format = format;

	send(CControld::CMD_SETVIDEOFORMAT, (char*)&msg2, sizeof(msg2));

	controld_close();
}

char CControldClient::getAspectRatio()
{
	CControld::responseAspectRatio rmsg;

	send(CControld::CMD_GETASPECTRATIO);

	receive((char*)&rmsg, sizeof(rmsg));

	controld_close();

	return rmsg.aspectRatio;
}

char CControldClient::getVideoFormat()
{
	CControld::responseVideoFormat rmsg;

	send(CControld::CMD_GETVIDEOFORMAT);

	receive((char*)&rmsg, sizeof(rmsg));

	controld_close();

	return rmsg.format;
}

void CControldClient::setVideoOutput(char output)
{
	CControld::commandVideoOutput msg2;

	msg2.output = output;

	send(CControld::CMD_SETVIDEOOUTPUT, (char*)&msg2, sizeof(msg2));

	controld_close();
}

char CControldClient::getVideoOutput()
{
	CControld::responseVideoOutput rmsg;

	send(CControld::CMD_GETVIDEOOUTPUT);

	receive((char*)&rmsg, sizeof(rmsg));

	controld_close();

	return rmsg.output;
}


void CControldClient::Mute(const bool avs)
{
	if (avs)
		send(CControld::CMD_MUTE_AVS);
	else
		send(CControld::CMD_MUTE);
	controld_close();
}

void CControldClient::UnMute(const bool avs)
{
	if (avs)
		send(CControld::CMD_UNMUTE_AVS);
	else
		send(CControld::CMD_UNMUTE);
	controld_close();
}

void CControldClient::setMute(const bool mute, const bool avs)
{
	if (mute)
		Mute(avs);
	else
		UnMute(avs);
}

bool CControldClient::getMute(const bool avs)
{
	CControld::responseMute rmsg;

	if (avs)
		send(CControld::CMD_GETMUTESTATUS_AVS);
	else
		send(CControld::CMD_GETMUTESTATUS);

	receive((char*)&rmsg, sizeof(rmsg));

	controld_close();

	return rmsg.mute;
}

void CControldClient::setAnalogOutput(int mode)
{
	CControld::commandAnalogMode msg2;

	msg2.mode = mode;

	send(CControld::CMD_SETANALOGMODE, (char*)&msg2, sizeof(msg2));

	controld_close();
}

void CControldClient::registerEvent(unsigned int eventID, unsigned int clientID, string udsName)
{
	CEventServer::commandRegisterEvent msg2;

	msg2.eventID = eventID;
	msg2.clientID = clientID;
	strcpy(msg2.udsName, udsName.c_str());

	send(CControld::CMD_REGISTEREVENT, (char*)&msg2, sizeof(msg2));

	controld_close();
}

void CControldClient::unRegisterEvent(unsigned int eventID, unsigned int clientID)
{
	CEventServer::commandUnRegisterEvent msg2;

	msg2.eventID = eventID;
	msg2.clientID = clientID;

	send(CControld::CMD_UNREGISTEREVENT, (char*)&msg2, sizeof(msg2));

	controld_close();
}

void CControldClient::videoPowerDown(bool powerdown)
{
        CControld::commandVideoPowerSave msg2;

        msg2.powerdown = powerdown;

        send(CControld::CMD_SETVIDEOPOWERDOWN, (char*)&msg2, sizeof(msg2));

        controld_close();
}

void CControldClient::saveSettings()
{
        send(CControld::CMD_SAVECONFIG);
        controld_close();
}
