/*
 * $Id: dmx.cpp,v 1.20 2009/09/30 17:34:20 seife Exp $
 *
 * (C) 2002-2003 Andreas Oberritter <obi@tuxbox.org>
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

#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <zapit/dmx.h>
#include <zapit/debug.h>
#include <zapit/settings.h>

CDemux::CDemux(void)
{
	if ((fd = open(DEMUX_DEVICE, O_RDWR)) < 0)
		ERROR(DEMUX_DEVICE);
}

CDemux::~CDemux(void)
{
	if (fd >= 0)
		close(fd);
}

int CDemux::sectionFilter(const unsigned short pid, const unsigned char * const filter, const unsigned char * const mask)
{
	struct dmx_sct_filter_params sctFilterParams;

	fop(ioctl, DMX_STOP);
	memset(&sctFilterParams, 0, sizeof(struct dmx_sct_filter_params));
#ifndef HAVE_TRIPLEDRAGON
#define XPDF_NO_CRC 0
	memcpy(&sctFilterParams.filter.filter, filter, DMX_FILTER_SIZE);
	memcpy(&sctFilterParams.filter.mask, mask, DMX_FILTER_SIZE);
#else
	sctFilterParams.filter[0] = filter[0];
	sctFilterParams.mask[0] = mask[0];
	/* we'll lose the latest 2 bytes of filter and mask here, but those are not really used anyway */
	memcpy(&sctFilterParams.filter[3], filter + 1, DMX_FILTER_SIZE - 2 - 1);
	memcpy(&sctFilterParams.mask[3], mask + 1, DMX_FILTER_SIZE - 2 - 1);
	sctFilterParams.filter_length = DMX_FILTER_SIZE;
#endif
	sctFilterParams.pid = pid;
	sctFilterParams.flags = DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START;

	switch (filter[0]) {
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
		sctFilterParams.timeout = 15000;
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
		sctFilterParams.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
		sctFilterParams.flags  |= (XPDF_NO_CRC); /* section has no CRC */
		sctFilterParams.pid     = 0x0014;
		sctFilterParams.timeout = 30000;
		break;

	case 0x71: /* running_status_section */
		sctFilterParams.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
		sctFilterParams.flags  |= (XPDF_NO_CRC); /* section has no CRC */
		sctFilterParams.timeout = 0;
		break;

	case 0x72: /* stuffing_section */
		sctFilterParams.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
		sctFilterParams.flags  |= (XPDF_NO_CRC); /* section has no CRC */
		sctFilterParams.timeout = 0;
		break;

	case 0x73: /* time_offset_section */
		sctFilterParams.pid     = 0x0014;
		sctFilterParams.timeout = 30000;
		break;

	/* 0x74 - 0x7D: reserved for future use */

	case 0x7E: /* discontinuity_information_section */
		sctFilterParams.flags  &= (~DMX_CHECK_CRC); /* section has no CRC */
		sctFilterParams.flags  |= (XPDF_NO_CRC); /* section has no CRC */
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

	return fop(ioctl, DMX_SET_FILTER, &sctFilterParams);
}

int CDemux::pesFilter(const unsigned short pid, const dmx_output_t output, const dmx_pes_type_t pes_type)
{
	struct dmx_pes_filter_params pesFilterParams;

	if ((pid <= 0x0001) && (pes_type != DMX_PES_PCR))
		return -1;

	if (((pid >= 0x0002) && (pid <= 0x0000F)) || (pid >= 0x1FFF))
		return -1;

	memset(&pesFilterParams, 0, sizeof(struct dmx_pes_filter_params));
	
	pesFilterParams.pid = pid;
#ifndef HAVE_TRIPLEDRAGON
	pesFilterParams.input = DMX_IN_FRONTEND;
#endif
	pesFilterParams.output = output;
	pesFilterParams.pes_type = pes_type;

	return fop(ioctl, DMX_SET_PES_FILTER, &pesFilterParams);
}

int CDemux::start(void)
{
	return fop(ioctl, DMX_START);
}

int CDemux::stop(void)
{
	return fop(ioctl, DMX_STOP);
}

int CDemux::read(unsigned char * const buf, const size_t n)
{
	return fop(read, buf, n);
}

