/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: teletext.cpp,v $
Revision 1.8  2002/10/05 20:31:10  obi
napi style teletext

Revision 1.7  2002/06/12 17:46:53  TheDOC
reinsertion readded

Revision 1.6  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.4  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.4  2001/12/20 00:31:38  tux
Plugins korrigiert

Revision 1.3  2001/12/17 14:00:41  tux
Another commit

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>

#include <ost/dmx.h>

#define BSIZE 10000

#include "teletext.h"

static int txtfd = -1;

void teletext::getTXT(int PID)
{
}

void teletext::startReinsertion(int PID)
{
	dmxPesFilterParams pesFilterParams;

	pesFilterParams.pid = PID;
	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = DMX_OUT_DECODER;
	pesFilterParams.pesType = DMX_PES_TELETEXT;
	pesFilterParams.flags  = DMX_IMMEDIATE_START;

	std::cout << "Start reinsertion on PID " << PID << std::endl;
	
	if (txtfd == -1)
		txtfd = open("/dev/dvb/card0/demux0", O_RDWR);
	
	if (ioctl(txtfd, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
		perror("DMX_SET_PES_FILTER");

}

void teletext::stopReinsertion()
{
	std::cout << "Stop reinsertion" << std::endl;
	
	int txtfd = open("/dev/dbox/vbi0", O_RDWR);
	
	if (ioctl(txtfd, DMX_STOP) < 0)
		perror("DMX_STOP");

	close(txtfd);
	txtfd = -1;
}

