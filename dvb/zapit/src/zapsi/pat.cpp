/*
 * $Id: pat.cpp,v 1.42 2002/11/27 03:42:36 obi Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <zapit/dmx.h>
#include <zapit/debug.h>
#include <zapit/pat.h>

#define PAT_SIZE 1024

int parse_pat (const int demux_fd, CZapitChannel * channel)
{
	/* buffer for program association table */
	unsigned char buffer[PAT_SIZE];

	/* current positon in buffer */
	unsigned short i;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	if (!channel)
		return -1;

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	mask[0] = 0xFF;
	mask[4] = 0xFF;

	do
	{
		/* set filter for program association section */
		if (setDmxSctFilter(demux_fd, 0x0000, filter, mask) < 0)
			return -1;

		/* read section */
		if (read(demux_fd, buffer, PAT_SIZE) < 0)
		{
			ERROR("read");
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
	while (filter[4]++ != buffer[7]);

	return -1;
}

