/*
 * $Id: getservices.cpp,v 1.64 2002/12/22 22:56:57 thegoodguy Exp $
 */

#include <stdio.h>

#include <zapit/bouquets.h>
#include <zapit/channel.h>
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/getservices.h>
#include <zapit/settings.h>
#include <zapit/xmlinterface.h>

extern std::map <uint32_t, transponder> transponders;
extern tallchans allchans;


#define GET_ATTR(node, name, fmt, arg)					\
	do {								\
		char * ptr = xmlGetAttribute(node, name);		\
		if ((ptr == NULL) || (sscanf(ptr, fmt, &arg) <= 0))	\
			arg = 0;					\
	}								\
	while (0)


void ParseTransponders(xmlNodePtr node, const uint8_t DiSEqC)
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	dvb_frontend_parameters feparams;
	uint8_t polarization = 0;
	uint8_t tmp;

	/* read all transponders */
	while ((node = xmlGetNextOccurence(node, "transponder")) != NULL)
	{
		/* common */
		GET_ATTR(node, "id", "%hx", transport_stream_id);
		GET_ATTR(node, "onid", "%hx", original_network_id);
		GET_ATTR(node, "frequency", "%u", feparams.frequency);
		GET_ATTR(node, "inversion", "%hhu", tmp);

		switch (tmp) {
		case 0:
			feparams.inversion = INVERSION_OFF;
			break;
		case 1:
			feparams.inversion = INVERSION_ON;
			break;
		default:
			feparams.inversion = INVERSION_AUTO;
			break;
		}

		/* cable */
		if (DiSEqC == 0xFF) {

			GET_ATTR(node, "symbol_rate", "%u", feparams.u.qam.symbol_rate);
			GET_ATTR(node, "fec_inner", "%hhu", tmp);
			feparams.u.qam.fec_inner = (fe_code_rate_t) tmp;
			GET_ATTR(node, "modulation", "%hhu", tmp);
			feparams.u.qam.modulation = CFrontend::getModulation(tmp);
		}

		/* terrestrial */
		else if (DiSEqC == 0xFE) {
			GET_ATTR(node, "bandwidth", "%hhu", tmp);
			feparams.u.ofdm.bandwidth = (fe_bandwidth_t) tmp;
			GET_ATTR(node, "code_rate_HP", "%hhu", tmp);
			feparams.u.ofdm.code_rate_HP = (fe_code_rate_t) tmp;
			GET_ATTR(node, "code_rate_LP", "%hhu", tmp);
			feparams.u.ofdm.code_rate_LP = (fe_code_rate_t) tmp;
			GET_ATTR(node, "constellation", "%hhu", tmp);
			feparams.u.ofdm.constellation = (fe_modulation_t) tmp;
			GET_ATTR(node, "transmission_mode", "%hhu", tmp);
			feparams.u.ofdm.transmission_mode = (fe_transmit_mode_t) tmp;
			GET_ATTR(node, "guard_interval", "%hhu", tmp);
			feparams.u.ofdm.guard_interval = (fe_guard_interval_t) tmp;
			GET_ATTR(node, "hierarchy_information", "%hhu", tmp);
			feparams.u.ofdm.hierarchy_information = (fe_hierarchy_t) tmp;
		}

		/* satellite */
		else {
			GET_ATTR(node, "symbol_rate", "%u", feparams.u.qpsk.symbol_rate);
			GET_ATTR(node, "fec_inner", "%hhu", tmp);
			feparams.u.qpsk.fec_inner = (fe_code_rate_t) tmp;
			GET_ATTR(node, "polarization", "%hhu", polarization);
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
		ParseChannels(node->xmlChildrenNode, transport_stream_id, original_network_id, DiSEqC);

		/* hop to next transponder */
		node = node->xmlNextNode;
	}

	return;
}

void ParseChannels(xmlNodePtr node, const t_transport_stream_id transport_stream_id, const t_original_network_id original_network_id, const unsigned char DiSEqC)
{
	t_service_id service_id;
	std::string  name;
	uint8_t      service_type;

	while ((node = xmlGetNextOccurence(node, "channel")) != NULL)
	{
		GET_ATTR(node, "service_id", "%hx", service_id);
		name = xmlGetAttribute(node, "name");
		GET_ATTR(node, "service_type", "%hhx", service_type);

		switch (service_type) {
		case DIGITAL_TELEVISION_SERVICE:
		case NVOD_REFERENCE_SERVICE:
		case NVOD_TIME_SHIFTED_SERVICE:
		case DIGITAL_RADIO_SOUND_SERVICE:
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
						CA_STATUS_FTA
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

	while (search)
	{
		if (!(strcmp(xmlGetName(search), "cable")))
			DiSEqC = 0xff;

		else if (!(strcmp(xmlGetName(search), "sat")))
			GET_ATTR(search, "diseqc", "%hhu", DiSEqC);

		else if (!(strcmp(xmlGetName(search), "terrestrial")))
			DiSEqC = 0xfe;

		else {
			search = search->xmlNextNode;
			continue;
		}

		INFO("going to parse dvb-%c provider %s", xmlGetName(search)[0], xmlGetAttribute(search, "name"));
		ParseTransponders(search->xmlChildrenNode, DiSEqC);
		search = search->xmlNextNode;
	}
}

int LoadServices(void)
{
	xmlDocPtr parser = parseXmlFile(string(SERVICES_XML));

	if (parser == NULL)
		return -1;

	FindTransponder(xmlDocGetRootElement(parser)->xmlChildrenNode);
	xmlFreeDoc(parser);
	return 0;
}

