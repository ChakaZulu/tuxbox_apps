/*
 * $Id: getservices.cpp,v 1.84 2003/12/09 21:12:28 thegoodguy Exp $
 *
 * (C) 2002, 2003 by Andreas Oberritter <obi@tuxbox.org>
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

#include <zapit/bouquets.h>
#include <zapit/channel.h>
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/getservices.h>
#include <zapit/settings.h>
#include <zapit/xmlinterface.h>

extern std::map <uint32_t, transponder> transponders;
extern tallchans allchans;

std::map<std::string, t_satellite_position> satellitePositions; //satellite position as specified in satellites.xml

std::map<t_satellite_position, uint8_t> motorPositions; //stored satellitepositions in diseqc 1.2 motor
std::map<t_satellite_position, uint8_t>::iterator mpos_it;

std::map<string, int> satelliteDiseqcs; //diseqcs per satellite
std::map<string, int>::iterator satdiseqc_it;

void ParseTransponders(xmlNodePtr node, const uint8_t DiSEqC, t_satellite_position satellitePosition)
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	struct dvb_frontend_parameters feparams;
	uint8_t polarization = 0;

	memset(&feparams, 0, sizeof(struct dvb_frontend_parameters));

	/* read all transponders */
	while ((node = xmlGetNextOccurence(node, "transponder")) != NULL)
	{
		/* common */
		transport_stream_id = xmlGetNumericAttribute(node, "id", 16);
		original_network_id = xmlGetNumericAttribute(node, "onid", 16);
		feparams.frequency = xmlGetNumericAttribute(node, "frequency", 0);
		feparams.inversion = (fe_spectral_inversion_t) xmlGetNumericAttribute(node, "inversion", 0);

		/* cable */
		if (DiSEqC == 0xFF) {
			feparams.u.qam.symbol_rate = xmlGetNumericAttribute(node, "symbol_rate", 0);
			feparams.u.qam.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(node, "fec_inner", 0);
			feparams.u.qam.modulation = CFrontend::getModulation(xmlGetNumericAttribute(node, "modulation", 0));
		}

		/* terrestrial */
		else if (DiSEqC == 0xFE) {
			feparams.u.ofdm.bandwidth = (fe_bandwidth_t) xmlGetNumericAttribute(node, "bandwidth", 0);
			feparams.u.ofdm.code_rate_HP = (fe_code_rate_t) xmlGetNumericAttribute(node, "code_rate_HP", 0);
			feparams.u.ofdm.code_rate_LP = (fe_code_rate_t) xmlGetNumericAttribute(node, "code_rate_LP", 0);
			feparams.u.ofdm.constellation = (fe_modulation_t) xmlGetNumericAttribute(node, "constellation", 0);
			feparams.u.ofdm.transmission_mode = (fe_transmit_mode_t) xmlGetNumericAttribute(node, "transmission_mode", 0);
			feparams.u.ofdm.guard_interval = (fe_guard_interval_t) xmlGetNumericAttribute(node, "guard_interval", 0);
			feparams.u.ofdm.hierarchy_information = (fe_hierarchy_t) xmlGetNumericAttribute(node, "hierarchy_information", 0);
		}

		/* satellite */
		else {
			feparams.u.qpsk.symbol_rate = xmlGetNumericAttribute(node, "symbol_rate", 0);
			feparams.u.qpsk.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(node, "fec_inner", 0);
			polarization = xmlGetNumericAttribute(node, "polarization", 0);
		}

		/* add current transponder to list */
		transponders.insert
		(
			std::pair <uint32_t, transponder>
			(
				(transport_stream_id << 16) | original_network_id,
				transponder
				(
					transport_stream_id,
					feparams,
					polarization,
					DiSEqC,
					original_network_id
				)
			)
		);

		/* read channels that belong to the current transponder */
		ParseChannels(node->xmlChildrenNode, transport_stream_id, original_network_id, DiSEqC, satellitePosition);

		/* hop to next transponder */
		node = node->xmlNextNode;
	}

	return;
}

