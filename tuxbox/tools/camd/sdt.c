/*
 * $Id: sdt.c,v 1.4 2002/11/03 02:18:27 obi Exp $
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

#ifdef LOOKUP_ONID

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/dvb/dmx.h>

#include "sdt.h"

#define SDT_SIZE 10
#define DEMUX_DEV "/dev/dvb/adapter0/demux0"

/*
 * parse service description table
 * return original network id on success
 * return zero on failure
 */

unsigned short parse_sdt ()
{
	unsigned char buf[SDT_SIZE];
	int dmx_fd;

	struct dmx_sct_filter_params sctFilterParams;

	memset(&sctFilterParams, 0x00, sizeof(struct dmx_sct_filter_params));

	sctFilterParams.filter.filter[0] = 0x42;
	sctFilterParams.filter.mask[0] = 0xFF;
	sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
	sctFilterParams.pid = 0x0011;
	sctFilterParams.timeout = 10000;

	if ((dmx_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[sdt.c] " DEMUX_DEV);
		return 0;
	}

	if (ioctl(dmx_fd, DMX_SET_FILTER, &sctFilterParams) < 0)
	{
		perror("[sdt.c] DMX_SET_FILTER");
		close(dmx_fd);
		return 0;
	}

	if (read(dmx_fd, buf, sizeof(buf)) < 0)
	{
		perror("[sdt.c] read");
		close(dmx_fd);
		return 0;
	}

	close(dmx_fd);

	return ((buf[8] << 8) | buf[9]);
}

#endif /* LOOKUP_ONID */
