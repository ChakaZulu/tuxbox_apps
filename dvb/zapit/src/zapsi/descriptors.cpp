/*
 * $Id: descriptors.cpp,v 1.23 2002/04/19 14:53:29 obi Exp $
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
#include <map>
#include <string>

#include "descriptors.h"
#include "frontend.h"
#include "sdt.h"
#include "scan.h"
#include "zapitclient.h"

std::map <uint32_t, transpondermap> scantransponders;
std::map <uint32_t, scanchannel> scanchannels;
multimap <std::string, bouquet_mulmap> scanbouquets;
std::string curr_chan_name;
uint32_t found_transponders;
uint32_t found_channels;
std::string lastProviderName;

std::string charToXML (uint8_t character)
{
	std::string returnString;

	switch (character)
	{
	case 0x00 ... 0x1F:
		return "";
	case 0x20 ... 0x21:
		return returnString += character;
	case 0x22:
		return "&quot;";
	case 0x23 ... 0x25:
		return returnString += character;
	case 0x26:
		return "&amp;";
	case 0x27 ... 0x3B:
		return returnString += character;
	case 0x3C:
		return "&lt;";
	case 0x3D ... 0x7F:
		return returnString += character;
	case 0x80 ... 0x82:
	case 0x86 ... 0x87:
	case 0x8a:
		return "";
	default:
		char val[7];
		sprintf(val, "&#%d;", character);
		return val;
	}
}

uint8_t generic_descriptor (uint8_t *buffer)
{
	int i;

	printf("[descriptors.cpp] generic descriptor dump:\n");
	for (i = 0; i < buffer[1] + 2; i++)
	{
		printf(" %02x", buffer[i]);
	}
	printf("\n");

	return buffer[1];
}

/* 0x02 */
uint8_t video_stream_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x03 */
uint8_t audio_stream_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x04 */
uint8_t hierarchy_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x05 */
uint8_t registration_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x06 */
uint8_t data_stream_alignment_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x07 */
uint8_t target_background_grid_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x08 */
uint8_t Video_window_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x09 */
uint8_t CA_descriptor (uint8_t *buffer, uint16_t ca_system_id, uint16_t* ca_pid)
{
	if ((((buffer[2] & 0x1F) << 8) | buffer[3]) == ca_system_id)
	{
		*ca_pid = ((buffer[4] & 0x1F) << 8) | buffer[5];
	}
	else
	{
		*ca_pid = INVALID;
	}

	return buffer[1];
}

/* 0x0A */
uint8_t ISO_639_language_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x0B */
uint8_t System_clock_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x0C */
uint8_t Multiplex_buffer_utilization_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x0D */
uint8_t Copyright_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x0E */
uint8_t Maximum_bitrate_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x0F */
uint8_t Private_data_indicator_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/*
 * more to come...
 */

/* 0x40 */
uint8_t network_name_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x41 */
uint8_t service_list_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x42 */
uint8_t stuffing_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x43 */
uint8_t satellite_delivery_system_descriptor (uint8_t *buffer, uint16_t transport_stream_id, uint16_t original_network_id, uint8_t DiSEqC)
{
	FrontendParameters feparams;
	uint8_t polarization;

	feparams.Frequency =
	(
		((buffer[2] >> 4)	* 100000000) +
		((buffer[2] & 0x0F)	* 10000000) +
		((buffer[3] >> 4)	* 1000000) +
		((buffer[3] & 0x0F)	* 100000) +
		((buffer[4] >> 4)	* 10000) +
		((buffer[4] & 0x0F)	* 1000) +
		((buffer[5] >> 4)	* 100) +
		((buffer[5] & 0x0F)	* 10)
	);

	feparams.Inversion = INVERSION_AUTO;

	feparams.u.qpsk.SymbolRate =
	(
		((buffer[9] >> 4)	* 100000000) +
		((buffer[9] & 0x0F)	* 10000000) +
		((buffer[10] >> 4)	* 1000000) +
		((buffer[10] & 0x0F)	* 100000) +
		((buffer[11] >> 4)	* 10000) +
		((buffer[11] & 0x0F)	* 1000) +
		((buffer[12] >> 4)	* 100)
	);

	feparams.u.qpsk.FEC_inner = CFrontend::getFEC(buffer[12] & 0x0F);
	polarization = (buffer[8] >> 5) & 0x03;

	if (scantransponders.count((transport_stream_id << 16) | original_network_id) == 0)
	{
		found_transponders++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CEventServer::INITID_ZAPIT,
			&found_transponders,
			sizeof(found_transponders)
		);

		scantransponders.insert
		(
			std::pair <uint32_t, transpondermap>
			(
				(transport_stream_id << 16) | original_network_id,
				transpondermap
				(
					transport_stream_id,
					original_network_id,
					feparams,
					polarization,
					DiSEqC
				)
			)
		);
	}

	return buffer[1];
}

