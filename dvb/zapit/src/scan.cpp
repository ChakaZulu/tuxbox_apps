/*
 * $Id: scan.cpp,v 1.87 2002/12/17 22:02:37 obi Exp $
 */

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/* libevent */
#include <eventserver.h>

#include <zapit/bat.h>
#include <zapit/bouquets.h>
#include <zapit/client/zapitclient.h>
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/nit.h>
#include <zapit/pat.h>
#include <zapit/scan.h>
#include <zapit/sdt.h>
#include <zapit/settings.h>
#include <zapit/xmlinterface.h>

short scan_runs;
short curr_sat;
static int status = 0;



CBouquetManager* scanBouquetManager;

extern tallchans allchans;   //  defined in zapit.cpp
extern int found_transponders;
extern int found_channels;
extern std::map <t_channel_id, uint8_t> service_types;

/* zapit.cpp */
extern CFrontend *frontend;
extern XMLTreeParser *scanInputParser;
extern std::map <uint8_t, std::string> scanProviders;
extern CZapitClient::bouquetMode bouquetMode;

extern CEventServer *eventServer;

void stop_scan()
{
	/* notify client about end of scan */
	scan_runs = 0;
	eventServer->sendEvent(CZapitClient::EVT_SCAN_COMPLETE, CEventServer::INITID_ZAPIT);
	if (scanBouquetManager)
	{
		for (vector<CBouquet*>::iterator it = scanBouquetManager->Bouquets.begin(); it != scanBouquetManager->Bouquets.end(); it++)
		{
			for (vector<CZapitChannel*>::iterator jt = (*it)->tvChannels.begin(); jt != (*it)->tvChannels.end(); jt++)
				delete (*jt);
			for (vector<CZapitChannel*>::iterator jt = (*it)->radioChannels.begin(); jt != (*it)->radioChannels.end(); jt++)
				delete (*jt);
		}
		scanBouquetManager->clearAll();
		delete scanBouquetManager;
	}
}


int bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun (uint32_t TsidOnid, dvb_frontend_parameters feparams, uint8_t polarity, uint8_t DiSEqC)
{
	if (scantransponders.find(TsidOnid) == scantransponders.end())
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
			std::pair <unsigned int, transpondermap>
			(
				TsidOnid,
				transpondermap
				(
					(TsidOnid >> 16),
					TsidOnid,
					feparams,
					polarity,
					DiSEqC
				)
			)
		);

		return 0;
	}

	return 1;
}


/* build transponder for cable-users with sat-feed*/
int build_bf_transponder(uint32_t frequency, uint32_t symbol_rate, fe_code_rate_t fec_inner, fe_modulation_t modulation)
{
	dvb_frontend_parameters feparams;

	if (frontend->getInfo()->type != FE_QAM)
		return -1;

	feparams.frequency = frequency;
	feparams.inversion = INVERSION_AUTO;
	feparams.u.qam.symbol_rate = symbol_rate;
	feparams.u.qam.fec_inner = fec_inner;
	feparams.u.qam.modulation = modulation;

	if (!frontend->tuneFrequency(&feparams, 0, 0))
		return -1;

	return bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun(get_sdt_TsidOnid(), feparams, 0, 0);
}


int get_nits (uint32_t frequency, uint32_t symbol_rate, fe_code_rate_t fec_inner, uint8_t polarization, uint8_t DiSEqC, fe_modulation_t modulation)
{
	dvb_frontend_parameters feparams;
	feparams.frequency = frequency;
	feparams.inversion = INVERSION_AUTO;

	switch (frontend->getInfo()->type) {
	case FE_QPSK:
		feparams.u.qpsk.symbol_rate = symbol_rate;
		feparams.u.qpsk.fec_inner = fec_inner;
		break;

	case FE_QAM:
		feparams.u.qam.symbol_rate = symbol_rate;
		feparams.u.qam.fec_inner = fec_inner;
		feparams.u.qam.modulation = modulation;
		break;

	case FE_OFDM:
	default:
		return -1;
	}

	if (!frontend->tuneFrequency(&feparams, polarization, DiSEqC))
		return -1;

	if ((status = parse_nit(DiSEqC)) <= -2) /* nit unavailable */
		status = bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun(get_sdt_TsidOnid(), feparams, polarization, DiSEqC);

	return status;
}


int get_sdts()
{
	stiterator tI;

	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++) {

		if (!frontend->tuneFrequency(&tI->second.feparams, tI->second.polarization, tI->second.DiSEqC))
			continue;

		INFO("parsing SDT (tsid:onid %04x:%04x)", tI->second.transport_stream_id, tI->second.original_network_id);

		parse_sdt(tI->second.transport_stream_id, tI->second.original_network_id, tI->second.DiSEqC);
	}

	return 0;
}

