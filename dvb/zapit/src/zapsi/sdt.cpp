/*
 * $Id: sdt.cpp,v 1.19 2002/04/19 14:53:29 obi Exp $
 */

#include <fcntl.h>
#include <ost/dmx.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "sdt.h"
#include "descriptors.h"

#define DEMUX_DEV "/dev/ost/demux0"

int parse_sdt ()
{
	struct dmxSctFilterParams flt;
	int demux_fd;
	uint8_t buffer[1024];
	uint8_t section = 0;

	/* position in buffer */
	uint16_t pos;
	uint16_t pos2;

	/* service_description_section elements */
	uint16_t section_length;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	uint16_t service_id;
	uint16_t descriptors_loop_length;
  
	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
		perror("[sdt.cpp] " DEMUX_DEV);
		return -1;
	}

	memset (&flt.filter, 0, sizeof(struct dmxFilter));

	flt.pid = 0x0011;
	flt.filter.filter[0] = 0x42;
	flt.filter.mask[0] = 0xFF;
	flt.timeout = 10000;		/* 10 sec max. accord. ETSI (2002-04-04 rasc) */
	flt.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;

	if (ioctl(demux_fd, DMX_SET_FILTER, &flt) < 0)
	{
		perror("[sdt.cpp] DMX_SET_FILTER");
		close(demux_fd);
		return -1;
	}

	do
	{
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

				case 0x5F:
					private_data_specifier_descriptor(buffer + pos2);
					break;

				case 0x64:
					data_broadcast_descriptor(buffer + pos2);
					break;

				default:
					printf("[sdt.cpp] descriptor_tag: %02x\n", buffer[pos2]);
					generic_descriptor(buffer + pos2);
					break;
				}
			}
		}
	}
	while (section++ != buffer[7]);
 
	close(demux_fd);
	return 0;
}

