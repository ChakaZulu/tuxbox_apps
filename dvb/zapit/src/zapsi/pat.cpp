/*
 * $Id: pat.cpp,v 1.19 2002/05/05 01:52:36 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "dmx.h"
#include "pat.h"

#define PAT_LENGTH 1024

int parse_pat (int demux_fd, CZapitChannel * channel)
{
	/* buffer for program association table */
	unsigned char buffer[PAT_LENGTH];

	/* number of read sections */
	unsigned char section = 0;

	/* current positon in buffer */
	unsigned short i;

	do
	{
		/* set filter for program association section */
		setDmxSctFilter(demux_fd, 0x0000, 0x00);

		/* read section */
		if (read(demux_fd, buffer, PAT_LENGTH) < 0)
		{
			perror("[pat.cpp] read");
    			return -1;
    		}

		/* loop over service id / program map table pid pairs */
		for (i = 8; i < (((buffer[1] & 0x0F) << 8) | buffer[2]) + 3; i += 4)
		{
			/* compare service id */
			if (channel->getServiceId() == ((buffer[i] << 8) | buffer[i+1]))
			{
				/* store program map table pid */
				channel->setPmtPid(((buffer[i+2] & 0x1F) << 8) | buffer[i+3]);
				return 0;
			}
		}
	}
	while (section++ != buffer[7]);

	return -1;
}