FILE *write_xml_header (const char *filename)
{
	FILE *fd = fopen(filename, "w");

	if (fd == NULL)
	{
		ERROR(filename);
		stop_scan();
		pthread_exit(0);
	}

	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");

	return fd;
}

int write_xml_footer(FILE *fd)
{
	if (fd != NULL)	{
		fprintf(fd, "</zapit>\n");
		return fclose(fd);
	}

	return -1;
}

void write_bouquets()
{
	if (bouquetMode == CZapitClient::BM_DELETEBOUQUETS) {
		INFO("removing existing bouquets");
		unlink(BOUQUETS_XML);
	}

	else if ((bouquetMode == CZapitClient::BM_DONTTOUCHBOUQUETS))
		INFO("leaving bouquets untouched");

	else
		scanBouquetManager->saveBouquets();
}

void write_transponder(FILE *fd, t_transport_stream_id transport_stream_id, t_original_network_id original_network_id, uint8_t diseqc)
{
	stiterator tI = scantransponders.find((transport_stream_id << 16) | original_network_id);

	switch (frontend->getInfo()->type) {
	case FE_QAM: /* cable */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hhu\" symbol_rate=\"%u\" fec_inner=\"%hhu\" modulation=\"%hhu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.qam.symbol_rate,
			tI->second.feparams.u.qam.fec_inner,
			tI->second.feparams.u.qam.modulation);
		break;

	case FE_QPSK: /* satellite */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hhu\" symbol_rate=\"%u\" fec_inner=\"%hhu\" polarization=\"%hhu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.qpsk.symbol_rate,
			tI->second.feparams.u.qpsk.fec_inner,
			tI->second.polarization);
		break;

	case FE_OFDM: /* terrestrial */
	default:
		return;
	}

	for (tallchans::const_iterator cI = allchans.begin(); cI != allchans.end(); cI++)
		if ((cI->second.getTransportStreamId() == transport_stream_id) && (cI->second.getOriginalNetworkId() == original_network_id)) {
			if (cI->second.getName().length() == 0)
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%04x\" service_type=\"%02x\"/>\n",
					cI->second.getServiceId(),
					cI->second.getServiceId(),
					cI->second.getServiceType());
			else
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%02x\"/>\n",
					cI->second.getServiceId(),
					convert_UTF8_To_UTF8_XML(cI->second.getName()).c_str(),
					cI->second.getServiceType());
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
			fd = write_xml_header(SERVICES_XML);
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
	allchans.clear();                  // different provider may have the same onid/sid pair // FIXME
	scantransponders.clear();

	return fd;
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
		WARN("unable to scan without a frontend");
		stop_scan();
		pthread_exit(0);
	}

	switch (frontend->getInfo()->type) {
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
		/* get name of current satellite oder cable provider */
		strcpy(providerName, search->GetAttributeValue("name"));

		/* look whether provider is wanted */
		for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
		{
			if (!strcmp(spI->second.c_str(), providerName))
				break;
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

		/* satellite receivers might need diseqc */
		if (frontend->getInfo()->type == FE_QPSK)
			diseqc_pos = spI->first;

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
				status = build_bf_transponder(frequency, symbol_rate, (fe_code_rate_t) fec_inner, CFrontend::getModulation(modulation));
			else
				/* read network information table */
				status = get_nits(frequency, symbol_rate, (fe_code_rate_t) fec_inner, polarization, diseqc_pos, CFrontend::getModulation(modulation));
			/* next transponder */
			transponder = transponder->GetNext();
		}

		/* 
		 * parse:
		 * service description table,
		 * program association table,
		 * bouquet association table,
		 * network information table
		 */
		status = get_sdts();

		/*
		 * channels from PAT do not have service_type set.
		 * some channels set the service_type in the BAT or the NIT.
		 * should the NIT be parsed on every transponder?
		 */
		std::map <t_channel_id, uint8_t>::iterator stI;
		for (stI = service_types.begin(); stI != service_types.end(); stI++)
		{
			tallchans_iterator scI = allchans.find(stI->first);

			if (scI != allchans.end())
			{
				if (scI->second.getServiceType() != stI->second)
				{
					INFO("setting service_type of channel_id " PRINTF_CHANNEL_ID_TYPE " from %02x to %02x",
							stI->first,
							scI->second.getServiceType(),
							stI->second);

					scI->second.setServiceType(stI->second);
				}
			}
		}

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
	INFO("found %d transponders and %d channels", found_transponders, found_channels);

	/* load new services */
	CZapitClient myZapitClient;
	myZapitClient.reinitChannels();

	stop_scan();
	pthread_exit(0);
}
