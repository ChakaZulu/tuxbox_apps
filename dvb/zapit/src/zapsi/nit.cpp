/*
 * $Id: nit.cpp,v 1.16 2002/04/21 22:05:40 obi Exp $
 */

#include <fcntl.h>
#include <ost/dmx.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "descriptors.h"
#include "nit.h"
#include "getservices.h"

#define DEMUX_DEV "/dev/ost/demux0"

extern std::map <uint32_t, transponder> transponders;

int parse_nit (uint8_t DiSEqC)
{
	struct dmxSctFilterParams flt;
	int demux_fd;
  	uint8_t buffer[1024];
	uint8_t section = 0;

	/* position in buffer */
	uint16_t pos;
	uint16_t pos2;

	/* network_information_section elements */
	uint16_t section_length;
	uint16_t network_descriptors_length;
	uint16_t transport_descriptors_length;
	uint16_t transport_stream_loop_length;
	uint16_t transport_stream_id;
	uint16_t original_network_id;
  
  	if ((demux_fd = open(DEMUX_DEV, O_RDWR)) < 0)
	{
    		perror("[nit.cpp] " DEMUX_DEV);
    		return -1;
  	}

  	memset (&flt.filter, 0, sizeof (struct dmxFilter));
  
  	flt.pid = 0x0010;
  	flt.filter.filter[0] = 0x40;
    	flt.filter.mask[0]  = 0xFF;
  	flt.timeout = 10000;		/* 10 Sec. Max. for NIT (ETSI)   (2002-04-04 rasc) */
  	flt.flags = DMX_CHECK_CRC | DMX_IMMEDIATE_START;
 
  	if (ioctl(demux_fd, DMX_SET_FILTER, &flt) < 0)
	{
    		perror("[nit.cpp] DMX_SET_FILTER");
  	}

	do
	{
		if ((read(demux_fd, buffer, sizeof(buffer))) < 0)
		{
			perror("[nit.cpp] read");
			close(demux_fd);
			return -1;
		}

		section_length = ((buffer[1] & 0x0F) << 8) + buffer[2];
		network_descriptors_length = ((buffer[8] & 0x0F) << 8) | buffer[9];

		for (pos = 10; pos < network_descriptors_length + 10; pos += buffer[pos + 1] + 2)
		{
			switch (buffer[pos])
			{
			case 0x0F:
				Private_data_indicator_descriptor(buffer + pos);
				break;

			case 0x40:
				network_name_descriptor(buffer + pos);
				break;

			case 0x4A:
				linkage_descriptor(buffer + pos);
				break;

			case 0x5F:
				private_data_specifier_descriptor(buffer + pos);
				break;

			case 0x80: /* unknown, Eutelsat 13.0E */
				break;

			case 0x90: /* unknown, Eutelsat 13.0E */
				break;

			default:
				printf("[nit.cpp] descriptor_tag (a): %02x\n", buffer[pos]);
				break;
			}
		}

		transport_stream_loop_length = ((buffer[pos] & 0x0F) << 8) | buffer[pos + 1];

		for (pos += 2; pos < section_length - 3; pos += transport_descriptors_length + 6)
		{
			transport_stream_id = (buffer[pos] << 8) | buffer[pos + 1];
			original_network_id = (buffer[pos + 2] << 8) | buffer[pos + 3];
			transport_descriptors_length = ((buffer[pos + 4] & 0x0F) << 8) | buffer[pos + 5];

			if (transponders.count((transport_stream_id << 16) | original_network_id) == 0)
			{
				for (pos2 = pos + 6; pos2 < pos + transport_descriptors_length + 6; pos2 += buffer[pos2 + 1] + 2)
				{
					switch (buffer[pos2])
					{
					case 0x41:
						service_list_descriptor(buffer + pos2);
						break;

					case 0x43:
						satellite_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id, DiSEqC);
						break;

					case 0x44:
						cable_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id);
						break;

					case 0x5F:
						private_data_specifier_descriptor(buffer + pos2);
						break;

					case 0x82: /* unknown, Eutelsat 13.0E */
						break;

					default:
						printf("[nit.cpp] descriptor_tag (b): %02x\n", buffer[pos2]);
						break;
					}
				}
			}
		}
	}
	while (section++ != buffer[7]);

	close(demux_fd);
	return 0;
}

