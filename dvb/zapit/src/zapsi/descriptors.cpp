/*
 * $Id: descriptors.cpp,v 1.71 2005/04/17 06:56:16 metallica Exp $
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

#include <cstdio>
#include <map>
#include <string>

/* libevent */
#include <eventserver.h>

#include <zapit/bouquets.h>
#include <zapit/client/zapitclient.h>
#include <zapit/descriptors.h>
#include <zapit/dvbstring.h>
#include <zapit/frontend.h>
#include <zapit/scan.h>
#include <zapit/sdt.h>
#include <zapit/debug.h>

extern tallchans allchans;              //  defined in zapit.cpp
extern transponder_list_t transponders; //  defined in zapit.cpp
std::string curr_chan_name;
uint32_t found_transponders;
uint32_t found_channels;
std::string lastProviderName;
uint32_t found_tv_chans;
uint32_t found_radio_chans;
uint32_t found_data_chans;
std::string lastServiceName;
std::map <t_channel_id, uint8_t> service_types;

extern CFrontend *frontend;
extern CEventServer *eventServer;

void generic_descriptor(const unsigned char * const)
{
#if 0
	DBG("generic descriptor dump:");
	for (unsigned short i = 0; i < buffer[1] + 2; i++)
		printf(" %02x", buffer[i]);
	printf("\n");
#endif
}

/* 0x02 */
void video_stream_descriptor(const unsigned char * const)
{
}

/* 0x03 */
void audio_stream_descriptor(const unsigned char * const)
{
}

/* 0x04 */
void hierarchy_descriptor(const unsigned char * const)
{
}

/* 0x05 */
void registration_descriptor(const unsigned char * const)
{
}

/* 0x06 */
void data_stream_alignment_descriptor(const unsigned char * const)
{
}

/* 0x07 */
void target_background_grid_descriptor(const unsigned char * const)
{
}

/* 0x08 */
void Video_window_descriptor(const unsigned char * const)
{
}

/* 0x09 */
void CA_descriptor(const unsigned char * const buffer, uint16_t ca_system_id, uint16_t* ca_pid)
{
	if ((((buffer[2] & 0x1F) << 8) | buffer[3]) == ca_system_id)
		*ca_pid = ((buffer[4] & 0x1F) << 8) | buffer[5];
}

/* 0x0A */
void ISO_639_language_descriptor(const unsigned char * const)
{
}

/* 0x0B */
void System_clock_descriptor(const unsigned char * const)
{
}

/* 0x0C */
void Multiplex_buffer_utilization_descriptor(const unsigned char * const)
{
}

/* 0x0D */
void Copyright_descriptor(const unsigned char * const)
{
}

/* 0x0E */
void Maximum_bitrate_descriptor(const unsigned char * const)
{
}

/* 0x0F */
void Private_data_indicator_descriptor(const unsigned char * const)
{
}

/* 0x10 */
void Smoothing_buffer_descriptor(const unsigned char * const)
{
}

/* 0x11 */
void STD_descriptor(const unsigned char * const)
{
}

/* 0x12 */
void IBP_descriptor(const unsigned char * const)
{
}

/*
 * 0x13 ... 0x1A: Defined in ISO/IEC 13818-6
 */

/* 0x1B */
void MPEG4_video_descriptor(const unsigned char * const)
{
}

/* 0x1C */
void MPEG4_audio_descriptor(const unsigned char * const)
{
}

/* 0x1D */
void IOD_descriptor(const unsigned char * const)
{
}

/* 0x1E */
void SL_descriptor(const unsigned char * const)
{
}

/* 0x1F */
void FMC_descriptor(const unsigned char * const)
{
}

/* 0x20 */
void External_ES_ID_descriptor(const unsigned char * const)
{
}

/* 0x21 */
void MuxCode_descriptor(const unsigned char * const)
{
}

/* 0x22 */
void FmxBufferSize_descriptor(const unsigned char * const)
{
}

/* 0x23 */
void MultiplexBuffer_descriptor(const unsigned char * const)
{
}

