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
$Log: tdt.cpp,v $
Revision 1.4  2002/06/02 12:18:47  TheDOC
source reformatted, linkage-pids correct, xmlrpc removed, all debug-messages removed - 110k smaller lcars with -Os :)

Revision 1.3  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>

#include <ost/dmx.h>

#include "tdt.h"
#include "help.h"
#include "pthread.h"

#define BSIZE 10000

int tdt::start_thread()
{

	int status;

	status = pthread_create( &timeThread,
	                         NULL,
	                         start_timereader,
	                         (void *)this );
	return status;

}



void* tdt::start_timereader( void * this_ptr )
{
	while(1)
	{
		int fd, r;
		struct dmxSctFilterParams flt;
		unsigned char buffer[BSIZE];
		time_t acttime = 0;

		while(acttime < 100000)
		{
			// Lies den TDT
			if ((fd=open("/dev/ost/demux0", O_RDONLY)) < 0)
				perror("TDT open");

			memset (&flt.filter, 0, sizeof (struct dmxFilter));
			r = BSIZE;
			flt.pid            = 0x14;
			flt.filter.filter[0] = 0x70;
			flt.filter.mask[0] = 0xFF;
			flt.timeout        = 0;
			flt.flags          = DMX_IMMEDIATE_START | DMX_ONESHOT;

			if (ioctl(fd, DMX_SET_FILTER, &flt) < 0)
				perror("TDT ioctl");
			r=read(fd, buffer, r);
			ioctl(fd,DMX_STOP,0);

			close(fd);

			if (r == 0)
				continue;

			int time = (buffer[5] << 16) | (buffer[6] << 8) | buffer[7];
			int mjd = (buffer[3] << 8) | buffer[4];
			//printf("Time: %x - MJD: %x\n", time, mjd);
			acttime = dvbtimeToLinuxTime(mjd, time);
			stime(&acttime);
			//printf("----------Aktuelle Zeit: %d\n", (int)acttime);
			//printf("----------Aktuelle Zeit: %s\n", ctime(&acttime));
		}
		sleep(1800); // alle 30 Minuten die Zeit neu setzen...
	}

}
