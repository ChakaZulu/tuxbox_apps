/*
$Id: scan.cpp,v 1.34 2002/04/04 23:40:13 obi Exp $



$Log: scan.cpp,v $
Revision 1.34  2002/04/04 23:40:13  obi
show number of found transponders / channels on console

Revision 1.33  2002/04/04 14:41:08  rasc
- New functions in zapitclient for handling favorites
  - test if a bouquet exists
- Some Log - CVS Entries in modules


*/


#include <stdio.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <ctype.h>
#include <string>
#include "descriptors.h"
#include "scan.h"

#include <ost/dmx.h>
#include <ost/frontend.h>
#include <ost/sec.h>
#include <ost/video.h>
#include <ost/ca.h>

#include "nit.h"
#include "pat.h"
#include "sdt.h"
#include "tune.h"


#define FRONT_DEV "/dev/ost/qpskfe0"
#define DEMUX_DEV "/dev/ost/demux0"
#define SEC_DEV   "/dev/ost/sec0"

typedef std::map<int, scanchannel>::iterator sciterator;
typedef std::map<int, transpondermap>::iterator stiterator;
typedef std::multimap<std::string, bouquet_mulmap>::iterator sbiterator;
std::string services_xml = CONFIGDIR "/zapit/services.xml";
std::string logfile = "/tmp/zapit_scan.log";
int prepare_channels();
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

void get_nits(uint32_t frequency, uint32_t symbol_rate, CodeRate FEC_inner, uint8_t polarity, uint8_t DiSEqC)
{
	FrontendParameters feparams;
	feparams.Frequency = frequency;
	feparams.Inversion = INVERSION_AUTO;
	feparams.u.qpsk.SymbolRate = symbol_rate;
	feparams.u.qpsk.FEC_inner = FEC_inner;

	if (finaltune(feparams, polarity, DiSEqC) == 0)
		nit(DiSEqC);
	else
		printf("No signal found on transponder\n");
}

