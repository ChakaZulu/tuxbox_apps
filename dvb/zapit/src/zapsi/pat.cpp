/*
 * $Id: pat.cpp,v 1.24 2002/05/13 17:17:05 obi Exp $
 *
 * (C) 2002 by Andreas Oberritter <obi@tuxbox.org> jaja :)
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

#include <clientlib/zapitclient.h>
#include <scan.h>
#include <zapost/dmx.h>

#include "pat.h"

#define PAT_LENGTH 1024

extern CEventServer * eventServer;
extern unsigned int found_transponders;

int fake_pat (unsigned short onid, FrontendParameters feparams)
{
	unsigned short tsid;
	int demux_fd;

	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[pat.cpp] " DEMUX_DEV);
		return -1;
	}

	/* buffer for program association table */
	unsigned char buffer[PAT_LENGTH];

	/* set filter for program association section */
	if (setDmxSctFilter(demux_fd, 0x0000, 0x00) < 0)
	{
		return -1;
	}

	/* read section */
	if (read(demux_fd, buffer, PAT_LENGTH) < 0)
	{
		perror("[pat.cpp] read");
		close(demux_fd);
		return -1;
	}

	tsid = (buffer[3]<<8)|buffer[4];

	if (scantransponders.count((tsid << 16) | onid) == 0)
	{
		found_transponders++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CEventServer::INITID_ZAPIT,
			&found_transponders,
			sizeof(found_transponders)
		);

		scantransponders.insert
		(
			std::pair <unsigned int, transpondermap>
			(
				(tsid << 16) | onid,
				transpondermap
				(
					tsid,
					onid,
					feparams,
					0,
					0
				)
			)
		);
	}

	close(demux_fd);
	return 0;
}

int parse_pat (int demux_fd, CZapitChannel * channel)
{
	/* buffer for program association table */
	unsigned char buffer[PAT_LENGTH];

	/* number of read sections */
	unsigned char section = 0;

	/* current positon in buffer */
	unsigned short i;

	/* set filter for program association section */
	if (setDmxSctFilter(demux_fd, 0x0000, 0x00) < 0)
	{
		return -1;
	}

	do
	{
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

