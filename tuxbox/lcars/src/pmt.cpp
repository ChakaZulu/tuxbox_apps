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
$Log: pmt.cpp,v $
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

#include "pmt.h"

#define BSIZE 10000

pmt_data pmt::readPMT(int pmt_pid)
{
	int fd, r;
	struct dmxSctFilterParams flt;
	unsigned char buffer[BSIZE];

	// Lies den PMT
	fd=open("/dev/ost/demux0", O_RDONLY);
	
	memset (&flt.filter, 0, sizeof (struct dmxFilter));
	r = BSIZE;
	flt.pid            = pmt_pid;
	flt.filter.filter[0] = 0x2;
	flt.filter.mask[0] = 0xFF;
	flt.timeout        = 10000;
	flt.flags          = DMX_IMMEDIATE_START | DMX_ONESHOT;
	
	ioctl(fd, DMX_SET_FILTER, &flt);
	r=read(fd, buffer, r);
	ioctl(fd,DMX_STOP,0);
		
	close(fd);
	
	struct pmt_data tmp_pmt;
	memset (&tmp_pmt, 0, sizeof (struct pmt_data));
	tmp_pmt.ecm_counter = 0;
	
	tmp_pmt.PCR = (buffer[8] & 0x1f) << 8 | buffer[9];
	
	int descriptors_length = ((buffer[10] & 0xf) << 8 | buffer[11]);
	int start = 12;
	while (start < 12 + descriptors_length)
	{
		if (buffer[start] == 0x09) // CA
		{
			tmp_pmt.CAID[tmp_pmt.ecm_counter] = (buffer[start + 2] << 8) | buffer[start + 3];
			tmp_pmt.ECM[tmp_pmt.ecm_counter++] = ((buffer[start + 4] & 0x1f) << 8) | buffer[start + 5];
		}
		
		start += buffer[start + 1] + 2;
	}

	int count = 12 + ((buffer[10] & 0xf) << 8 | buffer[11]);

	while (count < r - 5)
	{
		tmp_pmt.type[tmp_pmt.pid_counter] = buffer[count];
		tmp_pmt.PID[tmp_pmt.pid_counter] = (buffer[count + 1] & 0x1f) << 8 | buffer[count + 2];
		tmp_pmt.subtype[tmp_pmt.pid_counter] = 0;

		int start = count + 5;
		int descriptors_length = ((buffer[count + 3] & 0xf) << 8 | buffer[count + 4]);
		while (start < count + 5 + descriptors_length)
		{
			if (buffer[start] == 0x52) // stream_identifier_descriptor
			{
				tmp_pmt.component[tmp_pmt.pid_counter] = buffer[start + 2];
			}
			else if (buffer[start] == 0x56) // teletext_descriptor
			{
				printf("---> teletext_descriptor: <---\n");
				for (int i = start + 3; i < start + 3 + buffer[start + 1]; i += 5)
				{
				}
				tmp_pmt.subtype[tmp_pmt.pid_counter] = 1;

			}
			start += buffer[start + 1] + 2;
		}
		tmp_pmt.pid_counter++;
		count += 5 + ((buffer[count + 3] & 0xf) << 8 | buffer[count + 4]);
	}
	return tmp_pmt;
}
