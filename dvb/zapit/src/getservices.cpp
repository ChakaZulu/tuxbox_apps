/*
 * $Id: getservices.cpp,v 1.103 2009/03/21 14:29:12 seife Exp $
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
#include <xmltree/xmlinterface.h>
#include <configfile.h>
#include <sys/stat.h>

#if HAVE_DVB_API_VERSION < 3
#define frequency Frequency
#define symbol_rate SymbolRate
#define inversion Inversion
#define fec_inner FEC_inner
#define modulation QAM
#define bandwidth bandWidth
#define code_rate_LP LP_CodeRate
#define code_rate_HP HP_CodeRate
#define constellation Constellation
#define transmission_mode TransmissionMode
#define guard_interval guardInterval
#define hierarchy_information HierarchyInformation
#endif

extern transponder_list_t transponders;
extern tallchans allchans;

std::map<std::string, t_satellite_position> satellitePositions; //satellite position as specified in satellites.xml

std::map<t_satellite_position, uint8_t> motorPositions; //stored satellitepositions in diseqc 1.2 motor
std::map<t_satellite_position, uint8_t>::iterator mpos_it;

void ParseTransponders(xmlNodePtr node, const uint8_t DiSEqC, t_satellite_position satellitePosition)
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	dvb_frontend_parameters feparams;
	uint8_t polarization = 0;
	frequency_kHz_t frequency;
	
	memset(&feparams, 0, sizeof(dvb_frontend_parameters));

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
			feparams.u.qam.fec_inner = CFrontend::xml2FEC(xmlGetNumericAttribute(node, "fec_inner", 0));
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
			feparams.u.qpsk.fec_inner = CFrontend::xml2FEC(xmlGetNumericAttribute(node, "fec_inner", 0));
			polarization = xmlGetNumericAttribute(node, "polarization", 0);
		}

		if(feparams.frequency < 20000) feparams.frequency = feparams.frequency*1000;
		if(feparams.u.qpsk.symbol_rate < 50000) feparams.u.qpsk.symbol_rate = feparams.u.qpsk.symbol_rate * 1000;
		frequency = FREQUENCY_IN_KHZ(feparams.frequency);
		
		/* add current transponder to list */
		if (transponders.find(CREATE_TRANSPONDER_ID_FROM_FREQUENCY_SATELLITEPOSITION_ORIGINALNETWORK_TRANSPORTSTREAM_ID(frequency,satellitePosition,original_network_id,transport_stream_id)) != transponders.end())
		{
			printf("[getservices] dup transponder id %X onid %X\n", transport_stream_id, original_network_id);
		}

		transponders.insert
		(
			std::pair <transponder_id_t, transponder>
			(
				CREATE_TRANSPONDER_ID_FROM_FREQUENCY_SATELLITEPOSITION_ORIGINALNETWORK_TRANSPORTSTREAM_ID(frequency,satellitePosition,original_network_id,transport_stream_id),
				transponder
				(
					transport_stream_id,
					original_network_id,
					feparams,
					polarization,
					DiSEqC
				)
			)
		);

		/* read channels that belong to the current transponder */
		ParseChannels(node->xmlChildrenNode, transport_stream_id, original_network_id, DiSEqC, satellitePosition, feparams.frequency);

		/* hop to next transponder */
		node = node->xmlNextNode;
	}

	return;
}

void ParseChannels(xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, const unsigned char DiSEqC, t_satellite_position satellitePosition, const uint32_t frequency)
{
	extern CConfigFile config;
	bool trace_nukes = config.getBool("traceNukes", false);

	t_service_id service_id;
	std::string  name;
	uint8_t      service_type;

	frequency_kHz_t zfrequency = FREQUENCY_IN_KHZ(frequency);
	while ((node = xmlGetNextOccurence(node, "channel")) != NULL)
	{
		service_id = xmlGetNumericAttribute(node, "service_id", 16);
		name = xmlGetAttribute(node, "name");
		service_type = xmlGetNumericAttribute(node, "service_type", 16);

		char *ptr = xmlGetAttribute(node, "action");
		bool remove = ptr ? (!strcmp(ptr, "remove") || !strcmp(ptr, "replace")) : false;
		bool add    = ptr ? (!strcmp(ptr, "add")    || !strcmp(ptr, "replace")) : true;
		if (remove) {
		  int result = allchans.erase(CREATE_CHANNEL_ID);
		  if (!result || trace_nukes)
		    printf("[getservices]: %s '%s' (service_id=0x%x): %s", add ? "replacing" : "removing", 
			   xmlGetAttribute(node, "name"), service_id, result ? "succeded.\n" : "FAILED!\n");
		}

		if (add) {
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
							satellitePosition,
							zfrequency
						)
					)
				);
 				break;

			default:
				break;
			}
		}
		node = node->xmlNextNode;
	}

	return;
}