void ParseChannels(xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, const unsigned char DiSEqC, t_satellite_position satellitePosition)
{
	t_service_id service_id;
	std::string  name;
	uint8_t      service_type;

	while ((node = xmlGetNextOccurence(node, "channel")) != NULL)
	{
		service_id = xmlGetNumericAttribute(node, "service_id", 16);
		name = xmlGetAttribute(node, "name");
		service_type = xmlGetNumericAttribute(node, "service_type", 16);

		switch (service_type) {
		case ST_DIGITAL_TELEVISION_SERVICE:
		case ST_NVOD_REFERENCE_SERVICE:
		case ST_NVOD_TIME_SHIFTED_SERVICE:
		case ST_DIGITAL_RADIO_SOUND_SERVICE:
			allchans.insert
			(
				std::pair <t_channel_id, CZapitChannel>
				(
					CREATE_CHANNEL_ID,
					CZapitChannel
					(
						name,
						service_id,
						transport_stream_id,
						original_network_id,
						service_type,
						DiSEqC, 
						satellitePosition
					)
				)
			);

			break;

		default:
			break;
		}

		node = node->xmlNextNode;
	}

	return;
}

void FindTransponder(xmlNodePtr search)
{
	uint8_t DiSEqC;
	t_satellite_position satellitePosition = 0;

	while (search)
	{
		if (!(strcmp(xmlGetName(search), "cable")))
			DiSEqC = 0xff;

		else if (!(strcmp(xmlGetName(search), "sat")))
		     {	
			DiSEqC = xmlGetNumericAttribute(search, "diseqc", 0);
			satellitePosition = satellitePositions[xmlGetAttribute(search, "name")];
		     }

		else if (!(strcmp(xmlGetName(search), "terrestrial")))
			DiSEqC = 0xfe;

		else {
			search = search->xmlNextNode;
			continue;
		}

		INFO("going to parse dvb-%c provider %s", xmlGetName(search)[0], xmlGetAttribute(search, "name"));
		ParseTransponders(search->xmlChildrenNode, DiSEqC, satellitePosition);
		search = search->xmlNextNode;
	}
}

int LoadMotorPositions(void)
{
	FILE *fd = NULL;
	int motorPosition = 0;
	char buffer[256] = "";
	t_satellite_position satellitePosition;
	int temp1, temp2;
	
	printf("[getservices] loading motor positions...\n");
	
	motorPositions.clear();
	if ((fd = fopen(MOTORCONFIGFILE, "r")))
	{
		fgets(buffer, 255, fd);
		while(!feof(fd))
		{	
			sscanf(buffer, "%d %d", &temp1, &temp2);
			satellitePosition = (t_satellite_position)temp1;
			motorPosition = (uint8_t)temp2;
			motorPositions[satellitePosition] = motorPosition;
			fgets(buffer, 255, fd);	
		}
		fclose(fd);
		
//		for (mpos_it = motorPositions.begin(); mpos_it != motorPositions.end(); mpos_it++)
//			printf("satellitePosition = %d, motorPosition = %d\n", mpos_it->first, mpos_it->second);
	}
	else
		printf("[getservices] motor.conf not found.\n");
	
	return 0;
}

int LoadSatellitePositions(void)
{
	xmlDocPtr parser = parseXmlFile(SATELLITES_XML);

	if (parser == NULL)
	{
		printf("[getservices] satellites.xml not found.\n");
		return -1;
	}

	satellitePositions.clear();
	
	xmlNodePtr search = xmlDocGetRootElement(parser)->xmlChildrenNode;

	while (search)
	{
		if (!(strcmp(xmlGetName(search), "sat")))
			satellitePositions[xmlGetAttribute(search, "name")] = xmlGetSignedNumericAttribute(search, "position", 10);

		search = search->xmlNextNode;
	}
	
	xmlFreeDoc(parser);
		
	return 0;
}

int LoadServices(fe_type_t frontendType, diseqc_t diseqcType)
{
	if (frontendType == FE_QPSK)
	{
		LoadSatellitePositions();
	
		if (diseqcType == DISEQC_1_2)
			LoadMotorPositions();
	}
	
	xmlDocPtr parser = parseXmlFile(SERVICES_XML);

	if (parser == NULL)
		return -1;

	FindTransponder(xmlDocGetRootElement(parser)->xmlChildrenNode);
	xmlFreeDoc(parser);
	return 0;
}

