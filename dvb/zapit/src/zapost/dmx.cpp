/*
 * $Id: dmx.cpp,v 1.14 2002/12/27 16:59:41 obi Exp $
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

#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <zapit/dmx.h>
#include <zapit/debug.h>

int setDmxSctFilter (int fd, unsigned short pid, unsigned char * filter, unsigned char * mask)
{
	struct dmx_sct_filter_params sctFilterParams;

	if (fd < 0)
		return -1;

	memset(&sctFilterParams.filter, 0, sizeof(struct dmx_filter));
	memcpy(&sctFilterParams.filter.filter, filter, DMX_FILTER_SIZE);
	memcpy(&sctFilterParams.filter.mask, mask, DMX_FILTER_SIZE);

	sctFilterParams.pid = pid;
	sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;

	switch (sctFilterParams.filter.filter[0]) {
	case 0x00: /* program_association_section */
		sctFilterParams.timeout = 2000;
		break;

	case 0x01: /* conditional_access_section */
		sctFilterParams.timeout = 6000;
		break;

	case 0x02: /* program_map_section */
		sctFilterParams.timeout = 1500;
		break;

	case 0x03: /* transport_stream_description_section */
		sctFilterParams.timeout = 10000;
		break;

	/* 0x04 - 0x3F: reserved */

	case 0x40: /* network_information_section - actual_network */
		sctFilterParams.timeout = 10000;
		break;

	case 0x41: /* network_information_section - other_network */
		sctFilterParams.timeout = 10000;
		break;

	case 0x42: /* service_description_section - actual_transport_stream */
		sctFilterParams.timeout = 10000;
		break;

	/* 0x43 - 0x45: reserved for future use */

	case 0x46: /* service_description_section - other_transport_stream */
		sctFilterParams.timeout = 10000;
		break;

	/* 0x47 - 0x49: reserved for future use */

	case 0x4A: /* bouquet_association_section */
		sctFilterParams.timeout = 11000;
		break;

	/* 0x4B - 0x4D: reserved for future use */

	case 0x4E: /* event_information_section - actual_transport_stream, present/following */
		sctFilterParams.timeout = 2000;
		break;

	case 0x4F: /* event_information_section - other_transport_stream, present/following */
		sctFilterParams.timeout = 10000;
		break;

	/* 0x50 - 0x5F: event_information_section - actual_transport_stream, schedule */
	/* 0x60 - 0x6F: event_information_section - other_transport_stream, schedule */

	case 0x70: /* time_date_section */
		sctFilterParams.timeout = 30000;
		break;

	case 0x71: /* running_status_section */
		sctFilterParams.timeout = 0;
		break;

	case 0x72: /* stuffing_section */
		sctFilterParams.timeout = 0;
		break;

	case 0x73: /* time_offset_section */
		sctFilterParams.timeout = 30000;
		break;

	/* 0x74 - 0x7D: reserved for future use */

	case 0x7E: /* discontinuity_information_section */
		sctFilterParams.timeout = 0;
		break;

	case 0x7F: /* selection_information_section */
		sctFilterParams.timeout = 0;
		break;

	/* 0x80 - 0x8F: ca_message_section */
	/* 0x90 - 0xFE: user defined */
	/*        0xFF: reserved */
	default:
		return -1;
	}

	if (ioctl(fd, DMX_SET_FILTER, &sctFilterParams) < 0)
	{
		ERROR("DMX_SET_FILTER");
		return -1;
	}

	return 0;
}

int setDmxPesFilter (int fd, dmx_output_t output, dmx_pes_type_t pes_type, unsigned short pid)
{
	dmx_pes_filter_params pesFilterParams;

	if (fd < 0)
		return -1;

	if ((pid <= 0x0001) && (pes_type != DMX_PES_PCR))
		return -1;

	if ((pid >= 0x0002) && (pid <= 0x0000F))
		return -1;

	if (pid >= 0x1FFF)
		return -1;

	pesFilterParams.pid = pid;
	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = output;
	pesFilterParams.pes_type = pes_type;
	pesFilterParams.flags = 0;

	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
	{
		ERROR("DMX_SET_PES_FILTER");
		return -1;
	}

	return 0;
}

int startDmxFilter (int fd)
{
	if (fd < 0)
		return -1;

	if (ioctl(fd, DMX_START) < 0)
	{
		ERROR("DMX_START");
		return -1;
	}

	return 0;
}

int stopDmxFilter (int fd)
{
	if (fd < 0)
		return -1;

	if (ioctl(fd, DMX_STOP) < 0)
	{
		ERROR("DMX_STOP");
		return -1;
	}

	return 0;
}

int readDmx(int fd, unsigned char * buf, const size_t n)
{
	int return_value = read(fd, buf, n);
	
	if (return_value < 0)
		ERROR("DMX_READ");

	return return_value;
}
