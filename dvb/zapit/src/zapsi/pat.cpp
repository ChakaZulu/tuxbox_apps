/*
 * $Id: pat.cpp,v 1.27 2002/07/22 15:00:50 Homar Exp $
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
static int status = 0;

int fake_pat (uint32_t TsidOnid, FrontendParameters feparams, uint8_t polarity, uint8_t DiSEqC)
{
	if ((status = scantransponders.count(TsidOnid)) == 0)
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
				TsidOnid,
				transpondermap
				(
					(TsidOnid >> 16),
					TsidOnid,
					feparams,
					polarity,
					DiSEqC
				)
			)
		);
	}

	return status;
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
	if ((status = setDmxSctFilter(demux_fd, 0x0000, 0x00)) < 0)
	{
		return status;
	}

	do
	{
		/* read section */
		if ((status = read(demux_fd, buffer, PAT_LENGTH)) < 0)
		{
			perror("[pat.cpp] read");
			return status;
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

	return status;
}