void FindTransponder(xmlNodePtr search)
{
	uint8_t DiSEqC;
	t_satellite_position satellitePosition = SATELLITE_POSITION_OF_NON_SATELLITE_SOURCE;

	while (search)
	{
		const char * search_type = xmlGetName(search);

		if (!(strcmp(search_type, "cable")))
		{
			DiSEqC = 0xff;
		}
		else if (!(strcmp(search_type, "sat")))
		{	
			DiSEqC = xmlGetNumericAttribute(search, "diseqc", 0);
			satellitePosition = satellitePositions[xmlGetAttribute(search, "name")];
		}
		else if (!(strcmp(search_type, "terrestrial")))
		{
			DiSEqC = 0xfe;
		}
		else
		{
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
	struct stat buf;
	std::string filename = (std::string)ZAPITCONFIGDIR + "/" + SATELLITES_XML;
	if ((stat(filename.c_str(), &buf) == -1) && (errno == ENOENT))
		filename = (std::string)DATADIR + "/" + SATELLITES_XML;

	xmlDocPtr parser = parseXmlFile(filename.c_str());

	if (parser == NULL)
	{
		printf("[getservices] satellites.xml\n");
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
//	for (std::map<string, t_satellite_position>::iterator spos_it = satellitePositions.begin(); spos_it != satellitePositions.end(); spos_it++)
//		printf("satelliteName = %s, satellitePosition = %d\n", spos_it->first.c_str(), spos_it->second);

	return 0;
}

/* This function is temporary only
 * Deleting this needs lots of fixes in many places
 */
void parse_static_pids(CZapitChannel* channel)
{
	char id[32];
	struct stat dummy;

	snprintf(id, sizeof(id), "%012Lx", channel->getChannelID());
	if (stat(STATICPIDS_XML, &dummy) == 0) {
		xmlDocPtr parser = parseXmlFile(STATICPIDS_XML, false);
		if (parser) {
			xmlNodePtr node = xmlDocGetRootElement(parser);
			while ((node = xmlGetNextOccurence(node, "subtitles")) != NULL) {
				xmlNodePtr tmp = node->xmlChildrenNode;
				while ((tmp = xmlGetNextOccurence(tmp, "channel")) != NULL) {
					std::string curid = xmlGetAttribute(tmp, "id");
					if (curid == id) {
						std::string mode = xmlGetAttribute(tmp, "mode");
						std::string lang = xmlGetAttribute(tmp, "lang");
						int pid = xmlGetNumericAttribute(tmp, "pid", 16);
						int page = xmlGetNumericAttribute(tmp, "page", 16);
						
						if (mode == "ttx") {
							channel->addTTXSubtitle(pid, lang, page >> 8,
																			page & 0xff, false);
						} else if (mode == "dvb") {
							channel->addDVBSubtitle(pid, lang, 0x10, page, 0);
							/* TODO: subtitle type -> hardcoded to 0x10
							 * TODO: ancillary page -> hardcoded to 0
							 */
						}
					}
					tmp = tmp->xmlNextNode;
				}
				node = node->xmlNextNode;				
			}
			xmlFreeDoc(parser);
		}
	}
}


int LoadServices(fe_type_t frontendType, diseqc_t diseqcType, bool only_current_services)
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

	struct stat testbuf;

	if (!only_current_services) {
		FindTransponder(xmlDocGetRootElement(parser)->xmlChildrenNode);
		xmlFreeDoc(parser);
	}
	if(stat(CURRENTSERVICES_XML,&testbuf) == 0)
	{
		if ((parser = parseXmlFile(CURRENTSERVICES_XML, false))) {
			printf("[getservices] " CURRENTSERVICES_XML "  found.\n");
			FindTransponder(xmlDocGetRootElement(parser)->xmlChildrenNode);
			xmlFreeDoc(parser);
		}
	}

	if (!only_current_services) {
		if(stat(MYSERVICES_XML,&testbuf) == 0)
		{
			if ((parser = parseXmlFile(MYSERVICES_XML, false))) {
				printf("[getservices] " MYSERVICES_XML "  found.\n");
				FindTransponder(xmlDocGetRootElement(parser)->xmlChildrenNode);
				xmlFreeDoc(parser);
			}
		}
	}	

	return 0;
}

