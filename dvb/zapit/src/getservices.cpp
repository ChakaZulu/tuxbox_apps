/*
 * $Id: getservices.cpp,v 1.56 2002/11/02 17:21:15 obi Exp $
 */

#include <stdio.h>

#include <zapit/bouquets.h>
#include <zapit/channel.h>
#include <zapit/frontend.h>
#include <zapit/getservices.h>
#include <zapit/settings.h>
#include <zapit/xmlinterface.h>

extern std::map <uint32_t, transponder> transponders;
extern tallchans allchans;


#define GET_ATTR(node, name, fmt, arg)					\
	do {								\
		char * ptr = node->GetAttributeValue(name);		\
		if ((ptr == NULL) || (sscanf(ptr, fmt, &arg) <= 0))	\
			arg = 0;					\
	}								\
	while (0)


void ParseTransponders (XMLTreeNode *node, uint8_t DiSEqC)
{
	t_transport_stream_id transport_stream_id;
	t_original_network_id original_network_id;
	dvb_frontend_parameters feparams;
	uint8_t polarization = 0;
	uint8_t tmp;

	/* read all transponders */
	while ((node != NULL) && (!strcmp(node->GetType(), "transponder"))) {

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
			feparams.u.qam.fec_inner = CFrontend::getCodeRate(tmp);
			GET_ATTR(node, "modulation", "%hhu", tmp);
			feparams.u.qam.modulation = CFrontend::getModulation(tmp);
		}

		/* satellite */
		else {
			GET_ATTR(node, "symbol_rate", "%u", feparams.u.qpsk.symbol_rate);
			GET_ATTR(node, "fec_inner", "%hhu", tmp);
			feparams.u.qpsk.fec_inner = CFrontend::getCodeRate(tmp);
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
		ParseChannels(node->GetChild(), transport_stream_id, original_network_id, DiSEqC);

		/* hop to next transponder */
		node = node->GetNext();
	}

	return;
}

void ParseChannels (XMLTreeNode *node, t_transport_stream_id transport_stream_id, t_original_network_id original_network_id, uint8_t DiSEqC)
{
	t_service_id service_id;
	std::string  name;
	uint8_t      service_type;

	while ((node != NULL) && (!strcmp(node->GetType(), "channel")))
	{
		GET_ATTR(node, "service_id", "%hx", service_id);
		name = node->GetAttributeValue("name");
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
						DiSEqC
					)
				)
			);

			break;

		default:
			break;
		}

		node = node->GetNext();
	}

	return;
}

void FindTransponder (XMLTreeNode *search)
{
	uint8_t DiSEqC;

	while (search)
	{
		/* cable */
		if (!(strcmp(search->GetType(), "cable")))
		{
			printf("[getservices.cpp] going to parse cable %s\n", search->GetAttributeValue("name"));
			ParseTransponders(search->GetChild(), 0xFF);
		}

		/* satellite */
		else if (!(strcmp(search->GetType(), "sat")))
		{
			printf("[getservices.cpp] going to parse satellite %s\n", search->GetAttributeValue("name"));
			GET_ATTR(search, "diseqc", "%hhu", DiSEqC);
			ParseTransponders(search->GetChild(), DiSEqC);
		}

		/* hop to next satellite */
		search = search->GetNext();
	}
}

int LoadServices(void)
{
	XMLTreeParser *parser = parseXmlFile(string(SERVICES_XML));

	if (parser == NULL)
		return -1;

	FindTransponder(parser->RootNode()->GetChild());
	delete parser;
	return 0;
}

