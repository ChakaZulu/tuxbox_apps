/*
 * $Id: scan.cpp,v 1.51 2002/06/27 19:46:00 Homar Exp $
 */

#include <clientlib/zapitclient.h>
#include <xml/xmltree.h>
#include <zapost/frontend.h>
#include <zapsi/nit.h>
#include <zapsi/pat.h>
#include <zapsi/sdt.h>

#include "bouquets.h"
#include "scan.h"
#include "zapit.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#else
#define CONFIGDIR "/var/tuxbox/config"
#endif

typedef std::map <uint32_t, scanchannel>::iterator sciterator;
typedef std::map <uint32_t, transpondermap>::iterator stiterator;
typedef std::multimap <std::string, bouquet_mulmap>::iterator sbiterator;

short scan_runs;
short curr_sat;

CBouquetManager* scanBouquetManager;

extern int found_transponders;
extern int found_channels;

/* zapit.cpp */
extern CFrontend *frontend;
extern XMLTreeParser *scanInputParser;
extern std::map <uint8_t, std::string> scanProviders;
extern CZapitClient::bouquetMode bouquetMode;

extern CEventServer *eventServer;

/* build transponder for cable-users with sat-feed*/
void build_bf_transponder(uint32_t frequency, uint32_t symbol_rate, CodeRate FEC_inner, Modulation modulation)
{
	FrontendParameters feparams;
	feparams.Frequency = frequency;
	feparams.Inversion = INVERSION_AUTO;
	if (frontend->getInfo()->type == FE_QPSK)
	{
		feparams.u.qpsk.SymbolRate = symbol_rate;
		feparams.u.qpsk.FEC_inner = FEC_inner;
	}
	else
	{
		feparams.u.qam.SymbolRate = symbol_rate;
		feparams.u.qam.FEC_inner = FEC_inner;
		feparams.u.qam.QAM = modulation;
	}

	if (frontend->tuneFrequency(feparams, 0, 0) == true)
	{
		uint16_t onid = get_onid();
		fake_pat(onid, feparams,0,0);
	}
	else
	{
		printf("No signal found on transponder\n");
	}
}

int get_nits (uint32_t frequency, uint32_t symbol_rate, CodeRate FEC_inner, uint8_t polarity, uint8_t DiSEqC, Modulation modulation)
{
	FrontendParameters feparams;
	feparams.Frequency = frequency;
	feparams.Inversion = INVERSION_AUTO;

	if (frontend->getInfo()->type == FE_QPSK)
	{
		feparams.u.qpsk.SymbolRate = symbol_rate;
		feparams.u.qpsk.FEC_inner = FEC_inner;
	}
	else
	{
		feparams.u.qam.SymbolRate = symbol_rate;
		feparams.u.qam.FEC_inner = FEC_inner;
		feparams.u.qam.QAM = modulation;
	}

	if (frontend->tuneFrequency(feparams, polarity, DiSEqC) == true)
	{
		if(parse_nit(DiSEqC) == -2)
		{
			uint16_t onid = get_onid();
			fake_pat(onid, feparams, polarity, DiSEqC);
		}

		return 0;
	}
	else
	{
		printf("No signal found on transponder\n");
		return -1;
	}
}

void get_sdts()
{
	stiterator tI;

	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
	{
		if (frontend->tuneFrequency(tI->second.feparams, tI->second.polarization, tI->second.DiSEqC) == true)
		{
			printf("[scan.cpp] parsing sdt of tsid %04x, onid %04x\n", tI->second.transport_stream_id, tI->second.original_network_id);
			parse_sdt();
		}
		else
		{
			printf("[scan.cpp] No signal found on transponder\n");
		}
	}
}

