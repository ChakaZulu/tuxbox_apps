/*
 * $Id: scan.cpp,v 1.133 2004/02/01 14:34:28 metallica Exp $
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

#include <fcntl.h>
#include <unistd.h>

/* libevent */
#include <eventserver.h>

#include <zapit/bouquets.h>
#include <zapit/client/zapitclient.h>
#include <zapit/debug.h>
#include <zapit/frontend.h>
#include <zapit/nit.h>
#include <zapit/scan.h>
#include <zapit/sdt.h>
#include <zapit/settings.h>
#include <zapit/xmlinterface.h>

short scan_runs;
short curr_sat;
static int status = 0;
uint processed_transponders;
uint32_t actual_freq;
uint actual_polarisation;

CBouquetManager* scanBouquetManager;

extern tallchans allchans;   //  defined in zapit.cpp
extern int found_transponders;
extern int found_channels;
extern std::map <t_channel_id, uint8_t> service_types;
extern uint32_t found_tv_chans;
extern uint32_t found_radio_chans;
extern uint32_t found_data_chans;

/* zapit.cpp */
extern CFrontend *frontend;
extern xmlDocPtr scanInputParser;

extern std::map <uint8_t, std::string> scanProviders;
std::map <uint8_t, std::string>::iterator spI;

extern std::map<t_satellite_position, uint8_t> motorPositions;
extern std::map<t_satellite_position, uint8_t>::iterator mpos_it;

extern std::map<string, t_satellite_position> satellitePositions;

extern CZapitClient::bouquetMode bouquetMode;
extern CEventServer *eventServer;
extern diseqc_t diseqcType;

extern int motorRotationSpeed;
TP_map_t TP_scanmap;

void write_xml_header(FILE * fd);
void write_xml_footer(FILE * fd);
int write_provider(FILE *fd, const char *frontendType, const char *provider_name, const uint8_t DiSEqC);

t_satellite_position driveMotorToSatellitePosition(char * providerName)
{
	t_satellite_position currentSatellitePosition = 0;
	t_satellite_position satellitePosition = 0;
	int waitForMotor = 0;

	/* position satellite dish if provider is on a different satellite */
	currentSatellitePosition = frontend->getCurrentSatellitePosition();
	satellitePosition = satellitePositions[providerName];
	printf("[scan] scanning now: %s\n", providerName);
	printf("[scan] currentSatellitePosition = %d, scanSatellitePosition = %d\n", currentSatellitePosition, satellitePosition);
	printf("[scan] motorPosition = %d\n", motorPositions[satellitePosition]);
	if ((currentSatellitePosition != satellitePosition) && (motorPositions[satellitePosition] != 0))
	{
		printf("[scan] start_scanthread: moving satellite dish from satellite position %d to %d\n", currentSatellitePosition, satellitePosition);
		printf("[scan] motorPosition = %d\n", motorPositions[satellitePosition]);
		frontend->positionMotor(motorPositions[satellitePosition]);
		waitForMotor = abs(satellitePosition - currentSatellitePosition) / motorRotationSpeed;
		printf("[zapit] waiting %d seconds for motor to turn satellite dish.\n", waitForMotor);
		eventServer->sendEvent(CZapitClient::EVT_ZAP_MOTOR, CEventServer::INITID_ZAPIT, &waitForMotor, sizeof(waitForMotor));
		sleep(waitForMotor);
		frontend->setCurrentSatellitePosition(satellitePosition);
	}

	return satellitePosition;
}

void cp(char * from, char * to)
{
	char cmd[256] = "cp -f ";
	strcat(cmd, from);
	strcat(cmd, " ");
	strcat(cmd, to);
	system(cmd);
}

void copy_to_satellite(FILE * fd, FILE * fd1, char * providerName)
{
	//copies services from previous services.xml file from start up to the sat that is being scanned...
	char buffer[256] = "";

	//look for sat to be scanned... or end of file
	fgets(buffer, 255, fd1);
	while(!feof(fd1) && !((strstr(buffer, "sat name") && strstr(buffer, providerName)) || strstr(buffer, "</zapit>")))
	{
		fputs(buffer, fd);
		fgets(buffer, 255, fd1);
	}

	// if not end of file
	if (!feof(fd1) && !strstr(buffer, "</zapit>"))
		// skip to end of satellite
		while (!feof(fd1) && !strstr(buffer, "</sat>"))
			fgets(buffer, 255, fd1);
}

