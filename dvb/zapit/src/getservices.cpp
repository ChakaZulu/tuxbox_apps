/*
 * $Id: getservices.cpp,v 1.41 2002/08/27 15:46:23 obi Exp $
 */

#include <stdio.h>

#include <zapost/frontend.h>

#include "getservices.h"

uint8_t curr_diseqc = 0;

extern std::map <uint32_t, transponder> transponders;
extern std::map <uint32_t, CZapitChannel> allchans_tv;
extern std::map <uint32_t, uint32_t> numchans_tv;
extern std::map <std::string, uint32_t> namechans_tv;
extern std::map <uint32_t, CZapitChannel> allchans_radio;
extern std::map <uint32_t, uint32_t> numchans_radio;
extern std::map <std::string, uint32_t> namechans_radio;

void nameinsert (std::string name, uint16_t original_network_id, uint16_t service_id, uint8_t service_type)
{
	int number = 0;
	char cnumber[3];
	std::string newname = name;

	switch (service_type)
	{
	case DIGITAL_TELEVISION_SERVICE:
	case NVOD_REFERENCE_SERVICE:
		while (namechans_tv.count(newname) != 0)
		{
			sprintf(cnumber, "%2d", ++number);
			newname = name + cnumber;
		}

		namechans_tv.insert
		(
			std::pair <std::string, uint32_t>
			(
				newname,
				(original_network_id << 16) | service_id
			)
		);

		allchans_tv.find((original_network_id << 16) | service_id)->second.setName(newname);
		break;

	case DIGITAL_RADIO_SOUND_SERVICE:
		while (namechans_radio.count(newname) != 0)
		{
			sprintf(cnumber, "%2d", ++number);
			newname = name + cnumber;
		}

		namechans_radio.insert
		(
			std::pair <std::string, uint32_t>
			(
				newname,
				(original_network_id << 16) | service_id
			)
		);

		allchans_radio.find((original_network_id << 16) | service_id)->second.setName(newname);
		break;

	default:
		break;
	}
}

void ParseTransponders (XMLTreeNode *node, uint8_t DiSEqC)
{
	uint16_t transport_stream_id;
	uint16_t original_network_id;
	FrontendParameters feparams;
	uint8_t polarization = 0;
	uint8_t tmp;

	/* FIXME: get inversion from services list */
	feparams.Inversion = INVERSION_AUTO;

	/* read all transponders */
	while ((node != NULL) && (!strcmp(node->GetType(), "transponder")))
	{
		/* common */
		sscanf(node->GetAttributeValue("id"), "%hx", &transport_stream_id);
		sscanf(node->GetAttributeValue("onid"), "%hx", &original_network_id);
		sscanf(node->GetAttributeValue("frequency"), "%u", &feparams.Frequency);

		/* cable */
		if (DiSEqC == 0xFF)
		{
			sscanf(node->GetAttributeValue("symbol_rate"), "%u", &feparams.u.qam.SymbolRate);
			sscanf(node->GetAttributeValue("fec_inner"), "%hhu", &tmp);
			feparams.u.qam.FEC_inner = CFrontend::getFEC(tmp);
			sscanf(node->GetAttributeValue("modulation"), "%hhu", &tmp);
			feparams.u.qam.QAM = CFrontend::getModulation(tmp);
		}

		/* satellite */
		else
		{
			sscanf(node->GetAttributeValue("symbol_rate"), "%u", &feparams.u.qpsk.SymbolRate);
			sscanf(node->GetAttributeValue("fec_inner"), "%hhu", &tmp);
			feparams.u.qam.FEC_inner = CFrontend::getFEC(tmp);
			sscanf(node->GetAttributeValue("polarization"), "%hhu", &polarization);
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

void ParseChannels (XMLTreeNode *node, uint16_t transport_stream_id, uint16_t original_network_id, uint8_t DiSEqC)
{
	uint16_t service_id;
	std::string name;
	uint16_t service_type;
	uint16_t channel_number;

	while ((node != NULL) && (!strcmp(node->GetType(), "channel")))
	{
		sscanf(node->GetAttributeValue("service_id"), "%hx", &service_id);
		name = node->GetAttributeValue("name");
		sscanf(node->GetAttributeValue("service_type"), "%hd", &service_type);
		sscanf(node->GetAttributeValue("channel_nr"), "%hd", &channel_number);

		switch (service_type)
		{
		case DIGITAL_TELEVISION_SERVICE:
		case NVOD_REFERENCE_SERVICE:
			allchans_tv.insert
			(
				std::pair <uint32_t, CZapitChannel>
				(
					(original_network_id << 16) | service_id,
					CZapitChannel
					(
						name,
						service_id,
						transport_stream_id,
						original_network_id,
						service_type,
						channel_number,
						DiSEqC
					)
				)
			);

			if (channel_number != 0)
			{
				numchans_tv.insert
				(
					std::pair <uint16_t, uint32_t>
					(
						channel_number,
						(original_network_id << 16) | service_id
					)
				);
			}
			else
			{
				nameinsert(name, original_network_id, service_id, service_type);
			}

			break;

		case DIGITAL_RADIO_SOUND_SERVICE:
			allchans_radio.insert
			(
				std::pair <uint32_t, CZapitChannel>
				(
					(original_network_id << 16) | service_id,
					CZapitChannel
					(
						name,
						service_id,
						transport_stream_id,
						original_network_id,
						service_type,
						channel_number,
						DiSEqC
					)
				)
			);

			if (channel_number != 0)
			{
				numchans_radio.insert
				(
					std::pair <uint16_t, uint32_t>
					(
						channel_number,
						(original_network_id << 16) | service_id
					)
				);
			}
			else
			{
				nameinsert(name, original_network_id, service_id, service_type);
			}
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
			sscanf(search->GetAttributeValue("diseqc"), "%hhu", &DiSEqC);
			ParseTransponders(search->GetChild(), DiSEqC);
		}

		/* hop to next satellite */
		search = search->GetNext();
	}
}

int LoadServices(void)
{
	char buf[2048];
	bool done;
	size_t len;

	XMLTreeParser *parser = new XMLTreeParser("ISO-8859-1");
	FILE *in = fopen(CONFIGDIR "/zapit/services.xml", "r");

	if (!in)
	{
		perror("[getservices.cpp] " CONFIGDIR "/zapit/services.xml");
		return -1;
	}

	do
	{
		len = fread(buf, 1, sizeof(buf), in);
		done = len < sizeof(buf);

		if (!parser->Parse(buf, len, done))
		{
			printf("[getservices.cpp] parse error: %s at line %d\n", parser->ErrorString(parser->GetErrorCode()), parser->GetCurrentLineNumber());
			fclose(in);
			delete parser;
			return -1;
		}
	}
	while (!done);

	if (parser->RootNode())
	{
		FindTransponder(parser->RootNode()->GetChild());
	}

	fclose(in);
	delete parser;
	return 0;
}

