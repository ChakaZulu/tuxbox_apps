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
			}
		}
	}

	fprintf(fd, "<transponder transportID=\"%d\" networkID=\"0\">\n", transport_stream_id);
	fprintf(fd, "%s", transponder.c_str());
	fprintf(fd, "</transponder>\n");
	return;
}

void write_sat(FILE *fd, const char *satname, const uint8_t diseqc_pos)
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
		feparams.u.qam.SymbolRate = 6900000;
		feparams.u.qam.FEC_inner = FEC_AUTO;
		feparams.u.qam.QAM = QAM_64;

		for (feparams.Frequency = 306000; feparams.Frequency <= 460000; feparams.Frequency += 8000)
		{
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
					feparams.u.qam.SymbolRate = 6900000;
				}
			}
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
			//get_nits(10788000, 22000000, FEC_5_6, 1, diseqc_pos);	// 54
			get_nits(10832000, 22000000, FEC_5_6, 0, diseqc_pos);	// 57 **
			//get_nits(10862000, 22000000, FEC_5_6, 0, diseqc_pos);	// 59
			get_nits(10876000, 22000000, FEC_5_6, 1, diseqc_pos);	// 60 **
#if 0
			get_nits(11719500, 27500000, FEC_3_4, 0, diseqc_pos);	// 65
			get_nits(11739500, 27500000, FEC_3_4, 1, diseqc_pos);	// 66
			get_nits(11758500, 27500000, FEC_3_4, 0, diseqc_pos);	// 67
			get_nits(11778000, 27500000, FEC_3_4, 1, diseqc_pos);	// 68
			get_nits(11798000, 27500000, FEC_3_4, 0, diseqc_pos);	// 69 *
			get_nits(11817000, 27500000, FEC_3_4, 1, diseqc_pos);	// 70
			get_nits(11836500, 27500000, FEC_3_4, 0, diseqc_pos);	// 71
			get_nits(11856000, 27500000, FEC_3_4, 1, diseqc_pos);	// 72
			get_nits(11876000, 27500000, FEC_3_4, 0, diseqc_pos);	// 73
			get_nits(11895000, 27500000, FEC_3_4, 1, diseqc_pos);	// 74
			get_nits(11914000, 27500000, FEC_3_4, 0, diseqc_pos);	// 75 *
			get_nits(11934000, 27500000, FEC_3_4, 1, diseqc_pos);	// 76
			get_nits(11953500, 27500000, FEC_3_4, 0, diseqc_pos);	// 77 *
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
			write_sat(fd, satName, diseqc_pos);
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
			write_sat(fd, satName, diseqc_pos);
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
			write_sat(fd, satName, diseqc_pos);
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
			write_sat(fd, satName, diseqc_pos);
		}

		if (do_diseqc & 16)
		{
			diseqc_pos = 2;
			curr_sat = 16;
			strcpy(satName, "Sirius 5.0E");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
#if 0
			get_nits(12050, 27500, 1, 3, 2);
			get_nits(11975, 27500, 1, 3, 2);
			get_nits(12028, 27500, 0, 3, 2);
			get_nits(11994, 27500, 0, 3, 2);
			get_nits(11880, 27500, 0, 3, 2);
			get_nits(11804, 27500, 0, 3, 2);
			get_nits(11823, 27500, 1, 3, 2);
			get_nits(12380, 27500, 0, 3, 2);
			get_nits(11547, 27500, 0, 3, 2);
			get_nits(11727, 27500, 0, 3, 2);
			get_nits(11766, 27500, 0, 3, 2);
			get_nits(12073, 25376, 0, 3, 2);
			get_nits(12153,  7028, 0, 3, 2);
			get_nits(12188, 24500, 0, 7, 2);
			get_nits(12226, 25540, 0, 7, 2);
			get_nits(12245, 27500, 1, 7, 2);
			get_nits(12280, 27500, 1, 3, 2);
			get_nits(12303, 25548, 0, 7, 2);
			get_nits(12340, 20000, 0, 3, 2);
			get_nits(12415, 25540, 0, 7, 2);
			get_nits(12450, 18056, 0, 3, 2);
			get_nits(12469,  5185, 0, 3, 2);
			get_nits(12590,  6111, 1, 3, 2);
			get_nits(12600,  6111, 1, 3, 2);
			get_nits(12608,  6111, 1, 3, 2);
			get_nits(12616,  6111, 1, 3, 2);
			get_nits(12629,  3222, 1, 7, 2);
			get_nits(12633,  3643, 1, 7, 2);
			get_nits(12640,  4000, 1, 3, 2);
			get_nits(12644,  3200, 1, 3, 2);
			get_nits(12649,  3977, 1, 3, 2);
			get_nits(12661,  6110, 0, 3, 2);
			get_nits(12674,  6666, 1, 1, 2);
			get_nits(12674,  6110, 0, 3, 2);
			get_nits(12683,  6666, 1, 1, 2);
			get_nits(12686,  3400, 0, 3, 2);
			get_nits(12690,  3980, 1, 3, 2);
			get_nits(12718,  4000, 0, 7, 2);
#endif
			get_sdts();
			write_sat(fd, satName, diseqc_pos);
		}

		if (do_diseqc & 32)
		{
			diseqc_pos = 3;
			curr_sat = 32;
			strcpy(satName, "Thor 0.8W");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

			printf("[scan.cpp] scanning %s\n", satName);
#if 0
			get_nits(10966,  2963, 1, 3, 3);
			get_nits(10984,  6666, 1, 7, 3);
			get_nits(10984,  5924, 1, 3, 3);
			get_nits(10995, 10000, 1, 3, 3);
			get_nits(11003, 13300, 1, 7, 3);
			get_nits(11014, 26000, 0, 3, 3);
			get_nits(11029,  2816, 1, 3, 3);
			get_nits(11044,  5632, 1, 3, 3);
			get_nits(11054,  6110, 1, 3, 3);
			get_nits(11064,  6110, 1, 3, 3);
			get_nits(11174, 22500, 0, 2, 3);
			get_nits(11459,  3149, 0, 3, 3);
			get_nits(11468,  5632, 0, 3, 3);
			get_nits(11477,  5632, 0, 3, 3);
			get_nits(11484,  6140, 0, 7, 3);
			get_nits(11495,  5632, 0, 3, 3);
			get_nits(11504,  6110, 1, 3, 3);
			get_nits(11527,  4203, 0, 2, 3);
			get_nits(11540, 26000, 1, 3, 3);
			get_nits(11553, 26000, 0, 3, 3);
			get_nits(11587,  5632, 0, 3, 3);
			get_nits(11596,  6110, 0, 3, 3);
			get_nits(11597,  6110, 1, 3, 3);
			get_nits(11605,  6110, 1, 3, 3);
			get_nits(11619,  2940, 1, 3, 3);
			get_nits(11623,  2295, 1, 7, 3);
			get_nits(11630,  8054, 1, 7, 3);
			get_nits(11639,  4202, 1, 2, 3);
			get_nits(11652,  4202, 1, 2, 3);
			get_nits(11665,  6110, 1, 3, 3);
			get_nits(11677, 26000, 0, 3, 3);
			get_nits(11686, 11017, 1, 3, 3);
			get_nits(11229, 24500, 0, 7, 3);
			get_nits(11247, 24500, 1, 7, 3);
			get_nits(11278, 24500, 1, 7, 3);
			get_nits(11309, 24500, 1, 7, 3);
			get_nits(11372, 24500, 1, 7, 3);
			get_nits(11403, 24500, 1, 7, 3);
			get_nits(12054, 28000, 0, 7, 3);
			get_nits(12169, 28000, 0, 7, 3);
			get_nits(12226, 28000, 1, 7, 3);
			get_nits(12303, 27800, 1, 3, 3);
			get_nits(12322, 27800, 0, 3, 3);
			get_nits(12399, 28000, 0, 7, 3);
			get_nits(12456, 28000, 1, 3, 3);
			get_nits(12476, 27800, 0, 3, 3);
#endif
			get_sdts();
			write_sat(fd, satName, diseqc_pos);
		}
	}

	write_xml_footer(fd);
	write_bouquets(do_diseqc);

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

