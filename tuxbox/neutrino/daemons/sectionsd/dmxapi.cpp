/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/daemons/sectionsd/dmxapi.cpp,v 1.7 2009/10/22 20:48:22 seife Exp $
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

#include <stdio.h>         /* perror      */
#include <string.h>        /* memset      */
#include <sys/ioctl.h>     /* ioctl       */
#include <fcntl.h>         /* open        */
#include <unistd.h>        /* close, read */

bool setfilter(const int fd, const uint16_t pid, const uint8_t filter, const uint8_t mask, const uint32_t flags)
{
	struct dmx_sct_filter_params flt;

	memset(&flt, 0, sizeof(struct dmx_sct_filter_params));

	flt.pid              = pid;
#ifndef HAVE_TRIPLEDRAGON
	flt.filter.filter[0] = filter;
	flt.filter.mask  [0] = mask;
#else
	flt.filter[0] = filter;
	flt.mask[0] = mask;
	flt.filter_length = 1 + 2;
#endif
	flt.timeout          = 0;
	flt.flags            = flags;

	if (::ioctl(fd, DMX_SET_FILTER, &flt) == -1)
	{
		perror("[sectionsd] DMX: DMX_SET_FILTER");
		return false;
	}
	return true;
}

struct SI_section_TOT_header
{
	unsigned char      table_id                 :  8;
	unsigned char      section_syntax_indicator :  1;
	unsigned char      reserved_future_use      :  1;
	unsigned char      reserved1                :  2;
	unsigned short     section_length           : 12;
	unsigned long long UTC_time                 : 40;
	unsigned char      reserved2                :  4;
	unsigned short     descriptors_loop_length  : 12;
}
__attribute__ ((packed)); /* 10 bytes */

struct SI_section_TDT_header
{
	unsigned char      table_id                 :  8;
	unsigned char      section_syntax_indicator :  1;
	unsigned char      reserved_future_use      :  1;
	unsigned char      reserved1                :  2;
	unsigned short     section_length           : 12;
/*	unsigned long long UTC_time                 : 40;*/
	UTC_t              UTC_time;
}
__attribute__ ((packed)); /* 8 bytes */


bool getUTC(UTC_t * const UTC, const bool TDT)
{
	int fd;
	struct dmx_sct_filter_params flt;
	struct SI_section_TDT_header tdt_tot_header;
	char cUTC[5];

	if ((fd = ::open(DEMUX_DEVICE, O_RDWR)) < 0)
	{
		perror("[sectionsd] getUTC: open");
		return false;
	}

	memset(&flt, 0, sizeof(struct dmx_sct_filter_params));

	flt.pid              = 0x0014;
#ifndef HAVE_TRIPLEDRAGON
	flt.filter.filter[0] = TDT ? 0x70 : 0x73;
	flt.filter.mask  [0] = 0xFF;
#else
	flt.filter[0] = TDT ? 0x70 : 0x73;
	flt.mask  [0] = 0xFF;
	flt.filter_length = 1 + 2;
#endif
	flt.timeout          = 31000;
	flt.flags            = TDT ? (DMX_ONESHOT | DMX_IMMEDIATE_START | XPDF_NO_CRC) : (DMX_ONESHOT | DMX_CHECK_CRC | DMX_IMMEDIATE_START);

	if (::ioctl(fd, DMX_SET_FILTER, &flt) == -1)
	{
		perror("[sectionsd] getUTC: set filter");
		::close(fd);
		return false;
	}

	if (::read(fd, &tdt_tot_header, sizeof(tdt_tot_header)) != sizeof(tdt_tot_header))
	{
		perror("[sectionsd] getUTC: read");
		::close(fd);
		return false;
	}

	memcpy(cUTC, &tdt_tot_header.UTC_time, 5);
	if ((cUTC[2] > 0x23) || (cUTC[3] > 0x59) || (cUTC[4] > 0x59)) // no valid time
	{
		printf("[sectionsd] getUTC: invalid %s section received: %02x %02x %02x %02x %02x\n", 
			TDT ? "TDT" : "TOT", cUTC[0], cUTC[1], cUTC[2], cUTC[3], cUTC[4]);
		::close(fd);
		return false;
	}

	(*UTC) = tdt_tot_header.UTC_time;

	::close(fd);
	return true;
}
