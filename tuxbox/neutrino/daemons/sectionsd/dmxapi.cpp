/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/daemons/sectionsd/dmxapi.cpp,v 1.1 2003/02/06 17:52:18 thegoodguy Exp $
 *
 * DMX low level functions (sectionsd) - d-box2 linux project
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
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


#include <dmxapi.h>

#include <stdio.h>         /* perror */
#include <string.h>        /* memset */
#include <sys/ioctl.h>     /* ioctl  */

bool setfilter(const int fd, const uint16_t pid, const uint8_t filter, const uint8_t mask, const uint32_t flags)
{
	struct dmx_sct_filter_params flt;

	memset(&flt.filter, 0, sizeof(struct dmx_filter));

	flt.pid              = pid;
	flt.filter.filter[0] = filter;
	flt.filter.mask  [0] = mask;
	flt.timeout          = 0;
	flt.flags            = flags;

	if (ioctl(fd, DMX_SET_FILTER, &flt) == -1)
	{
		perror("[sectionsd] DMX: DMX_SET_FILTER");
		return false;
	}
	return true;
}
