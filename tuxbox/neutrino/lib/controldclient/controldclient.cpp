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
	memset(&remotemsg, 0, sizeof(remotemsg) );
}

int CControldClient::send(bool closesock)
{
	int sock_fd;

	struct sockaddr_un servaddr, cliaddr;
	int clilen;
	memset(&servaddr, 0, sizeof(struct sockaddr_un));
	servaddr.sun_family = AF_UNIX;
	strcpy(servaddr.sun_path, CONTROLD_UDS_NAME);
	clilen = sizeof(servaddr.sun_family) + strlen(servaddr.sun_path);
	
	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
	}	

	if(connect(sock_fd, (struct sockaddr*) &servaddr, clilen) <0 )
	{
  		perror("controldclient: connect(controld)");
		return -1;
	}

	write(sock_fd, &remotemsg, sizeof(remotemsg));
	if(closesock)
		close(sock_fd);
	memset(&remotemsg, 0, sizeof(remotemsg));
	return sock_fd;
}

void  CControldClient::shutdown()
{
	remotemsg.version=1;
	remotemsg.cmd=1;

	send(true);
}

void CControldClient::setBoxType(char type)
{
	remotemsg.version=1;
	remotemsg.cmd=7;
	remotemsg.param=type;
	send(true);
}

char CControldClient::getBoxType()
{
	char boxtype = 0;
	int sockfd = -1;

	remotemsg.version=1;
	remotemsg.cmd=132;
	sockfd = send(false);
	read(sockfd, &boxtype, sizeof(boxtype));
	close(sockfd);
	return boxtype;
}

void CControldClient::setScartMode(char mode)
{
	remotemsg.version=1;
	remotemsg.cmd=8;
	remotemsg.param=mode;
	send(true);
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
	remotemsg.version=1;
	remotemsg.cmd=2;
	remotemsg.param=volume;
	send(true);
}

char CControldClient::getVolume()
{
	char volume = 0;
	int sockfd = -1;

	remotemsg.version=1;
	remotemsg.cmd=128;
	sockfd = send(false);
	read(sockfd, &volume, sizeof(volume));
	close(sockfd);
	return volume;
}

void CControldClient::setVideoFormat(char format)
{
	remotemsg.version=1;
	remotemsg.cmd=5;
	remotemsg.param=format;
	send(true);
}

char CControldClient::getVideoFormat()
{
	char videoformat = 0;
	int sockfd = -1;

	remotemsg.version=1;
	remotemsg.cmd=130;
	sockfd = send(false);
	read(sockfd, &videoformat, sizeof(videoformat));
	close(sockfd);
	return videoformat;
}

void CControldClient::setVideoOutput(char format)
{
	remotemsg.version=1;
	remotemsg.cmd=6;
	remotemsg.param=format;
	send(true);
}

char CControldClient::getVideoOutput()
{
	char videooutput = 0;
	int sockfd = -1;

	remotemsg.version=1;
	remotemsg.cmd=131;
	sockfd = send(false);
	read(sockfd, &videooutput, sizeof(videooutput));
	close(sockfd);
	return videooutput;
}


void CControldClient::Mute()
{
	remotemsg.version=1;
	remotemsg.cmd=3;

	send(true);
}

void CControldClient::UnMute()
{
	remotemsg.version=1;
	remotemsg.cmd=4;

	send(true);
}

void CControldClient::setMute( bool mute)
{
	if (mute)
		Mute();
	else
		UnMute();
}

char CControldClient::getMute()
{
        char mute = 0;
        int sockfd = -1;

        remotemsg.version=1;
        remotemsg.cmd=129;
        sockfd = send(false);
        read(sockfd, &mute, sizeof(mute));
        close(sockfd);
        return mute;    
}