void copy_to_end(FILE * fd, FILE * fd1)
{
	//copies the services from previous services.xml file from the end of sat being scanned to the end of the file...
	char buffer[256] ="";

	fgets(buffer, 255, fd1);
	while(!feof(fd1) && !strstr(buffer, "</zapit>"))
	{
		fputs(buffer, fd);
		fgets(buffer, 255, fd1);
	}
	fclose(fd1);
	unlink(SERVICES_TMP);
}

int append_service(char* providerName, TP_params* TP)
{
	FILE* fd;
	FILE* fd1;
	char* frontendType = getFrontendName();
 	t_satellite_position satellitePosition = 0;

	if (!frontendType)
		return 0;

	for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
		if (!strcmp(spI->second.c_str(), providerName))
			break;

	/* copy services.xml to /tmp directory */
	cp(SERVICES_XML, SERVICES_TMP);

	fd = fopen(SERVICES_XML, "w");

	if ((fd1 = fopen(SERVICES_TMP, "r")))
		copy_to_satellite(fd, fd1, providerName);
	else
		write_xml_header(fd);

	if (!strcmp(frontendType, "sat") && (frontend->getDiseqcType() == DISEQC_1_2))
		satellitePosition = driveMotorToSatellitePosition(providerName);

	/* write services */
	int scan_status = write_provider(fd, frontendType, providerName, TP->diseqc);

	if (fd1)
		copy_to_end(fd, fd1);

	write_xml_footer(fd);

	fclose(fd);
	fclose(fd1);
	return scan_status;
}

char* getFrontendName(void)
{
	if (!frontend)
		return NULL;

	switch (frontend->getInfo()->type) {
	case FE_QPSK:   /* satellite frontend */
		return "sat";
	case FE_QAM:    /* cable frontend */
		return "cable";
	case FE_OFDM:   /* terrestrial frontend */
		return "terrestrial";
	default:        /* unsupported frontend */
		return NULL;
	}
}

void stop_scan(const bool success)
{
	/* notify client about end of scan */
	scan_runs = 0;
	eventServer->sendEvent(success ? CZapitClient::EVT_SCAN_COMPLETE : CZapitClient::EVT_SCAN_FAILED, CEventServer::INITID_ZAPIT);
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

int add_transponder (TP_params *TP)
{
	if (frontend->setParameters(TP) < 0)
		return -1;

	if (TP_scanmap.find(TP->TP_id) == TP_scanmap.end())
	{
		found_transponders++;

		eventServer->sendEvent
		(
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CEventServer::INITID_ZAPIT,
			&found_transponders,
			sizeof(found_transponders)
		);
	 	eventServer->sendEvent
	 	(
			CZapitClient::EVT_SCAN_REPORT_FREQUENCY,
			CEventServer::INITID_ZAPIT,
			&(TP->feparams.frequency),
			sizeof(TP->feparams.frequency)
		);

		TP_scanmap.insert
		(
			std::pair <uint32_t, TP_map>
			(
				TP->TP_id,
				TP_map
				(
					*TP
				)
			)
		);

		return 0;
	}

	return -1;
}

void get_transponder (TP_params *TP)
{
	memcpy(TP,frontend->getParameters(),sizeof(TP_params));
	return;
}

int get_nit(TP_params* TP)
{
 	eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCY,CEventServer::INITID_ZAPIT, &(TP->feparams.frequency),sizeof(TP->feparams.frequency));

	if (frontend->setParameters(TP) < 0)
		return -1;

	if ((status = parse_nit(TP->diseqc)) <= -2) // nit unavailable
		status = add_transponder(TP);

	return status;
}

int get_sdt(void)
{
	uint32_t TsidOnid;
	TP_iterator tI;

	for (tI = TP_scanmap.begin(); tI != TP_scanmap.end(); tI++)
	{
		if (frontend->setParameters(&tI->second.TP) < 0)
			continue;

		TsidOnid = get_sdt_TsidOnid();
		parse_sdt(((TsidOnid >> 16)&0xFFFF), TsidOnid &0xFFFF, tI->second.TP.diseqc);
	}
	return 0;
}

void scan_transponder(TP_params *TP)
{
/* only for testing, please remove <- wtf? */
	add_transponder(TP);
	get_nit(TP);
	get_sdt();
	append_service("Test", TP);
	TP_scanmap.clear();
}

int bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun(uint32_t TsidOnid, struct dvb_frontend_parameters *feparams, uint8_t polarity, uint8_t DiSEqC)
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
					(TsidOnid >> 16) &0xFFFF,
					TsidOnid &0xFFFF,
					*feparams,
					polarity,
					DiSEqC
				)
			)
		);

		return 0;
	}

	return 1;
}

int get_nits(struct dvb_frontend_parameters *feparams, uint8_t polarization, uint8_t DiSEqC)
{
 	eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCY,CEventServer::INITID_ZAPIT, &(feparams->frequency),sizeof(feparams->frequency));

	if (frontend->setParameters(feparams, polarization, DiSEqC) < 0)
		return -1;

	if ((status = parse_nit(DiSEqC)) <= -2) /* nit unavailable */
		status = bla_hiess_mal_fake_pat_hat_aber_nix_mit_pat_zu_tun(get_sdt_TsidOnid(), feparams, polarization, DiSEqC);

	return status;
}

int get_sdts(char * frontendType)
{
	stiterator tI;

	for (tI = scantransponders.begin(); tI != scantransponders.end(); tI++) {
		/* msg to neutrino */
		processed_transponders++;

		actual_freq = tI->second.feparams.frequency;

		eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS, CEventServer::INITID_ZAPIT, &processed_transponders, sizeof(processed_transponders));
 		eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCY,CEventServer::INITID_ZAPIT, &actual_freq,sizeof(actual_freq));

 		if (!strcmp(frontendType, "sat"))
 		{
	 		actual_polarisation = (uint)tI->second.polarization;
 			eventServer->sendEvent(CZapitClient::EVT_SCAN_REPORT_FREQUENCYP,CEventServer::INITID_ZAPIT,&actual_polarisation,sizeof(actual_polarisation));
 		}

		if (frontend->setParameters(&tI->second.feparams, tI->second.polarization, tI->second.DiSEqC) < 0)
			continue;

		INFO("parsing SDT (tsid:onid %04x:%04x)", tI->second.transport_stream_id, tI->second.original_network_id);
 		parse_sdt(tI->second.transport_stream_id, tI->second.original_network_id, tI->second.DiSEqC);
	}

	return 0;
}

void write_xml_header(FILE * fd)
{
	fprintf(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<zapit>\n");
}

void write_xml_footer(FILE *fd)
{
	fprintf(fd, "</zapit>\n");
	fclose(fd);
}

void write_bouquets(const char * const providerName)
{
	if (bouquetMode == CZapitClient::BM_DELETEBOUQUETS)
	{
		INFO("removing existing bouquets");
		unlink(BOUQUETS_XML);
	}

	else if ((bouquetMode == CZapitClient::BM_DONTTOUCHBOUQUETS))
		INFO("leaving bouquets untouched");

	else
		scanBouquetManager->saveBouquets(bouquetMode, providerName);
}

void write_transponder(FILE *fd, t_transport_stream_id transport_stream_id, t_original_network_id original_network_id)
{
	stiterator tI = scantransponders.find((transport_stream_id << 16) | original_network_id);
	static bool emptyTransponder = false;
	switch (frontend->getInfo()->type) {
	case FE_QAM: /* cable */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hu\" symbol_rate=\"%u\" fec_inner=\"%hu\" modulation=\"%hu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.qam.symbol_rate,
			tI->second.feparams.u.qam.fec_inner,
			tI->second.feparams.u.qam.modulation);
		break;

	case FE_QPSK: /* satellite */
		emptyTransponder = true;
		break;

	case FE_OFDM: /* terrestrial */
		fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hu\" bandwidth=\"%hu\" code_rate_HP=\"%hu\" code_rate_LP=\"%hu\" constellation=\"%hu\" transmission_mode=\"%hu\" guard_interval=\"%hu\" hierarchy_information=\"%hu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.ofdm.bandwidth,
			tI->second.feparams.u.ofdm.code_rate_HP,
			tI->second.feparams.u.ofdm.code_rate_LP,
			tI->second.feparams.u.ofdm.constellation,
			tI->second.feparams.u.ofdm.transmission_mode,
			tI->second.feparams.u.ofdm.guard_interval,
			tI->second.feparams.u.ofdm.hierarchy_information);
		break;

	default:
		return;
	}

	for (tallchans::const_iterator cI = allchans.begin(); cI != allchans.end(); cI++)
		if ((cI->second.getTransportStreamId() == transport_stream_id) && (cI->second.getOriginalNetworkId() == original_network_id)) {
			if(emptyTransponder){
					fprintf(fd,
			"\t\t<transponder id=\"%04x\" onid=\"%04x\" frequency=\"%u\" inversion=\"%hu\" symbol_rate=\"%u\" fec_inner=\"%hu\" polarization=\"%hu\">\n",
			tI->second.transport_stream_id,
			tI->second.original_network_id,
			tI->second.feparams.frequency,
			tI->second.feparams.inversion,
			tI->second.feparams.u.qpsk.symbol_rate,
			tI->second.feparams.u.qpsk.fec_inner,
			tI->second.polarization);
			emptyTransponder = false;
			}
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
	if(!emptyTransponder){
	fprintf(fd, "\t\t</transponder>\n");
	}
	return;
}