/* 0x24 */
void FlexMuxTiming_descriptor(const unsigned char * const)
{
}

/*
 * 0x25 ... 0x39:  ITU-T H.222.0 | ISO/IEC 13818-1 Reserved
 */

/* 0x40 */
void network_name_descriptor(const unsigned char * const)
{
}

/* 0x41 */
void service_list_descriptor(const unsigned char * const buffer, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id)
{
	for (int i = 0; i < buffer[1]; i += 3) {
		service_types[CREATE_CHANNEL_ID_FROM_SERVICE_ORIGINALNETWORK_TRANSPORTSTREAM_ID((((t_service_id)buffer[i + 2]) << 8) | buffer[i + 3], original_network_id, transport_stream_id)] = buffer[i + 4];
	}
}

/* 0x42 */
void stuffing_descriptor(const unsigned char * const)
{
}

/* 0x43 */
int satellite_delivery_system_descriptor(const unsigned char * const buffer, const transponder_id_t transponder_id2, unsigned char DiSEqC)
{
	dvb_frontend_parameters feparams;
	uint8_t polarization;
	frequency_kHz_t zfrequency;
	
	if (frontend->getInfo()->type != FE_QPSK)
		return -1;

	feparams.frequency =
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

	feparams.inversion = INVERSION_AUTO;

	feparams.u.qpsk.symbol_rate =
	(
		((buffer[9] >> 4)	* 100000000) +
		((buffer[9] & 0x0F)	* 10000000) +
		((buffer[10] >> 4)	* 1000000) +
		((buffer[10] & 0x0F)	* 100000) +
		((buffer[11] >> 4)	* 10000) +
		((buffer[11] & 0x0F)	* 1000) +
		((buffer[12] >> 4)	* 100)
	);

	feparams.u.qpsk.fec_inner = CFrontend::getCodeRate(buffer[12] & 0x0F);
	polarization = (buffer[8] >> 5) & 0x03;

	/* workarounds for braindead broadcasters (e.g. on Telstar 12 at 15.0W) */
	if (feparams.frequency >= 100000000)
		feparams.frequency /= 10;
	if (feparams.u.qpsk.symbol_rate >= 50000000)
		feparams.u.qpsk.symbol_rate /= 10;

	zfrequency = FREQUENCY_IN_KHZ(feparams.frequency);
	const transponder_id_t transponder_id = (transponder_id2) | (((transponder_id_t)zfrequency)<<48);
	
	if (transponders.find(transponder_id) == transponders.end())
	{
		found_transponders++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CEventServer::INITID_ZAPIT,
			&found_transponders,
			sizeof(found_transponders)
		);

		transponders.insert
		(
			std::pair<transponder_id_t, transponder>
			(
				transponder_id,
				transponder
				(
					GET_TRANSPORT_STREAM_ID_FROM_TRANSPONDER_ID(transponder_id),
					GET_ORIGINAL_NETWORK_ID_FROM_TRANSPONDER_ID(transponder_id),
					feparams,
					polarization,
					DiSEqC
				)
			)
		);
	}

	return 0;
}