void get_sdts()
{
	int sdt_tries;

	for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
	{
		sdt_tries = 0;

		if (finaltune(tI->second.feparams, tI->second.polarization, tI->second.DiSEqC) == 0)
		{
			printf("[scan.cpp] GETTING SDT FOR TSID: %04x\n", tI->second.transport_stream_id);
			
			while ((sdt(tI->second.transport_stream_id, true) == -2) && (sdt_tries != 5))
				sdt_tries++;
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
	fprintf(fd, "</zapit>\n");
	return fclose(fd);
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

void write_transponder(FILE *fd, uint16_t transport_stream_id)
{
	std::string transponder;
	stiterator tI = scantransponders.find(transport_stream_id);

	char frequency[9];
	char symbol_rate[9];
	char FEC_inner[2];
	char polarization[2];
	char modulation[2];

	char service_id[5];
	char original_network_id[5];
	char service_type[5];

	sprintf(frequency, "%8d", tI->second.feparams.Frequency);

	if (issatbox())
	{
		sprintf(symbol_rate, "%8d", tI->second.feparams.u.qpsk.SymbolRate);
		sprintf(FEC_inner, "%1d", tI->second.feparams.u.qpsk.FEC_inner);
		sprintf(polarization, "%1d", tI->second.polarization);

		transponder = "\t\t<sat ";
		transponder += "frequency=\"";
		transponder += frequency;
		transponder += "\" symbolRate=\"";
		transponder += symbol_rate;
		transponder += "\" fec=\"";
		transponder += FEC_inner;
		transponder += "\" polarity=\"";
		transponder += polarization;
	}
	else
	{
		sprintf(symbol_rate, "%05d", tI->second.feparams.u.qam.SymbolRate);
		sprintf(FEC_inner, "%01d", tI->second.feparams.u.qam.FEC_inner);
		sprintf(modulation, "%01d", tI->second.feparams.u.qam.QAM);

		transponder = "\t\t<cable ";
		transponder += "frequency=\"";
		transponder += frequency;
		transponder += "\" symbolRate=\"";
		transponder += symbol_rate;
		transponder += "\" fec=\"";
		transponder += FEC_inner;
		transponder += "\" modulation=\"";
		transponder += modulation;
	}

	transponder += "\"/>\n";

	for (sciterator cI = scanchannels.begin(); cI != scanchannels.end(); cI++)
	{
		if (cI->second.tsid == transport_stream_id)
		{
			sprintf(service_id, "%04x", cI->second.sid);
			sprintf(original_network_id, "%04x", cI->second.onid);
			sprintf(service_type, "%04x", cI->second.service_type);

			if (cI->second.name.length() > 0)
			{
				transponder += "\t\t<channel ServiceID=\"";
				transponder += service_id;
				transponder += "\" name=\"";
				transponder += cI->second.name;
				transponder += "\" onid=\"";
				transponder += original_network_id;
				transponder += "\" serviceType=\"";
				transponder += service_type;
				transponder += "\" channelNR=\"0\"/>\n";
				//transponder += "\"/>\n";
			}
		}
	}

	fprintf(fd, "\t<transponder transportID=\"%04x\">\n", transport_stream_id);
	fprintf(fd, "%s", transponder.c_str());
	fprintf(fd, "\t</transponder>\n");
	return;
}

FILE *write_sat(FILE *fd, const char *satname, const uint8_t diseqc_pos)
{
	if (!scantransponders.empty())
	{
		if (fd == NULL)
			fd = write_xml_header(services_xml.c_str());

		fprintf(fd, "<satellite name=\"%s\" diseqc=\"%hhd\">\n", satname, diseqc_pos);
		for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
		{
			write_transponder(fd, tI->second.transport_stream_id);
		}
		fprintf(fd, "</satellite>\n");
	}
	scanchannels.clear();
	scantransponders.clear();

	return fd;
}

void *start_scanthread(void *param)
{
	FILE *fd = NULL;
	std::string transponder;
	int is_satbox = issatbox();
	char satName[50];
	unsigned short do_diseqc = *(unsigned short *) (param);
	FrontendParameters feparams;
	uint8_t diseqc_pos; // TODO: get as parameter for each sat

	scan_runs = 1;

	if (is_satbox == -1)
	{
		scan_runs = 0;
		pthread_exit(0);
	}

	if (!is_satbox)
	{
		strcpy(satName, "cable");
		eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

		curr_sat = 0;
		feparams.Inversion = INVERSION_AUTO;
		feparams.u.qam.FEC_inner = FEC_AUTO;
		feparams.u.qam.QAM = QAM_64;

		for (feparams.Frequency = 306000; feparams.Frequency <= 460000; feparams.Frequency += 8000)
		{
			feparams.u.qam.SymbolRate = 6900000;

			if (finaltune(feparams, 0, 0) == 0)
			{
				fake_pat(&scantransponders, feparams);
			}
			else
			{
				printf("[scan.cpp] No signal found on transponder. Trying SymbolRate 6875000\n");

				feparams.u.qam.SymbolRate = 6875000;

				if (finaltune(feparams, 0, 0) == 0)
				{
					fake_pat(&scantransponders, feparams);
				}
				else
				{
					printf("[scan.cpp] No signal found on transponder\n");
				}
			}
		}
		
		feparams.Frequency = 522000;

		if (finaltune(feparams, 0, 0) == 0)
		{
			fake_pat(&scantransponders, feparams);
		}
		else
		{
			printf("[scan.cpp] No signal found on transponder.\n");
		}

		get_sdts();

		if (!scantransponders.empty())
		{
			fd = write_xml_header(services_xml.c_str());

			fprintf(fd,"<cable>\n");
			for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
			{
				write_transponder(fd, tI->second.transport_stream_id);
			}
			fprintf(fd,"</cable>\n");
		}

		scantransponders.clear();
		scanchannels.clear();
	}
	else
	{
		if (do_diseqc & 1)
		{
			diseqc_pos = 0;
			curr_sat = 1;
			strcpy(satName, "Astra 19.2E");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
			get_nits(10788000, 22000000, FEC_5_6, 1, diseqc_pos);	// 54
			get_nits(10832000, 22000000, FEC_5_6, 0, diseqc_pos);	// 57 **
			get_nits(10862000, 22000000, FEC_5_6, 0, diseqc_pos);	// 59
			get_nits(10876000, 22000000, FEC_5_6, 1, diseqc_pos);	// 60 **
#if 0
			get_nits(11719500, 27500000, FEC_3_4, 0, diseqc_pos);	// 65
			get_nits(11739500, 27500000, FEC_3_4, 1, diseqc_pos);	// 66
			get_nits(11758500, 27500000, FEC_3_4, 0, diseqc_pos);	// 67
			get_nits(11778000, 27500000, FEC_3_4, 1, diseqc_pos);	// 68
#endif
			get_nits(11798000, 27500000, FEC_3_4, 0, diseqc_pos);	// 69 *
#if 0
			get_nits(11817000, 27500000, FEC_3_4, 1, diseqc_pos);	// 70
			get_nits(11836500, 27500000, FEC_3_4, 0, diseqc_pos);	// 71
			get_nits(11856000, 27500000, FEC_3_4, 1, diseqc_pos);	// 72
			get_nits(11876000, 27500000, FEC_3_4, 0, diseqc_pos);	// 73
			get_nits(11895000, 27500000, FEC_3_4, 1, diseqc_pos);	// 74
#endif
			get_nits(11914000, 27500000, FEC_3_4, 0, diseqc_pos);	// 75 *
			//get_nits(11934000, 27500000, FEC_3_4, 1, diseqc_pos);	// 76
			get_nits(11953500, 27500000, FEC_3_4, 0, diseqc_pos);	// 77 *
#if 0
			get_nits(11973000, 27500000, FEC_3_4, 1, diseqc_pos);	// 78
			get_nits(11992500, 27500000, FEC_3_4, 0, diseqc_pos);	// 79
			get_nits(12012000, 27500000, FEC_3_4, 1, diseqc_pos);	// 80
			get_nits(12031500, 27500000, FEC_3_4, 0, diseqc_pos);	// 81
			get_nits(12051000, 27500000, FEC_3_4, 1, diseqc_pos);	// 82 *
			get_nits(12070500, 27500000, FEC_3_4, 0, diseqc_pos);	// 83
			get_nits(12090000, 27500000, FEC_3_4, 1, diseqc_pos);	// 84
			get_nits(12109500, 27500000, FEC_3_4, 0, diseqc_pos);	// 85
			get_nits(12129000, 27500000, FEC_3_4, 1, diseqc_pos);	// 86
			get_nits(12148000, 27500000, FEC_3_4, 0, diseqc_pos);	// 87
			get_nits(12168000, 27500000, FEC_3_4, 1, diseqc_pos);	// 88 *
#endif
			get_nits(12187500, 27500000, FEC_3_4, 0, diseqc_pos);	// 89 *
#if 0
			get_nits(12207000, 27500000, FEC_3_4, 1, diseqc_pos);	// 90
			get_nits(12226000, 27500000, FEC_3_4, 0, diseqc_pos);	// 91
			get_nits(12246000, 27500000, FEC_3_4, 1, diseqc_pos);	// 92
			get_nits(12265500, 27500000, FEC_3_4, 0, diseqc_pos);	// 93
			get_nits(12285000, 27500000, FEC_3_4, 1, diseqc_pos);	// 94
			get_nits(12304500, 27500000, FEC_3_4, 0, diseqc_pos);	// 95
			get_nits(12324000, 27500000, FEC_3_4, 1, diseqc_pos);	// 96
			get_nits(12343500, 27500000, FEC_3_4, 0, diseqc_pos);	// 97
			get_nits(12363000, 27500000, FEC_3_4, 1, diseqc_pos);	// 98
			get_nits(12382500, 27500000, FEC_3_4, 0, diseqc_pos);	// 99
			get_nits(12402000, 27500000, FEC_3_4, 1, diseqc_pos);	// 100
#endif
			get_nits(12422000, 27500000, FEC_3_4, 0, diseqc_pos);	// 101 empty?
#if 0
			get_nits(12441000, 27500000, FEC_3_4, 1, diseqc_pos);	// 102
			get_nits(12460000, 27500000, FEC_3_4, 0, diseqc_pos);	// 103
			get_nits(12480000, 27500000, FEC_3_4, 1, diseqc_pos);	// 104
			get_nits(12515300, 22000000, FEC_5_6, 0, diseqc_pos);	// 105
			get_nits(12522000, 22000000, FEC_5_6, 1, diseqc_pos);	// 106
			get_nits(12545000, 22000000, FEC_5_6, 0, diseqc_pos);	// 107
#endif
			get_nits(12551000, 22000000, FEC_5_6, 1, diseqc_pos);	// 108 *
#if 0
			get_nits(12574200, 22000000, FEC_5_6, 0, diseqc_pos);	// 109
			get_nits(12581000, 22000000, FEC_5_6, 1, diseqc_pos);	// 110
			get_nits(12604000, 22000000, FEC_5_6, 0, diseqc_pos);	// 111
			get_nits(12610500, 22000000, FEC_5_6, 1, diseqc_pos);	// 112
			get_nits(12633000, 22000000, FEC_5_6, 0, diseqc_pos);	// 113
			get_nits(12640000, 22000000, FEC_5_6, 1, diseqc_pos);	// 114
			get_nits(12663000, 22000000, FEC_5_6, 0, diseqc_pos);	// 115 empty?
			get_nits(12669500, 22000000, FEC_5_6, 1, diseqc_pos);	// 116
			get_nits(12692000, 22000000, FEC_5_6, 0, diseqc_pos);	// 117 *
			get_nits(12699000, 22000000, FEC_5_6, 1, diseqc_pos);	// 118
			get_nits(12722000, 22000000, FEC_5_6, 0, diseqc_pos);	// 119
			get_nits(12728000, 22000000, FEC_5_6, 1, diseqc_pos);	// 120 empty?
#endif
			get_sdts();
			fd = write_sat(fd, satName, diseqc_pos);
		}

		if (do_diseqc & 2)
		{
			diseqc_pos = 1;
			curr_sat = 2;
			strcpy(satName, "Hotbird 13.0E");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
			get_nits(10723000, 29900000, FEC_3_4, 0, diseqc_pos); // 111
			get_nits(10775000, 28000000, FEC_3_4, 0, diseqc_pos); // 113
			get_nits(11060000,  6510000, FEC_5_6, 1, diseqc_pos); // 128L
			get_nits(11131000,  5632000, FEC_3_4, 1, diseqc_pos); // 130U
			get_nits(11178000, 22000000, FEC_3_4, 0, diseqc_pos); // 131U
			get_nits(11196000,  9100000, FEC_1_2, 1, diseqc_pos); // 132U
			get_nits(11205000,  4000000, FEC_3_4, 0, diseqc_pos); // 1L
			get_nits(11283000, 27500000, FEC_3_4, 1, diseqc_pos); // 4
			get_nits(11304000, 30000000, FEC_3_4, 0, diseqc_pos); // 5
			get_nits(11338000,  5632000, FEC_3_4, 1, diseqc_pos); // 6U
			get_nits(11413000,  6200000, FEC_7_8, 0, diseqc_pos); // 11L
			get_nits(11464000,  4400000, FEC_7_8, 0, diseqc_pos); // 12U
			get_nits(11747000, 27500000, FEC_3_4, 0, diseqc_pos); // 51
			get_nits(11804000, 27500000, FEC_2_3, 1, diseqc_pos); // 54
			get_nits(11919000, 27500000, FEC_2_3, 1, diseqc_pos); // 60
			get_nits(12034000, 27500000, FEC_3_4, 1, diseqc_pos); // 66
			get_nits(12111000, 27500000, FEC_3_4, 1, diseqc_pos); // 70
			get_nits(12169000, 27500000, FEC_3_4, 0, diseqc_pos); // 72
			get_nits(12539000, 27500000, FEC_3_4, 0, diseqc_pos); // 91
			get_nits(12692000, 27500000, FEC_3_4, 0, diseqc_pos); // 99
			get_sdts();
			fd = write_sat(fd, satName, diseqc_pos);
		}

		if (do_diseqc & 4)
		{
			diseqc_pos = 2;
			curr_sat = 4;
			strcpy(satName, "Kopernikus 23.5E");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
			get_nits(11466000, 27500000, FEC_3_4, 0, diseqc_pos); // A
			get_nits(11646000, 27500000, FEC_3_4, 0, diseqc_pos); // C
			get_nits(11681000, 27500000, FEC_3_4, 0, diseqc_pos); // C
			get_nits(12541000,  2168000, FEC_7_8, 1, diseqc_pos); // 1
			get_nits(12658000, 27500000, FEC_3_4, 1, diseqc_pos); // 5
			get_sdts();
			fd = write_sat(fd, satName, diseqc_pos);
		}

		if (do_diseqc & 8)
		{
			diseqc_pos = 3;
			curr_sat = 8;
			strcpy(satName, "Tuerksat 42.0E");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
			get_nits(10986000,  2342000, FEC_3_4, 0, diseqc_pos); // 12
			get_nits(11110000,  4557000, FEC_5_6, 1, diseqc_pos); // 7
			get_nits(11135000,  4444000, FEC_5_6, 1, diseqc_pos); // 8
			get_nits(11154000,  4557000, FEC_3_4, 1, diseqc_pos); // 8
			get_nits(11457000,  5632000, FEC_3_4, 1, diseqc_pos); // 1
			get_sdts();
			fd = write_sat(fd, satName, diseqc_pos);
		}

		if (do_diseqc & 16)
		{
			diseqc_pos = 2;
			curr_sat = 16;
			strcpy(satName, "Sirius 5.0E");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
			get_nits(11376000,  2170000, FEC_3_4, 1, diseqc_pos); // 12
			get_nits(11727000, 27500000, FEC_3_4, 0, diseqc_pos); // 1
                        get_nits(11766000, 27500000, FEC_3_4, 0, diseqc_pos); // 3
                        get_nits(11785000, 27500000, FEC_3_4, 1, diseqc_pos); // 4 empty
                        get_nits(11804000, 27500000, FEC_3_4, 0, diseqc_pos); // 5
                        get_nits(11823000, 27500000, FEC_3_4, 1, diseqc_pos); // 6
                        get_nits(11843000, 27500000, FEC_3_4, 0, diseqc_pos); // 7
                        get_nits(11881000, 27500000, FEC_3_4, 0, diseqc_pos); // 9
                        get_nits(11900000, 27500000, FEC_3_4, 1, diseqc_pos); // 10
                        get_nits(11977000, 27500000, FEC_3_4, 1, diseqc_pos); // 14
                        get_nits(11996000, 27500000, FEC_3_4, 0, diseqc_pos); // 15
                        get_nits(12034000, 27500000, FEC_3_4, 0, diseqc_pos); // 17
                        get_nits(12054000, 27500000, FEC_3_4, 1, diseqc_pos); // 18
                        get_nits(12073000, 25378000, FEC_7_8, 0, diseqc_pos); // 19
                        get_nits(12111000, 27500000, FEC_3_4, 0, diseqc_pos); // 21
                        get_nits(12130000, 18080000, FEC_3_4, 1, diseqc_pos); // 22
                        get_nits(12149000, 27500000, FEC_3_4, 0, diseqc_pos); // 23
                        get_nits(12188000, 27500000, FEC_3_4, 0, diseqc_pos); // 25
                        get_nits(12226000, 25540000, FEC_7_8, 0, diseqc_pos); // 27
                        get_nits(12245000, 27500000, FEC_7_8, 1, diseqc_pos); // 28
                        get_nits(12284000, 27500000, FEC_3_4, 1, diseqc_pos); // 30
                        get_nits(12303000, 25548000, FEC_7_8, 0, diseqc_pos); // 31
                        get_nits(12341000, 20000000, FEC_3_4, 1, diseqc_pos); // 33
                        get_nits(12380000, 27500000, FEC_3_4, 1, diseqc_pos); // 35
                        get_nits(12418000, 25540000, FEC_7_8, 1, diseqc_pos); // 37
                        get_nits(12453000, 25540000, FEC_7_8, 0, diseqc_pos); // 39
                        get_nits(12469000, 25540000, FEC_3_4, 0, diseqc_pos); // 39 empty
                        get_nits(12590000,  6110000, FEC_3_4, 1, diseqc_pos); // 3
                        get_nits(12600000,  6110000, FEC_3_4, 1, diseqc_pos); // 3
                        get_nits(12608000,  6110000, FEC_3_4, 1, diseqc_pos); // 3
                        get_nits(12616000,  6110000, FEC_3_4, 1, diseqc_pos); // 3
                        get_nits(12629000,  3222000, FEC_7_8, 1, diseqc_pos); // 4
                        get_nits(12630000,  6110000, FEC_3_4, 0, diseqc_pos); // 10 empty
                        get_nits(12633000,  3720000, FEC_3_4, 1, diseqc_pos); // 4
                        get_nits(12640000,  4000000, FEC_3_4, 1, diseqc_pos); // 4
                        get_nits(12644000,  3200000, FEC_3_4, 1, diseqc_pos); // 4
                        get_nits(12649000,  4000000, FEC_3_4, 1, diseqc_pos); // 4 empty
                        get_nits(12661000,  6110000, FEC_3_4, 0, diseqc_pos); // 10
                        get_nits(12674000,  6666000, FEC_1_2, 1, diseqc_pos); // 5
                        get_nits(12674000,  6110000, FEC_3_4, 0, diseqc_pos); // 11
                        get_nits(12683000,  6666000, FEC_1_2, 1, diseqc_pos); // 5
                        get_nits(12686000,  6110000, FEC_3_4, 0, diseqc_pos); // 11 empty
                        get_nits(12690000,  6110000, FEC_3_4, 1, diseqc_pos); // 5 empty
                        get_nits(12696000,  6110000, FEC_7_8, 1, diseqc_pos); // 5 empty
                        get_nits(12700000,  3400000, FEC_3_4, 1, diseqc_pos); // 5
                        get_nits(12704000,  2963000, FEC_3_4, 1, diseqc_pos); // 5
                        get_nits(12718000,  4000000, FEC_7_8, 0, diseqc_pos); // 12
                        get_nits(12700000,  3400000, FEC_3_4, 1, diseqc_pos); // 5
                        get_nits(12704000,  2963000, FEC_3_4, 1, diseqc_pos); // 5
                        get_nits(12718000,  4000000, FEC_7_8, 0, diseqc_pos); // 12
			get_sdts();
			fd = write_sat(fd, satName, diseqc_pos);
		}

		if (do_diseqc & 32)
		{
			diseqc_pos = 3;
			curr_sat = 32;
			strcpy(satName, "Thor 0.8W");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
			get_nits(10966000,  2963000, FEC_3_4, 1, diseqc_pos); // 111XL
			get_nits(10974000,  6666000, FEC_3_4, 1, diseqc_pos); // 71 empty
			get_nits(10984000,  6666000, FEC_7_8, 1, diseqc_pos); // 71
			get_nits(10984000,  5632000, FEC_3_4, 1, diseqc_pos); // 111XL
			get_nits(10995000, 10000000, FEC_3_4, 1, diseqc_pos); // 111XL
			get_nits(11003000, 13330000, FEC_7_8, 1, diseqc_pos); // 71
			get_nits(11014000, 26000000, FEC_3_4, 0, diseqc_pos); // 61U
			get_nits(11015000,  6110000, FEC_3_4, 1, diseqc_pos); // 71
			get_nits(11029000,  2816000, FEC_3_4, 1, diseqc_pos); // 111XU
			get_nits(11044000,  5632000, FEC_3_4, 1, diseqc_pos); // 72L
			get_nits(11054000,  6110000, FEC_3_4, 1, diseqc_pos); // 72L
			get_nits(11064000,  6110000, FEC_3_4, 1, diseqc_pos); // 72L
			get_nits(11174000, 22500000, FEC_2_3, 0, diseqc_pos); // 63B
			get_nits(11216000, 24500000, FEC_7_8, 1, diseqc_pos); // 1
			get_nits(11229000, 24500000, FEC_7_8, 0, diseqc_pos); // 2
			get_nits(11247000, 24500000, FEC_7_8, 1, diseqc_pos); // 3
			get_nits(11278000, 24500000, FEC_7_8, 0, diseqc_pos); // 5
			get_nits(11293000, 24500000, FEC_7_8, 1, diseqc_pos); // 6
			get_nits(11309000, 24500000, FEC_7_8, 1, diseqc_pos); // 7
			get_nits(11372000, 24500000, FEC_7_8, 1, diseqc_pos); // 11
			get_nits(11403000, 24500000, FEC_7_8, 1, diseqc_pos); // 13
			get_nits(11459000,  3149000, FEC_3_4, 0, diseqc_pos); // 65L
			get_nits(11468000,  5632000, FEC_3_4, 0, diseqc_pos); // 65
			get_nits(11477000,  5632000, FEC_3_4, 0, diseqc_pos); // 65
			get_nits(11484000,  6137000, FEC_7_8, 0, diseqc_pos); // 65
        		get_nits(11494000,  5632000, FEC_3_4, 1, diseqc_pos); // 75
			get_nits(11495000,  5632000, FEC_3_4, 0, diseqc_pos); // 65
			get_nits(11504000,  6110000, FEC_3_4, 1, diseqc_pos); // 75
			get_nits(11527000,  4203000, FEC_2_3, 0, diseqc_pos); // 65
			get_nits(11540000, 26000000, FEC_3_4, 1, diseqc_pos); // 75U
			get_nits(11553000, 26000000, FEC_3_4, 0, diseqc_pos); // 65U
			get_nits(11585000,  6110000, FEC_3_4, 1, diseqc_pos); // 79
			get_nits(11587000,  5632000, FEC_3_4, 0, diseqc_pos); // 69
			get_nits(11596000,  6110000, FEC_3_4, 0, diseqc_pos); // 69
			get_nits(11597000,  6110000, FEC_3_4, 1, diseqc_pos); // 79
			get_nits(11605000,  6110000, FEC_3_4, 1, diseqc_pos); // 79
			get_nits(11619000,  2940000, FEC_3_4, 1, diseqc_pos); // 79
			get_nits(11623000,  2295000, FEC_7_8, 1, diseqc_pos); // 79
			get_nits(11630000,  8054000, FEC_7_8, 1, diseqc_pos); // 79
			get_nits(11639000,  4202000, FEC_2_3, 1, diseqc_pos); // 79
			get_nits(11652000,  4200000, FEC_2_3, 1, diseqc_pos); // 79
			get_nits(11665000,  6110000, FEC_3_4, 1, diseqc_pos); // 79
			get_nits(11677000, 26000000, FEC_3_4, 0, diseqc_pos); // 69
			get_nits(11686000, 11017000, FEC_3_4, 1, diseqc_pos); // 79
			get_nits(12054000, 28000000, FEC_7_8, 0, diseqc_pos); // 18
			get_nits(12169000, 28000000, FEC_7_8, 0, diseqc_pos); // 24
			get_nits(12226000, 28000000, FEC_7_8, 1, diseqc_pos); // 27
			get_nits(12245000, 28000000, FEC_7_8, 0, diseqc_pos); // 28
			get_nits(12303000, 27800000, FEC_3_4, 1, diseqc_pos); // 31
			get_nits(12322000, 27800000, FEC_3_4, 0, diseqc_pos); // 32
			get_nits(12399000, 28000000, FEC_7_8, 0, diseqc_pos); // 36
			get_nits(12456000, 28000000, FEC_3_4, 1, diseqc_pos); // 39
			get_nits(12476000, 27800000, FEC_3_4, 0, diseqc_pos); // 40
			get_sdts();
			fd = write_sat(fd, satName, diseqc_pos);
		}
	}

	write_xml_footer(fd);
	write_bouquets(do_diseqc);

	printf("[scan.cpp] found %d transponders and %d channels\n", found_transponders, found_channels);

	if (prepare_channels() < 0)
	{
		printf("[scan.cpp] Error parsing Services\n");
		pthread_exit(0);
	}

	printf("[scan.cpp] Channels have been loaded succesfully\n");

	scan_runs = 0;
	eventServer->sendEvent(CZapitClient::EVT_SCAN_COMPLETE, CEventServer::INITID_ZAPIT);
	pthread_exit(0);
}

