/*
 * $Id: bat.cpp,v 1.2 2002/05/12 01:56:19 obi Exp $
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

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* debug */
#include <stdlib.h>

#include <zapost/dmx.h>

#include "bat.h"
#include "descriptors.h"

#define DEMUX_DEV "/dev/ost/demux0"

int parse_bat (int demux_fd)
{
	uint8_t buffer[1024];
	uint8_t section = 0;

	/* position in buffer */
	uint16_t pos;
	uint16_t pos2;
	uint16_t pos3;

	/* bouquet_association_section elements */
	uint16_t section_length;
	uint16_t bouquet_id;
	uint16_t bouquet_descriptors_length;
	uint16_t transport_stream_loop_length;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t transport_descriptors_length;

	do
	{
		if (setDmxSctFilter(demux_fd, 0x0011, 0x4A) < 0)
		{
			return -1;
		}

		if (read(demux_fd, buffer, sizeof(buffer)) < 0)
		{
			perror("[bat.cpp] read");
			return -1;
		}

		section_length = ((buffer[1] & 0x0F) << 8) | buffer[2];
		bouquet_id = (buffer[3] << 8) | buffer[4];
		bouquet_descriptors_length = ((buffer[8] & 0x0F) << 8) | buffer[9];

		printf("[bat.cpp] dump:\n");
		for (pos = 0; pos < section_length + 3; pos++)
		{
			printf("%02x ", buffer[pos]);
		}
		printf("\n");

		for (pos = 10; pos < bouquet_descriptors_length + 10; pos += buffer[pos + 1] + 2)
		{
			switch (buffer[pos])
			{
			case 0x47:
				bouquet_name_descriptor(buffer + pos);
				break;

			case 0x4A:
				linkage_descriptor(buffer + pos);
				break;

			case 0x5F:
				private_data_specifier_descriptor(buffer + pos);
				break;

			default:
				printf("[bat.cpp] descriptor_tag (a): %02x\n", buffer[pos]);
				generic_descriptor(buffer + pos);
				break;
			}
		}

		transport_stream_loop_length = ((buffer[pos] & 0x0F) << 8) | buffer[pos + 1];
		printf("[bat.cpp] transport_stream_loop_length: %04x\n", transport_stream_loop_length);

		for (pos2 = pos + 2; pos2 < pos + 2 + transport_stream_loop_length; pos += transport_descriptors_length + 6)
		{
			transport_stream_id = (buffer[pos2] << 8) | buffer[pos2 + 1];
			original_network_id = (buffer[pos2 + 2] << 8) | buffer[pos2 + 3];
			transport_descriptors_length = ((buffer[pos2 + 4] & 0x0F) << 8) | buffer[pos2 + 5];

			printf("[bat.cpp] transport_descriptors_length: %04x\n", transport_descriptors_length);

			for (pos3 = pos2 + 6; pos3 < transport_descriptors_length + pos2 + 6; pos3 += buffer[pos3 + 1] + 2)
			{
				switch (buffer[pos3])
				{
				case 0x41:
					service_list_descriptor(buffer + pos3);
					break;

				default:
					printf("[bat.cpp] descriptor_tag (b): %02x\n", buffer[pos3]);
					generic_descriptor(buffer + pos3);
					break;
				}
			}
		}
	}
	while (section++ != buffer[7]);

	return 0;
}