FILE *write_xml_header (const char *filename)
{
	FILE *fd = fopen(filename, "w");

	if (fd == NULL)
	{
		perror("[scan.cpp] fopen");
		scan_runs = 0;
		pthread_exit(0);
	}
	else
	{
		fprintf(fd, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n<zapit>\n");
	}

	return fd;
}

int write_xml_footer(FILE *fd)
{
	if (fd != NULL)
	{
		fprintf(fd, "</zapit>\n");
		return fclose(fd);
	}
	else
	{
		return -1;
	}
}

void write_bouquets()
{
	std::string oldname = "";

	/*
	mode&1024 - loesche bouquets und erstelle sich nicht neu
	mode&512 - erstelle bouquets immer neu
	mode&256 - keine aenderung an bouqets
	*/

	if (bouquetMode == CZapitClient::BM_DELETEBOUQUETS)
	{
		printf("[zapit] removing existing bouqets.xml\n");
		system("/bin/rm " CONFIGDIR "/zapit/bouquets.xml");
		return;
	}
	else if ((bouquetMode == CZapitClient::BM_DONTTOUCHBOUQUETS)/* || (scanbouquets.empty())*/)
	{
		printf("[zapit] leavin bouquets.xml untouched\n");
		return;
	}
	else
	{
		printf("[zapit] creating new bouquets.xml\n");
		scanBouquetManager->cleanUp();
		scanBouquetManager->saveBouquets();
	}
	return;
}

void write_transponder(FILE *fd, uint16_t transport_stream_id, uint16_t original_network_id, uint8_t diseqc)
{
	stiterator tI = scantransponders.find((transport_stream_id << 16) | original_network_id);

	switch (frontend->getInfo()->type)
	{
	case FE_QAM: /* cable */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" symbol_rate=\"%u\" fec_inner=\"%hhu\" modulation=\"%hhu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.Frequency,
			tI->second.feparams.u.qam.SymbolRate,
			tI->second.feparams.u.qam.FEC_inner,
			tI->second.feparams.u.qam.QAM);
		break;

	case FE_QPSK: /* satellite */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" symbol_rate=\"%u\" fec_inner=\"%hhu\" polarization=\"%hhu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.Frequency,
			tI->second.feparams.u.qpsk.SymbolRate,
			tI->second.feparams.u.qpsk.FEC_inner,
			tI->second.polarization);
		break;

	default:
		return;
	}

	for (sciterator cI = scanchannels.begin(); cI != scanchannels.end(); cI++)
	{
		if ((cI->second.tsid == transport_stream_id) && (cI->second.onid == original_network_id))
		{
			if (cI->second.name.length() == 0)
			{
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%04x\" service_type=\"%04x\" channel_nr=\"0\"/>\n",
					cI->second.sid,
					cI->second.sid,
					cI->second.service_type);
			}
			else
			{
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%04x\" channel_nr=\"0\"/>\n",
					cI->second.sid,
					cI->second.name.c_str(),
					cI->second.service_type);
			}
		}
	}

	fprintf(fd, "\t\t</transponder>\n");

	return;
}

FILE *write_provider(FILE *fd, const char *type, const char *provider_name, const uint8_t DiSEqC)
{
	if (!scantransponders.empty())
	{
		/* create new file if needed */
		if (fd == NULL)
		{
			fd = write_xml_header(CONFIGDIR "/zapit/services.xml");
		}

		/* cable tag */
		if (!strcmp(type, "cable"))
		{
			fprintf(fd, "\t<%s name=\"%s\">\n", type, provider_name);
		}

		/* satellite tag */
		else
		{
			fprintf(fd, "\t<%s name=\"%s\" diseqc=\"%hhd\">\n", type, provider_name, DiSEqC);
		}

		/* channels */
		for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
		{
			write_transponder(fd, tI->second.transport_stream_id, tI->second.original_network_id, DiSEqC);
		}

		/* end tag */
		fprintf(fd, "\t</%s>\n", type);
	}

	/* clear results for next provider */
	scanchannels.clear();
	scantransponders.clear();

	return fd;
}

void stop_scan()
{
	/* notify client about end of scan */
	scan_runs = 0;
	eventServer->sendEvent(CZapitClient::EVT_SCAN_COMPLETE, CEventServer::INITID_ZAPIT);
	if (scanBouquetManager)
		delete scanBouquetManager;
}

