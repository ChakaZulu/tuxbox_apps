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
	bCallBackRegistered = false;
}

int CControldClient::send(bool closesock)
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(1610);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	#ifdef HAS_SIN_LEN
 		servaddr.sin_len = sizeof(servaddr); // needed ???
	#endif


	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
  		perror("neutrino: connect(controld)");
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
	printf("sending boxtype: %d \n", type);

	send(true);
}

void CControldClient::setScartMode(char mode)
{
	remotemsg.version=1;
	remotemsg.cmd=8;
	remotemsg.param=mode;
	printf("sending scartmode: %d \n", mode);

	send(true);
}

void CControldClient::setVolume(char volume )
{
	remotemsg.version=1;
	remotemsg.cmd=2;
	remotemsg.param=volume;
	printf("sending volume: %d \n", volume);

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
	printf("getting volume: %d \n", volume);
	return volume;
}

void CControldClient::videoFormatCallback( void* arg, int format)
{	CControldClient* Controld = (CControldClient*)arg;

	printf("In VideoFormat-CallBack: %d \n", format);
	int nNeutrinoFormat = 1;
	switch (format)
	{
		case 2 : nNeutrinoFormat = 2; break;
		case 3 : nNeutrinoFormat = 1; break;
	}
	Controld->setVideoFormat(nNeutrinoFormat, false);
}

void CControldClient::setVideoFormat(char format, bool bDoUnregister = true)
{
	remotemsg.version=1;
	remotemsg.cmd=5;
	remotemsg.param=format;
	printf("sending VideoFormat: %d \n", format);
	send(true);

}

void CControldClient::setVideoOutput(char format)
{
	remotemsg.version=1;
	remotemsg.cmd=6;
	remotemsg.param=format;
	printf("sending VideoOutput: %d \n", format);

	send(true);
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


