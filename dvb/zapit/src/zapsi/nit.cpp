/*
 * $Id: nit.cpp,v 1.36 2004/04/17 19:44:04 derget Exp $
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

/* system c++ */
#include <map>

/* zapit */
#include <zapit/debug.h>
#include <zapit/descriptors.h>
#include <zapit/dmx.h>
#include <zapit/getservices.h>
#include <zapit/nit.h>

#define NIT_SIZE 1024

extern std::map<unsigned int, transponder> transponders;

int parse_nit(unsigned char DiSEqC)
{
	CDemux dmx;

	unsigned char buffer[NIT_SIZE];

	/* position in buffer */
	unsigned short pos;
	unsigned short pos2;

	/* network_information_section elements */
	unsigned short section_length;
	unsigned short network_descriptors_length;
	unsigned short transport_descriptors_length;
	unsigned short transport_stream_loop_length;
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	unsigned short network_id;

	unsigned char filter[DMX_FILTER_SIZE];
	unsigned char mask[DMX_FILTER_SIZE];

	memset(filter, 0x00, DMX_FILTER_SIZE);
	memset(mask, 0x00, DMX_FILTER_SIZE);

	filter[0] = 0x40;
	filter[4] = 0x00;
	mask[0] = 0xFF;
	mask[4] = 0xFF;

	do {
		if ((dmx.sectionFilter(0x10, filter, mask) < 0) || (dmx.read(buffer, NIT_SIZE) < 0))
			return -1;

		section_length = ((buffer[1] & 0x0F) << 8) + buffer[2];
		network_id = ((buffer[3] << 8)| buffer [4]);
		network_descriptors_length = ((buffer[8] & 0x0F) << 8) | buffer[9];

		for (pos = 10; pos < network_descriptors_length + 10; pos += buffer[pos + 1] + 2)
		{
			switch (buffer[pos])
			{
/*			case 0x0F:
				Private_data_indicator_descriptor(buffer + pos);
				break;
*/
			case 0x40:
				network_name_descriptor(buffer + pos);
				break;

			case 0x4A:
				linkage_descriptor(buffer + pos);
				break;

			case 0x5B:
				multilingual_network_name_descriptor(buffer + pos);
				break;

/*			case 0x5F:
				private_data_specifier_descriptor(buffer + pos);
				break;
*/
			case 0x80: /* unknown, Eutelsat 13.0E */
				break;

			case 0x90: /* unknown, Eutelsat 13.0E */
				break;

			default:
				DBG("first_descriptor_tag: %02x", buffer[pos]);
				break;
			}
		}

		transport_stream_loop_length = ((buffer[pos] & 0x0F) << 8) | buffer[pos + 1];

		if (!transport_stream_loop_length)
			continue;

		for (pos += 2; pos < section_length - 3; pos += transport_descriptors_length + 6)
		{
			transport_stream_id = (buffer[pos] << 8) | buffer[pos + 1];
			original_network_id = (buffer[pos + 2] << 8) | buffer[pos + 3];
			transport_descriptors_length = ((buffer[pos + 4] & 0x0F) << 8) | buffer[pos + 5];

			if (transponders.find((transport_stream_id << 16) | original_network_id) == transponders.end())
			{
				for (pos2 = pos + 6; pos2 < pos + transport_descriptors_length + 6; pos2 += buffer[pos2 + 1] + 2)
				{
					switch (buffer[pos2])
					{
					case 0x41:
						service_list_descriptor(buffer + pos2, transport_stream_id, original_network_id);
						break;

					case 0x42:
						stuffing_descriptor(buffer + pos2);
						break;

					case 0x43:
						if (satellite_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id, DiSEqC) < 0)
							return -2;
						break;

					case 0x44:
						if (cable_delivery_system_descriptor(buffer + pos2, transport_stream_id, original_network_id) < 0)
							return -2;
						break;

					case 0x5A:
						if (terrestrial_delivery_system_descriptor(buffer + pos2) < 0)
							return -2;
						break;

					case 0x5F:
						private_data_specifier_descriptor(buffer + pos2);
						break;

					case 0x62:
						frequency_list_descriptor(buffer + pos2);
						break;

					case 0x82: /* unknown, Eutelsat 13.0E */
						break;

					default:
						DBG("second_descriptor_tag: %02x", buffer[pos2]);
						break;
					}
				}
			}
		}
	} while (filter[4]++ != buffer[7]);

	return 0;
}