/* 0x44 */
int cable_delivery_system_descriptor(const unsigned char * const buffer, const transponder_id_t transponder_id2)
{
	frequency_kHz_t zfrequency;
	
	if (frontend->getInfo()->type != FE_QAM)
		return -1;

	dvb_frontend_parameters feparams;

	feparams.frequency =
	(
		((buffer[2] >> 4)	* 1000000000) +
		((buffer[2] & 0x0F)	* 100000000) +
		((buffer[3] >> 4)	* 10000000) +
		((buffer[3] & 0x0F)	* 1000000) +
		((buffer[4] >> 4)	* 100000) +
		((buffer[4] & 0x0F)	* 10000) +
		((buffer[5] >> 4)	* 1000) +
		((buffer[5] & 0x0F)	* 100)
	);

	feparams.inversion = INVERSION_AUTO;

	feparams.u.qam.symbol_rate =
	(
		((buffer[9] >> 4)	* 100000000) +
		((buffer[9] & 0x0F)	* 10000000) +
		((buffer[10] >> 4)	* 1000000) +
		((buffer[10] & 0x0F)	* 100000) +
		((buffer[11] >> 4)	* 10000) +
		((buffer[11] & 0x0F)	* 1000) +
		((buffer[12] >> 4)	* 100)
	);

	feparams.u.qam.fec_inner = CFrontend::getCodeRate(buffer[12] & 0x0F);
	feparams.u.qam.modulation = CFrontend::getModulation(buffer[8]);

	zfrequency = FREQUENCY_IN_KHZ(feparams.frequency);
	const transponder_id_t transponder_id = (transponder_id2) | (((transponder_id_t)zfrequency)<<48);
	
	if (transponders.find(transponder_id) == transponders.end())
	{
		found_transponders++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CEventServer::INITID_ZAPIT,
			&found_transponders,
			sizeof(found_transponders)
		);

		transponders.insert
		(
			std::pair<transponder_id_t, transponder>
			(
				transponder_id,
				transponder
				(
					GET_TRANSPORT_STREAM_ID_FROM_TRANSPONDER_ID(transponder_id),
					GET_ORIGINAL_NETWORK_ID_FROM_TRANSPONDER_ID(transponder_id),
					feparams
				)
			)
		);
	}

	return 0;
}

/* 0x45 */
void VBI_data_descriptor(const unsigned char * const)
{
}

/* 0x46 */
void VBI_teletext_descriptor(const unsigned char * const)
{
}

/* 0x47 */
void bouquet_name_descriptor(const unsigned char * const)
{
}