int write_provider(FILE *fd, const char *frontendType, const char *provider_name, const uint8_t DiSEqC)
{
	int status = -1;

	if (!scantransponders.empty())
	{
		/* cable tag */
		if (!strcmp(frontendType, "cable"))
		{
			fprintf(fd, "\t<%s name=\"%s\">\n", frontendType, provider_name);
		}

		/* satellite tag */
		else
		{
			fprintf(fd, "\t<%s name=\"%s\" diseqc=\"%hd\">\n", frontendType, provider_name, DiSEqC);
		}

		/* channels */
		for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
		{
			write_transponder(fd, tI->second.transport_stream_id, tI->second.original_network_id);
		}

		/* end tag */
		fprintf(fd, "\t</%s>\n", frontendType);
		status = 0; // this indicates that services have been found and that bouquets should be written...
	}

	/* clear results for next provider */
	allchans.clear();                  // different provider may have the same onid/sid pair // FIXME
	scantransponders.clear();

	return status;
}

int scan_transponder(xmlNodePtr transponder, uint8_t diseqc_pos)
{
	uint8_t polarization = 0;
	struct dvb_frontend_parameters feparams;
	memset(&feparams, 0x00, sizeof(struct dvb_frontend_parameters));

	feparams.frequency = xmlGetNumericAttribute(transponder, "frequency", 0);
	feparams.inversion = INVERSION_AUTO;

	/* cable */
	if (frontend->getInfo()->type == FE_QAM)
	{
		feparams.u.qam.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		feparams.u.qam.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "fec_inner", 0);
		feparams.u.qam.modulation = (fe_modulation_t) xmlGetNumericAttribute(transponder, "modulation", 0);
		diseqc_pos = 0;
	}

	/* satellite */
	else if (frontend->getInfo()->type == FE_QPSK)
	{
		feparams.u.qpsk.symbol_rate = xmlGetNumericAttribute(transponder, "symbol_rate", 0);
		feparams.u.qpsk.fec_inner = (fe_code_rate_t) xmlGetNumericAttribute(transponder, "fec_inner", 0);
		polarization = xmlGetNumericAttribute(transponder, "polarization", 0);
	}

	/* terrestrial */
	else if (frontend->getInfo()->type == FE_OFDM)
	{
		feparams.u.ofdm.bandwidth = (fe_bandwidth_t) xmlGetNumericAttribute(transponder, "bandwidth", 0);
		feparams.u.ofdm.code_rate_HP = FEC_AUTO;
		feparams.u.ofdm.code_rate_LP = FEC_AUTO;
		feparams.u.ofdm.constellation = QAM_AUTO;
		feparams.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
		feparams.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
		feparams.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
	}

		/* read network information table */
	status = get_nits(&feparams, polarization, diseqc_pos);

	return 0;
}

