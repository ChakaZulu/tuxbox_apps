/*
 * $Id: scan.cpp,v 1.39 2002/04/10 18:36:21 obi Exp $
 */

#include "scan.h"

typedef std::map<int, scanchannel>::iterator sciterator;
typedef std::map<int, transpondermap>::iterator stiterator;
typedef std::multimap<std::string, bouquet_mulmap>::iterator sbiterator;

short scan_runs;
short curr_sat;

extern int found_transponders;
extern int found_channels;

int issatbox()
{
	FILE *fp;
	char buffer[100];
	int fe = -1;
	fp = fopen("/proc/bus/dbox", "r");

	if (fp == NULL)
		return -1;

	while (!feof(fp))
	{
		fgets(buffer, 100, fp);
		sscanf(buffer, "fe=%d", &fe);
	}

	fclose(fp);

	return fe;
}

int get_nits(uint32_t frequency, uint32_t symbol_rate, CodeRate FEC_inner, uint8_t polarity, uint8_t DiSEqC, Modulation modulation)
{
	FrontendParameters feparams;
	feparams.Frequency = frequency;
	feparams.Inversion = INVERSION_AUTO;

	if (DiSEqC < 0xff)
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

	if (finaltune(feparams, polarity, DiSEqC) == 0)
	{
		nit(DiSEqC);
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
	int sdt_tries;
	stiterator tI;

	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
	{
		sdt_tries = 0;

		if (finaltune(tI->second.feparams, tI->second.polarization, tI->second.DiSEqC) == 0)
		{
			printf("[scan.cpp] looking up SDT of transport_stream_id %04x\n", tI->second.transport_stream_id);

			while ((sdt(tI->second.transport_stream_id, true) == -2) && (sdt_tries != 5))
			{
				sdt_tries++;
			}
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

void write_bouquets(unsigned short mode)
{
	FILE *bouq_fd;
	std::string oldname = "";

	/*
	mode&1024 - loesche bouquets und erstelle sich nicht neu
	mode&512 - erstelle bouquets immer neu
	mode&256 - keine aenderung an bouqets
	*/

	if (mode & 1024)
	{
		printf("[zapit] removing existing bouqets.xml\n");
		system("/bin/rm " CONFIGDIR "/zapit/bouquets.xml");
		scanbouquets.clear();
		return;
	}

	if ((mode & 256) || (scanbouquets.empty()))
	{
		printf("[zapit] leavin bouquets.xml untouched\n");
		scanbouquets.clear();
		return;
	}
	else
	{
		printf("[zapit] creating new bouquets.xml\n");

		bouq_fd = write_xml_header(CONFIGDIR "/zapit/bouquets.xml");
		for (sbiterator bI = scanbouquets.begin(); bI != scanbouquets.end(); bI++)
      		{
      			if (bI->second.provname != oldname)
      			{
      				if (oldname != "")
      				{
      					fprintf(bouq_fd, "</Bouquet>\n");
      				}

      				fprintf(bouq_fd, "<Bouquet name=\"%s\">\n", bI->second.provname.c_str());

      			}
      			fprintf(bouq_fd, "\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n", bI->second.sid, bI->second.servname.c_str(), bI->second.onid);

      			oldname = bI->second.provname;
      		}

      		fprintf(bouq_fd, "</Bouquet>\n");
		write_xml_footer(bouq_fd);
      	}
      	scanbouquets.clear();
	return;
}

void write_transponder(FILE *fd, uint16_t transport_stream_id, uint8_t diseqc)
{
	stiterator tI = scantransponders.find(transport_stream_id);

	/* cable */
	if (diseqc == 0xFF)
	{
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%d\" symbol_rate=\"%d\" fec_inner=\"%d\" modulation\"%d\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.Frequency,
			tI->second.feparams.u.qam.SymbolRate,
			tI->second.feparams.u.qam.FEC_inner,
			tI->second.feparams.u.qam.QAM);
	}

	/* satellite */
	else
	{
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%d\" symbol_rate=\"%d\" fec_inner=\"%d\" polarization=\"%d\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.Frequency,
			tI->second.feparams.u.qpsk.SymbolRate,
			tI->second.feparams.u.qpsk.FEC_inner,
			tI->second.polarization);
	}

	for (sciterator cI = scanchannels.begin(); cI != scanchannels.end(); cI++)
	{
		if (cI->second.tsid == transport_stream_id)
		{
			if (cI->second.name.length() > 0)
			{
				fprintf(fd,
					"\t\t\t<channel service_id=\"%04x\" name=\"%s\" service_type=\"%04x\" channel_nr=\"0\"/>\n",
					cI->second.sid,
					cI->second.name.c_str(),
					cI->second.service_type);
			}
			else
			{
				fprintf(stderr, "[scan.cpp] skipping channel without name\n");
			}
		}
	}
	
	fprintf(fd, "\t\t</transponder>\n");

	return;
}

FILE *write_provider(FILE *fd, const char *type, const char *provider_name, const uint8_t diseqc_pos)
{
	if (!scantransponders.empty())
	{
		/* create new file if needed */
		if (fd == NULL)
			fd = write_xml_header(CONFIGDIR "/services.xml");

		/* cable tag */
		if (diseqc_pos == 0xFF)
		{
			fprintf(fd, "\t<%s name=\"%s\">\n", type, provider_name);
		}

		/* satellite tag */
		else
		{
			fprintf(fd, "\t<%s name=\"%s\" diseqc=\"%hhd\">\n", type, provider_name, diseqc_pos);
		}

		/* channels */
		for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
		{
			write_transponder(fd, tI->second.transport_stream_id, diseqc_pos);
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
}

void *start_scanthread(void *param)
{
	FILE *fd = NULL;
	int is_satbox = issatbox();
	char providerName[32];
	char filename[32];
	char type[8];

	unsigned short do_diseqc = *(unsigned short *) (param);
	uint8_t diseqc_pos = 0xFF;

	uint32_t frequency;
	uint32_t symbol_rate;
	uint8_t polarization;
	uint8_t fec_inner;
	uint8_t modulation;

	char buffer[2048];
	size_t length;
	size_t done;

	FILE *transponder_xml;

	/* notify client about start */
	curr_sat = 0;
	scan_runs = 1;

	/* could the box type be determined? */
	if (is_satbox == -1)
	{
		stop_scan();
		pthread_exit(0);
	}

	/* cable constants */
	if (!is_satbox)
	{
		strcpy(filename, CONFIGDIR "/cables.xml");
		strcpy(type, "cable");
		polarization = 0;
	}

	/* satellite constants */
	else
	{
		strcpy(filename, CONFIGDIR "/satellites.xml");
		strcpy(type, "sat");
		modulation = 0;
	}

	/* open list of transponders */
	transponder_xml = fopen(filename, "r");

	if (transponder_xml == NULL)
	{
		perror(filename);
		stop_scan();
		pthread_exit(0);
	}

	/* create xml parser */
	XMLTreeParser *parser = new XMLTreeParser("ISO-8859-1");

	/* read list of transponders */
	do
	{
		length = fread(buffer, 1, sizeof(buffer), transponder_xml);
		done = length < sizeof(buffer);

		if (!parser->Parse(buffer, length, done))
		{
			printf("[scan.cpp] parse error: %s at line %d\n", parser->ErrorString(parser->GetErrorCode()), parser->GetCurrentLineNumber());
			fclose(transponder_xml);
			delete parser;
			stop_scan();
			pthread_exit(0);
		}
	}
	while(!done);

	/* close input */
	fclose(transponder_xml);

	/* search root element */
	if (!parser->RootNode())
	{
		delete parser;
		stop_scan();
		pthread_exit(0);
	}

	/* get first child */
	XMLTreeNode *search = parser->RootNode()->GetChild();
	XMLTreeNode *transponder = NULL;

	/* read all sat or cable sections */
	while ((search) && (!strcmp(search->GetType(), type)))
	{
		/* get name of current satellite oder cable provider */
		strcpy(providerName, search->GetAttributeValue("name"));

		// TODO: if providerName is in wanted list
		if ((!strcmp(providerName, "Astra 19.2E")) || (!strcmp(providerName, "Telekom")))
		{
			/* satellite might need diseqc */
			if (is_satbox)
			{
				// TODO: get dieseqc position from client
				diseqc_pos = 0;
			}

			/* increase sat counter */
			curr_sat++;

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
				if (!is_satbox)
				{
					sscanf(transponder->GetAttributeValue("modulation"), "%hhu", &modulation);
				}

				/* satellite */
				else
				{
					sscanf(transponder->GetAttributeValue("polarization"), "%hhu", &polarization);
				}

				/* read network information table */
				get_nits(frequency, symbol_rate, getFEC(fec_inner), polarization, diseqc_pos, getModulation(modulation));

				/* next transponder */
				transponder = transponder->GetNext();
			}

			/* read service description table */
			get_sdts();

			/* write services */
			fd = write_provider(fd, type, providerName, diseqc_pos);
		}

		/* go to next satellite */
		search = search->GetNext();
	}

	/* clean up - should this be done before every GetNext() ? */
	delete transponder;
	delete search;
	delete parser;

	/* close xml tags */
	if (write_xml_footer(fd) != -1)
	{
		/* write bouquets if channels did not fail */
		write_bouquets(do_diseqc);
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