/* 0x48 */
void service_descriptor(const unsigned char * const buffer, const t_service_id service_id, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, const t_satellite_position satellite_position, const uint8_t DiSEqC, const uint32_t frequency)
{
	frequency_kHz_t zfrequency;
	tallchans_iterator I = allchans.find(CREATE_CHANNEL_ID);

	if (I != allchans.end())
		return;
	
	uint8_t service_type = buffer[2];
	uint8_t service_provider_name_length = buffer[3];

	std::string providerName((const char*)&(buffer[4]), service_provider_name_length);
	std::string serviceName;

	bool in_blacklist = false;

	if (providerName == "CanalSat\xE9lite")
	{
		providerName = "CanalSat\xC3\xA9lite";
		in_blacklist = true;
	}
	else if (providerName == "Chambre des D\xE9" "put\xE9" "es")
	{
		providerName = "Chambre des D\xC3\xA9" "put\xC3\xA9" "es";
		in_blacklist = true;
	}
	else if (providerName == "PREMIERE")
	{
		providerName = "Premiere"; // well the name PREMIERE itself is not a problem
		in_blacklist = true;
	}

	if (in_blacklist)
	{
		if (((unsigned char)buffer[4 + service_provider_name_length + 1]) >= 0x20) // no encoding info
			serviceName  = CDVBString(("\x05" + std::string((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1))).c_str(), (2 + buffer[1]) - (4 + service_provider_name_length + 1) + 1).getContent(); // add artificial encoding info
		else
			serviceName  = CDVBString((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1)).getContent();
	}
	else
	{
		providerName = CDVBString((const char*)&(buffer[4]), service_provider_name_length).getContent();
		serviceName  = CDVBString((const char*)&(buffer[4 + service_provider_name_length + 1]), (2 + buffer[1]) - (4 + service_provider_name_length + 1)).getContent();
	}

	found_channels++;

	eventServer->sendEvent
	(
		CZapitClient::EVT_SCAN_NUM_CHANNELS,
		CEventServer::INITID_ZAPIT,
		&found_channels,
		sizeof(found_channels)
	);

	zfrequency = FREQUENCY_IN_KHZ(frequency);
	allchans.insert
	(
		std::pair <t_channel_id, CZapitChannel>
		(
			CREATE_CHANNEL_ID,
			CZapitChannel
			(
				serviceName,
				service_id,
				transport_stream_id,
				original_network_id,
				service_type,
				DiSEqC,
				satellite_position,
				zfrequency 
			)
		)
	);

#define UNKNOWN_PROVIDER_NAME "Unknown Provider"

 //scanifostruct scaninfo;

	if (providerName.empty())
		providerName = CDVBString(UNKNOWN_PROVIDER_NAME, strlen(UNKNOWN_PROVIDER_NAME)).getContent();

	if (lastProviderName != providerName)
	{
		lastProviderName = providerName;
		eventServer->sendEvent(CZapitClient::EVT_SCAN_PROVIDER, CEventServer::INITID_ZAPIT, (void *) lastProviderName.c_str(), lastProviderName.length() + 1);
	}

	switch (service_type) {
	case ST_DIGITAL_TELEVISION_SERVICE:
 		found_tv_chans++;
 		eventServer->sendEvent(CZapitClient::EVT_SCAN_FOUND_TV_CHAN, CEventServer::INITID_ZAPIT, &found_tv_chans, sizeof(found_tv_chans));
 		break;
 	case ST_DIGITAL_RADIO_SOUND_SERVICE:
 		found_radio_chans++;
 		eventServer->sendEvent(CZapitClient::EVT_SCAN_FOUND_RADIO_CHAN, CEventServer::INITID_ZAPIT, &found_radio_chans, sizeof(found_radio_chans));
 		break;
 	case ST_NVOD_REFERENCE_SERVICE:
 	case ST_NVOD_TIME_SHIFTED_SERVICE:
 	case ST_DATA_BROADCAST_SERVICE:
 	case ST_RCS_MAP:
 	case ST_RCS_FLS:
 	default:
 		found_data_chans++;
 		eventServer->sendEvent(CZapitClient::EVT_SCAN_FOUND_DATA_CHAN, CEventServer::INITID_ZAPIT, &found_data_chans, sizeof(found_data_chans));
 		break;
 	}
 	switch (service_type) {
 	case ST_DIGITAL_TELEVISION_SERVICE:
	case ST_DIGITAL_RADIO_SOUND_SERVICE:
	case ST_NVOD_REFERENCE_SERVICE:
	case ST_NVOD_TIME_SHIFTED_SERVICE:
	{
		CBouquet* bouquet;
		int bouquetId;

		bouquetId = scanBouquetManager->existsBouquet(providerName.c_str());

		if (bouquetId == -1)
			bouquet = scanBouquetManager->addBouquet(providerName);
		else
			bouquet = scanBouquetManager->Bouquets[bouquetId];

 		lastServiceName = serviceName;
 		eventServer->sendEvent(CZapitClient::EVT_SCAN_SERVICENAME, CEventServer::INITID_ZAPIT, (void *) lastServiceName.c_str(), lastServiceName.length() + 1);

//		bouquet->addService(new CZapitChannel(serviceName, service_id, transport_stream_id, original_network_id, service_type, 0, satellite_position));
		bouquet->addService(new CZapitChannel(serviceName, service_id, transport_stream_id, original_network_id, service_type, 0, satellite_position, zfrequency ));

 // thegoodguy schau dir das hier mal an
 //		scaninfo  test;
 //		test.found_tv_chans = found_tv_chans;
 //		test.found_radio_chans = found_radio_chans;
 //		test.found_data_chans = found_data_chans;
 //		strncpy(test.ServiceName,serviceName.c_str(),30);
 //		test.ServiceName = serviceName;
 //		INFO ( " Sendername : %s ! " , test.ServiceName);
 //		eventServer->sendEvent(CZapitClient::EVT_SCAN_FOUND_A_CHAN, CEventServer::INITID_ZAPIT, &test, sizeof(test));

		break;
	}
	default:
		break;
	}
}

/* 0x49 */
void country_availability_descriptor(const unsigned char * const)
{
}

/* 0x4A */
void linkage_descriptor(const unsigned char * const)
{
}

