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
#include "lcddMsg.h"

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>


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
		return true;
	}
	return false;
}

bool CLcddClient::send(char* data, int size)
{
	write(sock_fd, data, size);
	return true;
}

bool CLcddClient::receive(char* data, int size)
{
	read(sock_fd, data, size);
	return true;
}

void CLcddClient::setMode(char mode, string head)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandMode msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETMODE;
	msg2.mode = mode;
	strcpy( msg2.text, head.substr(0, sizeof(msg2.text)-2).c_str() );
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setMenuText(char pos, string text, char highlight)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandMenuText msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETMENUTEXT;
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
	CLcddMsg::commandHead msg;
	CLcddMsg::commandServiceName msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETSERVICENAME;
	strcpy( msg2.servicename, name.substr(0, sizeof(msg2.servicename)-2).c_str() );
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setMute(bool mute)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandMute msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETMUTE;
	msg2.mute = mute;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setVolume(char volume)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandVolume msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETVOLUME;
	msg2.volume = volume;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

void CLcddClient::setContrast(int contrast)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandSetBrightness msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETLCDCONTRAST;
	msg2.brightness = contrast;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

int CLcddClient::getContrast()
{
	CLcddMsg::commandHead msg;
	CLcddMsg::responseGetBrightness msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_GETLCDCONTRAST;

	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&msg2, sizeof(msg2));
	lcdd_close();

	return msg2.brightness;
}

void CLcddClient::setBrightness(int brightness)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandSetBrightness msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETLCDBRIGHTNESS;
	msg2.brightness = brightness;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

int CLcddClient::getBrightness()
{
	CLcddMsg::commandHead msg;
	CLcddMsg::responseGetBrightness msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_GETLCDBRIGHTNESS;

	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&msg2, sizeof(msg2));
	lcdd_close();

	return msg2.brightness;
}

void CLcddClient::setBrightnessStandby(int brightness)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandSetBrightness msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETSTANDBYLCDBRIGHTNESS;
	msg2.brightness = brightness;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

int CLcddClient::getBrightnessStandby()
{
	CLcddMsg::commandHead msg;
	CLcddMsg::responseGetBrightness msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_GETSTANDBYLCDBRIGHTNESS;

	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&msg2, sizeof(msg2));
	lcdd_close();

	return msg2.brightness;
}

void CLcddClient::setPower(bool power)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandPower msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETLCDPOWER;
	msg2.power = power;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

bool CLcddClient::getPower()
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandPower msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_GETLCDPOWER;

	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&msg2, sizeof(msg2));
	lcdd_close();

	return msg2.power;
}

void CLcddClient::setInverse(bool inverse)
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandInverse msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SETLCDINVERSE;
	msg2.inverse = inverse;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	send((char*)&msg2, sizeof(msg2));
	lcdd_close();
}

bool CLcddClient::getInverse()
{
	CLcddMsg::commandHead msg;
	CLcddMsg::commandInverse msg2;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_GETLCDINVERSE;

	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	receive((char*)&msg2, sizeof(msg2));
	lcdd_close();

	return msg2.inverse;
}

void CLcddClient::shutdown()
{
	setMode(CLcddClient::MODE_SHUTDOWN, "");

	CLcddMsg::commandHead msg;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_SHUTDOWN;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	lcdd_close();
}

void CLcddClient::update()
{
	CLcddMsg::commandHead msg;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_UPDATE;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	lcdd_close();
}

void CLcddClient::pause()
{
	CLcddMsg::commandHead msg;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_PAUSE;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	lcdd_close();
}

void CLcddClient::resume()
{
	CLcddMsg::commandHead msg;
	msg.version=CLcddMsg::ACTVERSION;
	msg.cmd=CLcddMsg::CMD_RESUME;
	lcdd_connect();
	send((char*)&msg, sizeof(msg));
	lcdd_close();
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