/* 0x44 */
uint8_t cable_delivery_system_descriptor (uint8_t *buffer, uint16_t transport_stream_id, uint16_t original_network_id)
{
	FrontendParameters feparams;

	feparams.Frequency =
	(
		((buffer[2] >> 4)	* 1000000) +
		((buffer[2] & 0x0F)	* 100000) +
		((buffer[3] >> 4)	* 10000) +
		((buffer[3] & 0x0F)	* 1000) +
		((buffer[4] >> 4)	* 100) +
		((buffer[4] & 0x0F)	* 10) +
		((buffer[5] >> 4)	* 1)
	);

	feparams.Inversion = INVERSION_AUTO;

	feparams.u.qam.SymbolRate =
	(
		((buffer[9] >> 4)	* 100000000) +
		((buffer[9] & 0x0F)	* 10000000) +
		((buffer[10] >> 4)	* 1000000) +
		((buffer[10] & 0x0F)	* 100000) +
		((buffer[11] >> 4)	* 10000) +
		((buffer[11] & 0x0F)	* 1000) +
		((buffer[12] >> 4)	* 100)
	);

	feparams.u.qam.FEC_inner = CFrontend::getFEC(buffer[12] & 0x0F);
	feparams.u.qam.QAM = CFrontend::getModulation(buffer[8]);

	if (scantransponders.count((transport_stream_id << 16) | original_network_id) == 0)
	{
		found_transponders++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CEventServer::INITID_ZAPIT,
			&found_transponders,
			sizeof(found_transponders)
		);

		scantransponders.insert
		(
			std::pair <uint32_t, transpondermap>
			(
				(transport_stream_id << 16) | original_network_id,
				transpondermap
				(
					transport_stream_id,
					original_network_id,
					feparams
				)
			)
		);
	}

	return buffer[1];
}

/* 0x45 */
uint8_t VBI_data_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x46 */
uint8_t VBI_teletext_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x47 */
uint8_t bouquet_name_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x48 */
uint8_t service_descriptor (uint8_t *buffer, uint16_t service_id, uint16_t transport_stream_id, uint16_t original_network_id)
{
	uint8_t service_type = buffer[2];
	uint8_t service_provider_name_length = buffer[3];
	uint16_t pos;

	std::string providerName;
	std::string serviceName;
	std::map <uint32_t, scanchannel>::iterator I = scanchannels.find((transport_stream_id << 16) | service_id);

	for (pos = 4; pos < service_provider_name_length + 4; pos++)
	{
		providerName += charToXML(buffer[pos]);
	}

	for (pos++; pos < buffer[1] + 2; pos++)
	{
		serviceName += charToXML(buffer[pos]);
	}

	if (scanchannels.count((transport_stream_id << 16) | service_id) != 0)
	{
		I->second.name = serviceName;
		I->second.onid = original_network_id;
		I->second.service_type = service_type;
	}
	else
	{
		found_channels++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_CHANNELS,
			CEventServer::INITID_ZAPIT,
			&found_channels,
			sizeof(found_channels)
		);

		scanchannels.insert
		(
			std::pair <uint32_t, scanchannel>
			(
				(transport_stream_id << 16) | service_id,
				scanchannel
				(
					serviceName,
					service_id,
					transport_stream_id,
					original_network_id,
					service_type
				)
			)
		);
	}

	if (providerName == "")
	{
		providerName = "Unknown Provider";
	}

	if (lastProviderName != providerName)
	{
		lastProviderName = providerName;
		eventServer->sendEvent(CZapitClient::EVT_SCAN_PROVIDER, CEventServer::INITID_ZAPIT, (void *) lastProviderName.c_str(), lastProviderName.length());
	}

	switch (service_type)
	{
	case DIGITAL_TELEVISION_SERVICE:
	case DIGITAL_RADIO_SOUND_SERVICE:
	case NVOD_REFERENCE_SERVICE:
	case NVOD_TIME_SHIFTED_SERVICE:
		scanbouquets.insert(std::pair <std::string, bouquet_mulmap> (providerName.c_str(), bouquet_mulmap(providerName, serviceName, service_id, original_network_id)));
		break;

	default:
		break;
	}

	return buffer[1];
}

/* 0x49 */
uint8_t country_availability_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x4A */
uint8_t linkage_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x4B */
uint8_t NVOD_reference_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x4C */
uint8_t time_shifted_service_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x4D */
uint8_t short_event_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x4E */
uint8_t extended_event_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x4F */
uint8_t time_shifted_event_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x50 */
uint8_t component_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x51 */
uint8_t mosaic_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x52 */
uint8_t stream_identifier_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x53 */
uint8_t CA_identifier_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x54 */
uint8_t content_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x55 */
uint8_t parental_rating_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x56 */
uint8_t teletext_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x57 */
uint8_t telephone_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x58 */
uint8_t local_time_offset_descriptor (uint8_t* buffer)
{
	return buffer[1];
}

/* 0x59 */
uint8_t subtitling_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x5A */
uint8_t terrestrial_delivery_system_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x5B */
uint8_t multilingual_network_name_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x5C */
uint8_t multilingual_bouquet_name_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x5D */
uint8_t multilingual_service_name_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x5E */
uint8_t multilingual_component_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x5F */
uint8_t private_data_specifier_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x60 */
uint8_t service_move_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x61 */
uint8_t short_smoothing_buffer_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x62 */
uint8_t frequency_list_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x63 */
uint8_t partial_transport_stream_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x64 */
uint8_t data_broadcast_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x65 */
uint8_t CA_system_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x66 */
uint8_t data_broadcast_id_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x67 */
uint8_t transport_stream_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x68 */
uint8_t DSNG_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x69 */
uint8_t PDC_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x6A */
uint8_t AC3_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x6B */
uint8_t ancillary_data_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x6C */
uint8_t cell_list_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x6D */
uint8_t cell_frequency_link_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

/* 0x6E */
uint8_t announcement_support_descriptor (uint8_t *buffer)
{
	return buffer[1];
}

