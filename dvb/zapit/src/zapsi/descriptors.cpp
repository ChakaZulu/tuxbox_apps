/*
 * $Id: descriptors.cpp,v 1.21 2002/04/14 06:06:31 obi Exp $
 */

#include <stdio.h>
#include <map>
#include <string>
#include <map.h>
#include "sdt.h"
#include "scan.h"

std::map <uint32_t, transpondermap> scantransponders;
std::map <uint32_t, scanchannel> scanchannels;
multimap <std::string, bouquet_mulmap> scanbouquets;
std::string curr_chan_name;
uint32_t found_transponders;
uint32_t found_channels;
char last_provider[100];

CodeRate getFEC(uint8_t FEC_inner)
{
	switch (FEC_inner)
	{
	case 0x01:
		return FEC_1_2;
	case 0x02:
		return FEC_2_3;
	case 0x03:
		return FEC_3_4;
	case 0x04:
		return FEC_5_6;
	case 0x05:
		return FEC_7_8;
	case 0x0F:
		return FEC_NONE;
	default:
		return FEC_AUTO;
	}
}

Modulation getModulation (uint8_t modulation)
{
	switch (modulation)
	{
	case 0x01:
		return QAM_16;
	case 0x02:
		return QAM_32;
	case 0x03:
		return QAM_64;
	case 0x04:
		return QAM_128;
	case 0x05:
		return QAM_256;
	default:
		return QAM_64;
	}
}

uint8_t ca_descriptor(uint8_t *buffer, uint16_t ca_system_id, uint16_t* ca_pid)
{
	if (((((buffer[2] & 0x1F) << 8) | buffer[3]) == ca_system_id) && ((buffer[2] & 0x1F) == 0x17))
	{
		*ca_pid = ((buffer[4] & 0x1F) << 8) | buffer[5];
	}
	else
	{
		*ca_pid = INVALID;
	}

	return buffer[1];
}

uint8_t stuffing_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t linkage_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t priv_data_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t network_name_desc(uint8_t *buffer)
{
	uint8_t descriptor_length = buffer[1];
#if 0
	uint8_t pos;
	std::string name;

	for (pos = 0; pos < descriptor_length; pos++)
		name += buffer[pos + 2];

	printf("Network-name: %s\n", name.c_str());
#endif
	return descriptor_length;
}

uint8_t service_list_desc(uint8_t *buffer)
{
	uint8_t descriptor_length = buffer[1];
#if 0
	uint8_t pos;
	uint16_t service_id;
	uint8_t service_type;

	for (pos = 2; pos < descriptor_length + 2; pos += 3)
	{
		service_id = (buffer[pos] << 8) | buffer[pos + 1];
		service_type = buffer[pos + 2];
		printf("service_id: %04x\n", service_id);
		printf("service_type: %04d\n", service_type);
	}
#endif
	return descriptor_length;
}