void scan_provider(xmlNodePtr search, char * providerName, uint8_t diseqc_pos, char * frontendType)
{
	xmlNodePtr transponder = NULL;

	/* send sat name to client */
	eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, providerName, strlen(providerName) + 1);
	transponder = search->xmlChildrenNode;

	/* read all transponders */
	while ((transponder = xmlGetNextOccurence(transponder, "transponder")) != NULL)
	{
		scan_transponder(transponder, diseqc_pos);

		/* next transponder */
		transponder = transponder->xmlNextNode;
	}

	/*
	 * parse:
	 * service description table,
	 * program association table,
	 * bouquet association table,
	 * network information table
	 */
	status = get_sdts(frontendType);

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
				switch (scI->second.getServiceType()) {
				case ST_DIGITAL_TELEVISION_SERVICE:
				case ST_DIGITAL_RADIO_SOUND_SERVICE:
				case ST_NVOD_REFERENCE_SERVICE:
				case ST_NVOD_TIME_SHIFTED_SERVICE:
					break;
				default:
					INFO("setting service_type of channel_id " PRINTF_CHANNEL_ID_TYPE " from %02x to %02x",
						stI->first,
						scI->second.getServiceType(),
						stI->second);
					scI->second.setServiceType(stI->second);
					break;
				}
			}
		}
	}
}

void *start_scanthread(void *)
{
	FILE *fd = NULL;
	FILE *fd1 = NULL;
	char providerName[32] = "";
	char providerName2[32] = "";
	char *frontendType = NULL;
	uint8_t diseqc_pos = 0;
	int scan_status = -1;

	scanBouquetManager = new CBouquetManager();
	processed_transponders = 0;
 	found_tv_chans = 0;
 	found_radio_chans = 0;
 	found_data_chans = 0;

	curr_sat = 0;

        if ((frontendType = getFrontendName()) == NULL)
	{
		WARN("unable to scan without a supported frontend");
		stop_scan(false);
		pthread_exit(0);
	}

	/* get first child */
	xmlNodePtr search = xmlDocGetRootElement(scanInputParser)->xmlChildrenNode;
	xmlNodePtr transponder = NULL;

	if  (!strcmp(frontendType, "cable"))
	{
		fd = fopen(SERVICES_XML, "w");
		write_xml_header(fd);
	}

	/* read all sat or cable sections */
	while ((search = xmlGetNextOccurence(search, frontendType)) != NULL)
	{
		/* get name of current satellite oder cable provider */
		strcpy(providerName, xmlGetAttribute(search, "name"));

		/* look whether provider is wanted */
		for (spI = scanProviders.begin(); spI != scanProviders.end(); spI++)
			if (!strcmp(spI->second.c_str(), providerName))
				break;

		/* provider is not wanted - jump to the next one */
		if (spI != scanProviders.end())
		{
			strncpy(providerName2, providerName, 30);

			/* increase sat counter */
			curr_sat++;

			if  (!strcmp(frontendType, "sat"))
			{
				/* copy services.xml to /tmp directory */
				cp(SERVICES_XML, SERVICES_TMP);

				if (!(fd = fopen(SERVICES_XML, "w")))
				{
					WARN("unable to open %s for writing", SERVICES_XML);
					goto abort_scan;
				}
				if ((fd1 = fopen(SERVICES_TMP, "r")))
					copy_to_satellite(fd, fd1, providerName);
				else
					write_xml_header(fd);
			}

			/* satellite receivers might need diseqc */
			if (frontend->getInfo()->type == FE_QPSK)
				diseqc_pos = spI->first;
			if (diseqc_pos == 255 /* = -1 */)
				diseqc_pos = 0;

			if (!strcmp(frontendType, "sat") && (frontend->getDiseqcType() == DISEQC_1_2))
				driveMotorToSatellitePosition(providerName);

			scan_provider(search, providerName, diseqc_pos, frontendType);

			/* write services */
			scan_status = write_provider(fd, frontendType, providerName, diseqc_pos);

			if (!strcmp(frontendType, "sat"))
			{
				if (fd1)
					copy_to_end(fd, fd1);

				write_xml_footer(fd);
			}
		}

		/* go to next satellite */
		search = search->xmlNextNode;
	}

	if  (!strcmp(frontendType, "cable"))
		write_xml_footer(fd);

	chmod(SERVICES_XML, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	abort_scan:
	/* clean up - should this be done before every xmlNextNode ? */
	delete transponder;
	delete search;

	/* write bouquets if services were found */
	if (scan_status != -1)
		write_bouquets(providerName2);

	/* report status */
	INFO("found %d transponders and %d channels", found_transponders, found_channels);

	/* load new services */
	CZapitClient myZapitClient;
	myZapitClient.reinitChannels();

	stop_scan(true);
	pthread_exit(0);
}
