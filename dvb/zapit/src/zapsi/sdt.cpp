/*
 * $Id: sdt.cpp,v 1.26 2002/08/24 11:10:53 obi Exp $
 */

/* system c */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* zapit */
#include <zapost/dmx.h>

#include "descriptors.h"
#include "sdt.h"

unsigned int get_sdt_TsidOnid ()
{
	int demux_fd;
	unsigned char buffer[1024];

	/* service_description_section elements */
	uint16_t transport_stream_id;
	uint16_t original_network_id;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x42;
	mask[0] = 0xFF;
	
	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[sdt.cpp] " DEMUX_DEV);
		return 0;
	}

	if (setDmxSctFilter(demux_fd, 0x0011, filter, mask) < 0)
	{
		close(demux_fd);
		return 0;
	}

	if (read(demux_fd, buffer, sizeof(buffer)) < 0)
	{
		perror("[sdt.cpp] read");
		close(demux_fd);
		return 0;
	}

	close(demux_fd);

	transport_stream_id = (buffer[3] << 8) | buffer[4];
	original_network_id = (buffer[8] << 8) | buffer[9];

	return ((transport_stream_id << 16) | original_network_id );
}

int parse_sdt ()
{
	int demux_fd;
	unsigned char buffer[1024];

	/* position in buffer */
	unsigned short pos;
	unsigned short pos2;

	/* service_description_section elements */
	unsigned short section_length;
	unsigned short transport_stream_id;
	unsigned short original_network_id;
	unsigned short service_id;
	unsigned short descriptors_loop_length;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x42;
	filter[4] = 0x00;
	mask[0] = 0xFF;
	mask[4] = 0xFF;

	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[sdt.cpp] " DEMUX_DEV);
		return -1;
	}

	do
	{
		if (setDmxSctFilter(demux_fd, 0x0011, filter, mask) < 0)
		{
			close(demux_fd);
			return -1;
		}

		if (read(demux_fd, buffer, sizeof(buffer)) < 0)
		{
			perror("[sdt.cpp] read");
			close(demux_fd);
			return -1;
		}

		section_length = ((buffer[1] & 0x0F) << 8) | buffer[2];
		transport_stream_id = (buffer[3] << 8) | buffer[4];
		original_network_id = (buffer[8] << 8) | buffer[9];

		for (pos = 11; pos < section_length - 1; pos += descriptors_loop_length + 5)
		{
			service_id = (buffer[pos] << 8) | buffer[pos + 1];
			descriptors_loop_length = ((buffer[pos + 3] & 0x0F) << 8) | buffer[pos + 4];

			for (pos2 = pos + 5; pos2 < pos + descriptors_loop_length + 5; pos2 += buffer[pos2 + 1] + 2)
			{
				switch (buffer[pos2])
				{
				case 0x0A:
					ISO_639_language_descriptor(buffer + pos2);
					break;

				case 0x40:
					network_name_descriptor(buffer + pos2);
					break;

				case 0x47:
					bouquet_name_descriptor(buffer + pos2);
					break;

				case 0x48:
					service_descriptor(buffer + pos2, service_id, transport_stream_id, original_network_id);
					break;

				case 0x49:
					country_availability_descriptor(buffer + pos2);
					break;

				case 0x4A:
					linkage_descriptor(buffer + pos2);
					break;

				case 0x4B:
					NVOD_reference_descriptor(buffer + pos2);
					break;

				case 0x4C:
					time_shifted_service_descriptor(buffer + pos2);
					break;

				case 0x53:
					CA_identifier_descriptor(buffer + pos2);
					break;

				case 0x5D:
					multilingual_service_name_descriptor(buffer + pos2);
					break;

				case 0x5F:
					private_data_specifier_descriptor(buffer + pos2);
					break;

				case 0x64:
					data_broadcast_descriptor(buffer + pos2);
					break;

				case 0x80: /* unknown, Eutelsat 13.0E */
					break;

				case 0x84: /* unknown, Eutelsat 13.0E */
					break;

				case 0x86: /* unknown, Eutelsat 13.0E */
					break;

				case 0x88: /* unknown, Astra 19.2E */
					break;

				case 0xB2: /* unknown, Eutelsat 13.0E */
					break;

				case 0xC0: /* unknown, Eutelsat 13.0E */
					break;

				case 0xE4: /* unknown, Astra 19.2E */
					break;

				case 0xE5: /* unknown, Astra 19.2E */
					break;

				case 0xE7: /* unknown, Eutelsat 13.0E */
					break;

				case 0xED: /* unknown, Astra 19.2E */
					break;

				case 0xF8: /* unknown, Astra 19.2E */
					break;

				case 0xF9: /* unknown, Astra 19.2E */
					break;

				default:
					printf("[sdt.cpp] descriptor_tag: %02x\n", buffer[pos2]);
					generic_descriptor(buffer + pos2);
					break;
				}
			}
		}
	}
	while (filter[4]++ != buffer[7]);

	close(demux_fd);
	return 0;
}