uint8_t cable_deliv_system_desc(uint8_t *buffer, uint16_t transport_stream_id, uint16_t original_network_id)
{
	FrontendParameters feparams;

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

	feparams.u.qam.FEC_inner = getFEC(buffer[12] & 0x0F);
	feparams.u.qam.QAM = getModulation(buffer[8]);

#ifdef DEBUG
	printf("[descriptor.cpp] frequency: %u\n", feparams.Frequency);
	printf("[descriptor.cpp] modulation %d, symbol_rate %u, FEC_inner %d\n", feparams.u.qam.QAM, feparams.u.qam.SymbolRate, feparams.u.qam.FEC_inner);
#endif

	if (scantransponders.count((transport_stream_id << 16) | original_network_id) == 0)
	{
		printf("[descriptor.cpp] new transponder - tsid: %04x, onid %04x\n", transport_stream_id, original_network_id);
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

uint8_t sat_deliv_system_desc(uint8_t *buffer, uint16_t transport_stream_id, uint16_t original_network_id, uint8_t DiSEqC)
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

	feparams.u.qpsk.FEC_inner = getFEC(buffer[12] & 0x0F);
	polarization = (buffer[8] >> 5) & 0x03;

#ifdef DEBUG
	printf("[descriptor.cpp] frequency: %u, polarization: %s\n", feparams.Frequency, polarization ? "V" : "H");
	printf("[descriptor.cpp] symbol_rate %u, FEC_inner %d\n", feparams.u.qpsk.SymbolRate, feparams.u.qpsk.FEC_inner);
#endif

	if (scantransponders.count((transport_stream_id << 16) | original_network_id) == 0)
	{
		printf("[descriptor.cpp] new transponder - tsid: %04x, onid %04x\n", transport_stream_id, original_network_id);
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

uint8_t terr_deliv_system_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t multilingual_network_name_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t freq_list_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t cell_list_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t cell_freq_list_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t announcement_support_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t service_name_desc(uint8_t *buffer, uint16_t service_id, uint16_t transport_stream_id, uint16_t original_network_id, bool scan_mode)
{
	uint8_t descriptor_length = buffer[1];
	uint8_t service_type = buffer[2];
	uint8_t service_provider_name_length = buffer[3];
	//uint8_t service_name_length = buffer[service_provider_name_length + 4];

	uint16_t pos;
	char val[5];

	std::string provname;
	std::string servicename;
	std::map <uint32_t, scanchannel>::iterator I = scanchannels.find((transport_stream_id << 16) | service_id);

	for (pos = 4; pos < service_provider_name_length + 4; pos++)
	{
		switch (buffer[pos])
		{
		case '&':
			provname += "&amp;";
       			break;
		case '<':
			provname += "&lt;";
			break;
		case '\"':
			provname += "&quot;";
			break;
		case 0x81:
		case 0x82:
			break;
		case 0x86:
			//provname += "<b>";
			break;
		case 0x87:
			//provname += "</b>";
			break;
		case 0x8a:
			//provname += "<br/>";
			break;
		default:
			if (buffer[pos] < 32)
				break;
			if ((buffer[pos] >= 32) && (buffer[pos] < 128))
				provname += buffer[pos];
			else if (buffer[pos] == 128)
				break;
			else
			{
				sprintf(val, "%d", buffer[pos]);
				provname += "&#";
				provname += val;
				provname += ";";
			}
		}
	}

	for (pos++; pos < descriptor_length + 2; pos++)
	{
		switch (buffer[pos])
		{
     		case '&':
			servicename += "&amp;";
			break;
		case '<':
			servicename += "&lt;";
			break;
		case '\"':
			servicename += "&quot;";
			break;
		case 0x81:
		case 0x82:
			break;
		case 0x86:
			//servicename += "<b>";
			break;
		case 0x87:
			//servicename += "</b>";
			break;
		case 0x8a:
			//servicename += "<br/>";
			break;
		default:
			if (buffer[pos] < 32)
				break;
			if ((buffer[pos] >= 32) && (buffer[pos] < 128))
				servicename += buffer[pos];
			else if (buffer[pos] == 128)
				break;
			else
			{
				sprintf(val, "%d", buffer[pos]);
				servicename += "&#";
				servicename += val;
				servicename += ";";
			}
		}
	}

#ifdef DEBUG
	printf("provider: %s\n", provname.c_str());
	printf("service: %s\n", servicename.c_str());
#endif

	if (scan_mode)
	{
		if (scanchannels.count((transport_stream_id << 16) | service_id) != 0)
		{
#ifdef DEBUG
			printf("[descriptors.cpp] Found a channel in map\n");
#endif
			I->second.name = servicename;
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
						servicename,
						service_id,
						transport_stream_id,
						original_network_id,
						service_type
					)
				)
			);
		}

		if (provname == "")
			provname = "Unknown Provider";

		if (strcmp(last_provider, provname.c_str()) != 0)
		{
			strcpy(last_provider, provname.c_str());
			eventServer->sendEvent(CZapitClient::EVT_SCAN_PROVIDER, CEventServer::INITID_ZAPIT, last_provider, strlen(last_provider) + 1);
		}

		if ((service_type == 1) || (service_type == 2) || (service_type == 4) || (service_type == 5))
			scanbouquets.insert(std::pair<std::string, bouquet_mulmap>(provname.c_str(), bouquet_mulmap(provname, servicename, service_id, original_network_id)));
	}

	return descriptor_length;
}

uint8_t bouquet_name_desc(uint8_t *buffer)
{
	uint8_t descriptor_length = buffer[1];
#if 0
	uint8_t pos;
	std::string name;

	for (pos = 0; pos < descriptor_length; pos++)
		name += buffer[pos + 2];

	printf("Bouquet name: %s\n", name.c_str());
#endif
	return descriptor_length;
}



uint8_t country_availability_desc(uint8_t *buffer)
{
	return buffer[1];
}


uint8_t nvod_ref_desc(uint8_t *buffer, uint16_t transport_stream_id, bool scan_mode)
{
	return buffer[1];
}

uint8_t time_shift_service_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t mosaic_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t ca_ident_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t telephone_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t multilingual_service_name_desc(uint8_t *buffer)
{
	return buffer[1];
}

uint8_t data_broadcast_desc(uint8_t *buffer)
{
	return buffer[1];
}

