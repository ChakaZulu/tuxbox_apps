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
	commandHead msg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SHUTDOWN;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	controld_close();
}

void CControldClient::setBoxType(char type)
{
	commandHead msg;
	commandBoxType msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETBOXTYPE;
	msg2.boxtype = type;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getBoxType()
{
	commandHead msg;
	responseBoxType rmsg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_GETBOXTYPE;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.boxtype;
}

void CControldClient::setScartMode(bool mode)
{
	commandHead msg;
	commandScartMode msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETSCARTMODE;
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
	commandHead msg;
	commandVolume msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETVOLUME;
	msg2.volume = volume;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getVolume()
{
	commandHead msg;
	responseVolume rmsg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_GETVOLUME;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.volume;
}

void CControldClient::setVideoFormat(char format)
{
	commandHead msg;
	commandVideoFormat msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETVIDEOFORMAT;
	msg2.format = format;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getVideoFormat()
{
	commandHead msg;
	responseVideoFormat rmsg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_GETVIDEOFORMAT;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.format;
}

void CControldClient::setVideoOutput(char output)
{
	commandHead msg;
	commandVideoOutput msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETVIDEOOUTPUT;
	msg2.output = output;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}

char CControldClient::getVideoOutput()
{
	commandHead msg;
	responseVideoOutput rmsg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_GETVIDEOOUTPUT;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.output;
}


void CControldClient::Mute()
{
	commandHead msg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_MUTE;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	controld_close();
}

void CControldClient::UnMute()
{
	commandHead msg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_UNMUTE;
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
	commandHead msg;
	responseMute rmsg;
	msg.version=ACTVERSION;
	msg.cmd=CMD_GETMUTESTATUS;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&rmsg, sizeof(rmsg));
	controld_close();
	return rmsg.mute;
}

void CControldClient::videoPowerDown(bool powerdown)
{
	commandHead msg;
	responseVideoPowerSave msg2;
	msg.version=ACTVERSION;
	msg.cmd=CMD_SETVIDEOPOWERDOWN;
	msg2.powerdown = powerdown;
	controld_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	controld_close();
}