/* 0x4B */
int NVOD_reference_descriptor(
	const unsigned char * const buffer,
	const unsigned int num,
	t_transport_stream_id * const tsid,
	t_original_network_id * const onid,
	t_service_id * const sid)
{
	if ((unsigned int)(buffer[1] / 6) + 1 >= num) {
		*tsid = (buffer[2 + (6 * num)] << 16) | buffer[3 + (6 * num)];
		*onid = (buffer[4 + (6 * num)] << 16) | buffer[5 + (6 * num)];
		*sid =  (buffer[6 + (6 * num)] << 16) | buffer[7 + (6 * num)];
		return 0;
	}

	return -1;
}

/* 0x4C */
void time_shifted_service_descriptor(const unsigned char * const)
{
}

/* 0x4D */
void short_event_descriptor(const unsigned char * const)
{
}

/* 0x4E */
void extended_event_descriptor(const unsigned char * const)
{
}

/* 0x4F */
void time_shifted_event_descriptor(const unsigned char * const)
{
}

/* 0x50 */
void component_descriptor(const unsigned char * const)
{
}

/* 0x51 */
void mosaic_descriptor(const unsigned char * const)
{
}

/* 0x52 */
void stream_identifier_descriptor(const unsigned char * const)
{
}

/* 0x53 */
void CA_identifier_descriptor(const unsigned char * const)
{
}

/* 0x54 */
void content_descriptor(const unsigned char * const)
{
}

/* 0x55 */
void parental_rating_descriptor(const unsigned char * const)
{
}

/* 0x56 */
void teletext_descriptor(const unsigned char * const)
{
}

/* 0x57 */
void telephone_descriptor(const unsigned char * const)
{
}

/* 0x58 */
void local_time_offset_descriptor(const unsigned char * const)
{
}

/* 0x59 */
void subtitling_descriptor(const unsigned char * const)
{
}

/* 0x5A */
int terrestrial_delivery_system_descriptor(const unsigned char * const)
{
	if (frontend->getInfo()->type != FE_OFDM)
		return -1;

	/* TODO */

	return 0;
}

/* 0x5B */
void multilingual_network_name_descriptor(const unsigned char * const)
{
}

/* 0x5C */
void multilingual_bouquet_name_descriptor(const unsigned char * const)
{
}

/* 0x5D */
void multilingual_service_name_descriptor(const unsigned char * const)
{
}

/* 0x5E */
void multilingual_component_descriptor(const unsigned char * const)
{
}

/* 0x5F */
void private_data_specifier_descriptor(const unsigned char * const)
{
}

/* 0x60 */
void service_move_descriptor(const unsigned char * const)
{
}

/* 0x61 */
void short_smoothing_buffer_descriptor(const unsigned char * const)
{
}

/* 0x62 */
void frequency_list_descriptor(const unsigned char * const)
{
}

/* 0x63 */
void partial_transport_stream_descriptor(const unsigned char * const)
{
}

/* 0x64 */
void data_broadcast_descriptor(const unsigned char * const)
{
}

/* 0x65 */
void CA_system_descriptor(const unsigned char * const)
{
}

/* 0x66 */
void data_broadcast_id_descriptor(const unsigned char * const)
{
}

/* 0x67 */
void transport_stream_descriptor(const unsigned char * const)
{
}

/* 0x68 */
void DSNG_descriptor(const unsigned char * const)
{
}

/* 0x69 */
void PDC_descriptor(const unsigned char * const)
{
}

/* 0x6A */
void AC3_descriptor(const unsigned char * const)
{
}

/* 0x6B */
void ancillary_data_descriptor(const unsigned char * const)
{
}

/* 0x6C */
void cell_list_descriptor(const unsigned char * const)
{
}

/* 0x6D */
void cell_frequency_link_descriptor(const unsigned char * const)
{
}

/* 0x6E */
void announcement_support_descriptor(const unsigned char * const)
{
}

/* 0x6F ... 0x7F: reserved */
/* 0x80 ... 0xFE: user private */
/* 0xFF: forbidden */
