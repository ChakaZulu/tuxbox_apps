/*
 * $Id: dmx.cpp,v 1.2 2002/07/17 03:28:28 obi Exp $
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

#include "dmx.h"

int setDmxSctFilter (int fd, dvb_pid_t pid, uint8_t filter0, uint8_t filter1, uint8_t filter2)
{
	dmxSctFilterParams sctFilterParams;

	memset(&sctFilterParams.filter.filter, 0x00, DMX_FILTER_SIZE);
	memset(&sctFilterParams.filter.mask, 0x00, DMX_FILTER_SIZE);

	sctFilterParams.filter.filter[0] = filter0;
	sctFilterParams.filter.filter[1] = filter1;
	sctFilterParams.filter.filter[2] = filter2;
	sctFilterParams.pid = pid;

	switch (sctFilterParams.pid)
	{
	case 0x0000: /* Program Association Table */
		sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0001: /* Conditional Access Table */
		sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0002: /* Transport Streams Description Table */
		sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0003 ... 0x000F: /* reserved */
		return -1;

	case 0x0010: /* Network Information Table, Stuffing Table */
		sctFilterParams.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0011: /* Service Description Table, Bouquet Association Table, Stuffing Table */
		sctFilterParams.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0012: /* Event Information Table, Stuffing Table */
		sctFilterParams.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0013: /* Running Status Table, Stuffing Table */
		sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0014: /* Time and Date Table, Time Offset Table, Stuffing Table */
		sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;

	case 0x0015: /* network synchronization */
		return -1;

	case 0x0016 ... 0x001D: /* reserved */
		return -1;

	case 0x001E: /* Discontinuity Information Table */
		return -1;

	case 0x001F: /* Selection Information Table */
		return -1;

	case 0x0020 ... 0x1FFB:
		sctFilterParams.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
		break;
	
	case 0x1FFC: /* ATSC SI */
		return -1;

	case 0x1FFD: /* ATSC Master Program Guide */
		return -1;

	case 0x1FFE:
		return -1;

	case 0x1FFF: /* reserved */
		return -1;

	default:
		break;
	}

	switch (sctFilterParams.filter.filter[0])
	{
	case 0x00: /* program_association_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 2000;
		break;

	case 0x01: /* conditional_access_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 6000;
		break;

	case 0x02: /* program_map_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.filter.mask[1] = 0xFF;
		sctFilterParams.filter.mask[2] = 0xFF;
		sctFilterParams.timeout = 1500;
		break;

	case 0x03: /* transport_stream_description_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 10000;
		break;

	case 0x04 ... 0x3F: /* reserved */
		return -1;

	case 0x40: /* network_information_section - actual_network */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 10000;
		break;

	case 0x41: /* network_information_section - other_network */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 10000;
		break;

	case 0x42: /* service_description_section - actual_transport_stream */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 10000;
		break;

	case 0x43 ... 0x45: /* reserved for future use */
		return -1;

	case 0x46: /* service_description_section - other_transport_stream */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 10000;
		break;

	case 0x47 ... 0x49: /* reserved for future use */
		return -1;

	case 0x4A: /* bouquet_association_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 10000;
		break;

	case 0x4B ... 0x4D: /* reserved for future use */
		return -1;

	case 0x4E: /* event_information_section - actual_transport_stream, present/following */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 2000;
		break;

	case 0x4F: /* event_information_section - other_transport_stream, present/following */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 10000;
		break;

	case 0x50 ... 0x5F: /* event_information_section - actual_transport_stream, schedule */
		sctFilterParams.filter.mask[0] = 0xF0;
		sctFilterParams.timeout = 10000;
		break;

	case 0x60 ... 0x6F: /* event_information_section - other_transport_stream, schedule */
		sctFilterParams.filter.mask[0] = 0xF0;
		sctFilterParams.timeout = 10000;
		break;

	case 0x70: /* time_date_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 30000;
		break;

	case 0x71: /* running_status_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 0;
		break;

	case 0x72: /* stuffing_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 0;
		break;

	case 0x73: /* time_offset_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 30000;
		break;

	case 0x74 ... 0x7D: /* reserved for future use */
		return -1;

	case 0x7E: /* discontinuity_information_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 0;
		break;

	case 0x7F: /* selection_information_section */
		sctFilterParams.filter.mask[0] = 0xFF;
		sctFilterParams.timeout = 0;
		break;

	case 0x80 ... 0x8F: /* ca_message_section */
		sctFilterParams.filter.mask[0] = 0xF0;
		sctFilterParams.timeout = 1000;
		break;

	case 0x90 ... 0xFE: /* user defined */
		return -1;

	case 0xFF: /* reserved */
		return -1;
	}

	if (ioctl(fd, DMX_SET_FILTER, &sctFilterParams) < 0)
	{
		perror("[dmx.cpp] DMX_SET_FILTER");
		return -1;
	}

	return 0;
}

int setDmxPesFilter (int fd, dmxOutput_t output, dmxPesType_t pesType, dvb_pid_t pid)
{
	dmxPesFilterParams pesFilterParams;

	if ((pid < 0x0020) || (pid > 0x1FFB))
	{
		return -1;
	}

	pesFilterParams.pid = pid;
	pesFilterParams.input = DMX_IN_FRONTEND;
	pesFilterParams.output = output;
	pesFilterParams.pesType = pesType;
	pesFilterParams.flags = DMX_IMMEDIATE_START;

	if (ioctl(fd, DMX_SET_PES_FILTER, &pesFilterParams) < 0)
	{
		perror("[dmx.cpp] DMX_SET_PES_FILTER");
		return -1;
	}

	return 0;
}

int unsetDmxFilter (int fd)
{
	if (ioctl(fd, DMX_STOP) < 0)
	{
		return -1;
	}

	return 0;
}

