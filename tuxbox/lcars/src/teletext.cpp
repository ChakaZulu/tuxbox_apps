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

void teletext::getTXT(int PID)
{
	int fd, r;
	struct dmxPesFilterParams flt;
	unsigned char buffer[BSIZE];
		
	if ((fd=open("/dev/ost/demux0", O_RDONLY)) < 0)
		perror("Teletext open");
		
	r = BSIZE;
	printf("TXT-PID: %x\n", PID);
	flt.pid            = PID;
	flt.input=DMX_IN_FRONTEND;
    flt.output=DMX_OUT_TAP;
    flt.pesType=DMX_PES_OTHER;
    flt.flags=DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_PES_FILTER, &flt) < 0)
		perror("Teletext ioctl");

	for (int i = 0; i < 100; i++)
	{
		r = BSIZE;
		r=read(fd, buffer, r);
		printf("\n-------------------------------\n");
		printf("r: %d\n", r);
		for (int j = 0; j < r; j++)
		{
			if ((buffer[j] > 64 && buffer[j] < 123) || buffer[j] == 32 || buffer[j] == 0x2c || buffer[j] == 0x2e || buffer[j] == 0xfc || buffer[j] == 0xf6 || buffer[j] == 0xe4 || buffer[j] == 0xdf)
				printf("%c", buffer[j]);
			else if (buffer[j] == 10)
				printf("\n");
			else
				printf("%x ", buffer[j]);
			
		}
	}
	
	ioctl(fd,DMX_STOP,0);

	close(fd);
}