void *start_scanthread(void *param)
{
	FILE *fd = NULL;

	char providerName[32];
	char type[8];

	uint8_t diseqc_pos = 0;

	uint32_t frequency;
	uint32_t symbol_rate;
	uint8_t polarization;
	uint8_t fec_inner;
	uint8_t modulation;

	bool satfeed = false;

	scanBouquetManager = new CBouquetManager();

	curr_sat = 0;
	if ((frontend == NULL) || (frontend->isInitialized() == false))
	{
		printf("[scan.cpp] unable not scan without a frontend \n");
		stop_scan();
		pthread_exit(0);
	}

	switch (frontend->getInfo()->type)
	{
	case FE_QPSK:	/* satellite frontend */
		strcpy(type, "sat");
		modulation = 0;
		break;

	case FE_QAM:	/* cable frontend */
		strcpy(type, "cable");
		polarization = 0;
		break;

	default:	/* unsupported frontend */
		stop_scan();
		pthread_exit(0);
	}

	/* get first child */
	XMLTreeNode *search = scanInputParser->RootNode()->GetChild();
	XMLTreeNode *transponder = NULL;

	std::map <uint8_t, std::string>::iterator spI;

	/* read all sat or cable sections */
	while ((search) && (!strcmp(search->GetType(), type)))
	{
		/*
		 * notify client about start - yes should be outside
		 * of this loop but fuckig pthread doesn't recognize
		 * it
		 */
		scan_runs = 1;

		/* get name of current satellite oder cable provider */
		strcpy(providerName, search->GetAttributeValue("name"));

		/* look whether provider is wanted */
		for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
		{
			if (!strcmp(spI->second.c_str(), providerName))
			{
				break;
			}
		}

		/* provider is not wanted - jump to the next one */
		if (spI == scanProviders.end())
		{
			search = search->GetNext();
			continue;
		}

		/* Special mode for cable-users with sat-feed*/
		if (!strcmp(type, "cable") && search->GetAttributeValue("satfeed"))
			if (!strcmp(search->GetAttributeValue("satfeed"),"true"))
				satfeed = true;


		/* increase sat counter */
		curr_sat++;

		/* satellite tuners might need diseqc */
		if (frontend->getInfo()->type == FE_QPSK)
		{
			diseqc_pos = spI->first;
		}

		/* send sat name to client */
		eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &providerName, strlen(providerName) + 1);
		transponder = search->GetChild();

		/* read all transponders */
		while ((transponder) && (!strcmp(transponder->GetType(), "transponder")))
		{
			/* generic */
			sscanf(transponder->GetAttributeValue("frequency"), "%u", &frequency);
			sscanf(transponder->GetAttributeValue("symbol_rate"), "%u", &symbol_rate);
			sscanf(transponder->GetAttributeValue("fec_inner"), "%hhu", &fec_inner);

			/* cable */
			if (frontend->getInfo()->type == FE_QAM)
			{
				sscanf(transponder->GetAttributeValue("modulation"), "%hhu", &modulation);
			}

			/* satellite */
			else
			{
				sscanf(transponder->GetAttributeValue("polarization"), "%hhu", &polarization);
			}

			if (!strcmp(type,"cable") && satfeed)
				/* build special transponder for cable with satfeed*/
				build_bf_transponder(frequency, symbol_rate, CFrontend::getFEC(fec_inner), CFrontend::getModulation(modulation));
			else
				/* read network information table */
				get_nits(frequency, symbol_rate, CFrontend::getFEC(fec_inner), polarization, diseqc_pos, CFrontend::getModulation(modulation));

			/* next transponder */
			transponder = transponder->GetNext();
		}

		/* read service description table */
		get_sdts();

		/* write services */
		fd = write_provider(fd, type, providerName, diseqc_pos);

		/* go to next satellite */
		search = search->GetNext();
	}

	/* clean up - should this be done before every GetNext() ? */
	delete transponder;
	delete search;

	/* close xml tags */
	if (write_xml_footer(fd) != -1)
	{
		/* write bouquets if channels did not fail */
		write_bouquets();
	}

	/* report status */
	printf("[scan.cpp] found %d transponders and %d channels\n", found_transponders, found_channels);

	/* load new services */
	if (prepare_channels() < 0)
	{
		printf("[scan.cpp] Error parsing Services\n");
		stop_scan();
		pthread_exit(0);
	}
	else
	{
		printf("[scan.cpp] Channels have been loaded succesfully\n");
	}

	stop_scan();
	pthread_exit(0);
}

