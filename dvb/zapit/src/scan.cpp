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

void get_nits(int frequency, int symbol_rate, int polarity, int FEC_inner, int DiSEqC)
{
	if (finaltune(frequency, symbol_rate, polarity, FEC_inner, DiSEqC) == 0)
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

		if (finaltune(tI->second.freq, tI->second.symbolrate, tI->second.polarization, tI->second.fec_inner, tI->second.diseqc) == 0)
		{
			//if (pat(tI->second.freq,tI->second.symbolrate) >0)
			//{
				printf("GETTING SDT FOR TSID: %04x\n", tI->second.tsid);
				while (sdt(tI->second.tsid, true) == -2 && sdt_tries != 5)
				sdt_tries++;
			//}
		}
		else
			printf("No signal found on transponder\n");
	}
}

void write_fake_bouquets(FILE *fd)
{
	fprintf(fd, "<Bouquet name=\"dummy\">\n\t<channel id=\"0000\"/>\n</Bouquet>\n");
}

void write_bouquets(unsigned short mode)
{
	FILE *bouq_fd;
	std::string oldname = "";

	/*
	mode&1024 == löschn bouqets und erstelle sich nicht neu.
	mode&512 == erstelle bouquets immer neu
	mode&256 = keine änderung an bouqets.
	*/

	if (mode&1024)
	{
		printf("[zapit] removin existing bouqets.xml\n");
		system("/bin/rm " CONFIGDIR "/zapit/bouquets.xml");
		scanbouquets.clear();
		return;
	}

	bouq_fd = fopen(CONFIGDIR "/zapit/bouquets.xml", "r");


	if (mode&256 || scanbouquets.empty())
	{
		printf("[zapit] leavin bouquets.xml untouched\n");
		scanbouquets.clear();
		if (bouq_fd != NULL)
			fclose(bouq_fd);
		return;
	}
	else
	{
		printf("[zapit] creating new bouquets.xml\n");
		if (bouq_fd != NULL)
			fclose(bouq_fd);
		bouq_fd = fopen(CONFIGDIR "/zapit/bouquets.xml", "w");

		if (bouq_fd == NULL)
			{
				perror("fopen " CONFIGDIR "/zapit/bouquets.xml");
				scanbouquets.clear();
				return;
			}


		fprintf(bouq_fd, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n<ZAPIT>\n");

		for (sbiterator bI = scanbouquets.begin(); bI != scanbouquets.end(); bI++)
      		{
      			if (bI->second.provname != oldname)
      			{
      				if (oldname != "")
      				{
      					fprintf(bouq_fd, "</Bouquet>\n");
      					//printf("</Bouquet>\n");
      				}

      				fprintf(bouq_fd, "<Bouquet name=\"%s\">\n", bI->second.provname.c_str());
      				//printf("<Bouquet name=\"%s\">\n", bI->second.provname.c_str());

      			}
      			fprintf(bouq_fd, "\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n", bI->second.sid, bI->second.servname.c_str(), bI->second.onid);
      			//printf("\t<channel serviceID=\"%04x\" name=\"%s\" onid=\"%04x\"/>\n", bI->second.sid, bI->second.servname.c_str(), bI->second.onid);

      			oldname = bI->second.provname;
      		}

      		fprintf(bouq_fd, "</Bouquet>\n</ZAPIT>\n");
      		//printf("</Bouquet>\n</ZAPIT>\n");
      	}
      	scanbouquets.clear();
      	fclose(bouq_fd);
}



void write_transponder(int tsid, FILE *fd)
{
  std::string transponder;
  stiterator tI = scantransponders.find(tsid);
  char freq[6];
  char sr[6];
  char fec[2];
  char pol[2];


  sprintf(freq, "%05d", tI->second.freq);
  sprintf(sr, "%05d", tI->second.symbolrate);
  sprintf(fec, "%01d", tI->second.fec_inner);
  sprintf(pol, "%01d", tI->second.polarization);

  if (issatbox())
    transponder = "<sat ";
  else
    transponder = "<cable ";


  transponder += "frequency=\"";
  transponder += freq;
  transponder += "\" symbolRate=\"";
  transponder += sr;
  transponder += "\" fec=\"";
  transponder += fec;
  transponder += "\" polarity=\"";
  transponder += pol;
  transponder += "\"/>\n";


  for (sciterator cI = scanchannels.begin(); cI != scanchannels.end(); cI++)
    {
      if (cI->second.tsid == tsid)
	{

	  char sid[5];
//	  char tsid[5];
	  char pmt[5];
	  char onid[5];
	  char service_type[5];

	  sprintf(sid, "%04x", cI->second.sid);
//	  sprintf(tsid, "%04x", cI->second.tsid);
	  sprintf(pmt, "%04x", cI->second.pmt);
	  sprintf(onid, "%04x", cI->second.onid);
	  sprintf(service_type, "%04x", cI->second.service_type);

	  if (cI->second.name.length() > 0)
	    {
	      transponder += "\t\t<channel ServiceID=\"";
	      transponder += sid;
	      transponder += "\" name=\"";
	      transponder += cI->second.name;
	      transponder += "\" pmt=\"";
	      transponder += pmt;
	      transponder += "\" onid=\"";
	      transponder += onid;
//	      transponder += "\" tsid=\"";
//	      transponder += tsid;
	      transponder += "\" serviceType=\"";
	      transponder += service_type;
//	      transponder += "\" channelNR=\"0\" ecmpid=\"0\">\n";
//	      transponder += "\" channelNR=\"0\">\n";
	      transponder += "\" channelNR=\"0\" />\n";
//	      transponder += "\t\t</channel>\n";


	      //printf("%30s tsid: %04x sid: %04x pmt: %04x onid: %04x\n", cI->second.name.c_str(),cI->second.tsid, cI->second.sid, cI->second.pmt, cI->second.onid);
	    }
	}
    }
  fprintf(fd,"%s\n",transponder.c_str());
}

void *start_scanthread(void *param)
{
	FILE *fd = NULL;
	std::string transponder;
	int is_satbox = issatbox();
	char satName[50];
	unsigned short do_diseqc = *(unsigned short *) (param);

	if (is_satbox == -1)
	{
		printf("Is your dbox properly set up?\n");
		scan_runs = 1; //start_scan is waiting till scan_runs = 1
		usleep(500);
		scan_runs = 0;
		pthread_exit(0);
	}
	
	scan_runs = 1;

	if (!is_satbox)
	{
		strcpy(satName, "cable");
		eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName) + 1);

		curr_sat = 0;
		int symbolrate = 6900;
		int symbolrate2 = 6875;

		for (int freq = 3300; freq <= 4600; freq += 80)
		{
			if (finaltune(freq, symbolrate, 0, 0, 0) == 0)
			{
				fake_pat(&scantransponders, freq, symbolrate);
			}
			else
			{
				printf("[scan.cpp] No signal found on transponder. Trying SR 6875\n");

				if (finaltune(freq, symbolrate2, 0, 0, 0) == 0)
					fake_pat(&scantransponders, freq, symbolrate2);
				else
					printf("[scan.cpp] No signal found on transponder\n");
			}
		}

		get_sdts();

		if (!scantransponders.empty())
		{
			fd = fopen(services_xml.c_str(), "w" );

			if (fd == NULL)
			{
				perror("[zapit.cpp] open services.xml");
				scan_runs = 0;
				pthread_exit(0);
			}

			fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
			fprintf(fd,"<ZAPIT>\n<cable>\n");
			for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
			{
				fprintf(fd, "<transponder transportID=\"%05d\" networkID=\"0\">\n", tI->second.tsid);
				write_transponder(tI->second.tsid, fd);
				fprintf(fd, "</transponder>\n");
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
			curr_sat = 1;
			strcpy(satName, "ASTRA");
			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName)+ 1 );

			printf("[scan.cpp] SCANNING ASTRA\n");
			get_nits(11797, 27500, 0, 0, 0);
			get_nits(12551, 22000, 1, 5, 0);
			get_nits(12168, 27500, 1, 3, 0);
			get_nits(12692, 22000, 0, 5, 0);
			get_nits(11913, 27500, 0, 3, 0);
			get_nits(11954, 27500, 0, 3, 0);
			get_nits(12051, 27500, 1, 3, 0);
			get_sdts();

			if (!scantransponders.empty())
			{
				if (fd == NULL)
				{
					fd = fopen(services_xml.c_str(), "w" );

					if (fd == NULL)
					{
						perror("[scan.cpp] open services.xml");
						scan_runs = 0;
						pthread_exit(0);
					}

					fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
					fprintf(fd,"<ZAPIT>\n");
				}

				fprintf(fd, "<satellite name=\"Astra 19.2E\" diseqc=\"0\">\n");
				for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
				{
					fprintf(fd, "<transponder transportID=\"%d\" networkID=\"0\">\n", tI->second.tsid);
					write_transponder(tI->second.tsid,fd);
					fprintf(fd, "</transponder>\n");
				}
				fprintf(fd, "</satellite>\n");
			}

			scanchannels.clear();
			scantransponders.clear();
		}

		if (do_diseqc & 2)
		{
			curr_sat = 2;
			strcpy(satName, "HOTBIRD");

			eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName)+ 1 );

			printf("[scan.cpp] SCANNING HOTBIRD\n");
			get_nits(12692, 27500, 0, 3, 1);
			get_nits(12539, 27500, 0, 3, 1);
			get_nits(11746, 27500, 0, 3, 1);
			get_nits(12168, 27500, 0, 3, 1);
			get_nits(12034, 27500, 1, 3, 1);
			get_nits(11919, 27500, 1, 2, 1);
			get_nits(11804, 27500, 1, 2, 1);
			get_nits(12169, 27500, 0, 3, 1);
			get_nits(12539, 27500, 0, 3, 1);
			get_nits(12111, 27500, 1, 3, 1);
			get_nits(12168, 27500, 0, 3, 1);
			get_nits(11283, 27500, 1, 3, 1);
			get_nits(12283, 27500, 1, 3, 1);
			get_nits(11331,  6111, 1, 3, 1);
			get_nits(11412,  6198, 0, 7, 1);
			get_nits(10723, 29895, 0, 3, 1);
			get_nits(10775, 28000, 0, 3, 1);
			get_nits(10975,  4340, 0, 3, 1);
			get_nits(11060,  6510, 1, 5, 1);
			get_nits(11131,  5632, 1, 3, 1);
			get_nits(11178, 21100, 0, 3, 1);
			get_nits(11196,  9100, 1, 1, 1);
			get_nits(11205,  4000, 0, 3, 1);
			get_nits(11304, 30000, 0, 3, 1);
			get_nits(11338,  5632, 1, 3, 1);
			get_nits(11457,  6111, 0, 3, 1);
			get_nits(11464,  4398, 0, 7, 1);
#if 0
			get_nits(12211,  5632, 0, 3, 1);
			get_nits(12220,  6161, 0, 3, 1);
			get_nits(12236, 13400, 1, 3, 1);
			get_nits(12198, 12130, 0, 7, 1);
			get_nits(12484,  8300, 1, 3, 1);
			get_nits(12573,  5632, 0, 3, 1);
			get_nits(12581,  5632, 0, 3, 1);
			get_nits(12590,  5632, 0, 3, 1);
#endif
			get_sdts();

			if (!scantransponders.empty())
			{
				if (fd == NULL)
				{
					fd = fopen(services_xml.c_str(), "w" );
					
					if (fd == NULL)
					{
						perror("[scan.cpp] open services.xml");
						scan_runs = 0;
						pthread_exit(0);
					}

					fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
					fprintf(fd,"<ZAPIT>\n");
				}

				fprintf(fd, "<satellite name=\"Hotbird 13.0E\" diseqc=\"1\">\n");
				for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
				{
					fprintf(fd, "<transponder transportID=\"%d\" networkID=\"0\">\n", tI->second.tsid);
					write_transponder(tI->second.tsid,fd);
					fprintf(fd,"</transponder>\n");
				}
				fprintf(fd, "</satellite>\n");
			}

			scanchannels.clear();
			scantransponders.clear();
		}

      if (do_diseqc & 4)
      {
      	curr_sat = 4;
      	strcpy(satName, "KOPERNIKUS");
    	eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName)+ 1 );

      printf("[scan.cpp] SCANNING KOPERNIKUS\n");
      get_nits(12655,27500,1,3,2);
      get_nits(12521,27500,1,3,2);
      get_sdts();

       if (!scantransponders.empty())
	{
		if (fd == NULL)
	  {
	  	fd = fopen(services_xml.c_str(), "w" );
	  	if (fd == NULL)
      		{
			perror("[scan.cpp] open services.xml");
      			scan_runs = 0;
  			pthread_exit(0);
  		}
	  	fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
      	  	fprintf(fd,"<ZAPIT>\n");
      	}
	fprintf(fd, "<satellite name=\"Kopernikus\" diseqc=\"2\">\n");
	  for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
	    {
	    fprintf(fd, "<transponder transportID=\"%d\" networkID=\"0\">\n", tI->second.tsid);
	      write_transponder(tI->second.tsid,fd);
	      fprintf(fd,"</transponder>\n");
	    }
	  fprintf(fd, "</satellite>\n");
	}
       scanchannels.clear();
       scantransponders.clear();
       }

      if (do_diseqc & 16)
      {
      	curr_sat = 16;
		strcpy(satName, "SIRIUS");
    	eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName)+ 1 );

      printf("[scan.cpp] SCANNING SIRIUS\n");
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
      get_sdts();

       if (!scantransponders.empty())
	{
		if (fd == NULL)
	  {
	  	fd = fopen(services_xml.c_str(), "w" );
	  	if (fd == NULL)
      		{
			perror("[scan.cpp] open services.xml");
      			scan_runs = 0;
  			pthread_exit(0);
  		}
	  	fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
      	  	fprintf(fd,"<ZAPIT>\n");
      	}
	fprintf(fd, "<satellite name=\"Sirius 5.0E\" diseqc=\"2\">\n");
	  for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
	    {
	    fprintf(fd, "<transponder transportID=\"%d\" networkID=\"0\">\n", tI->second.tsid);
	      write_transponder(tI->second.tsid,fd);
	      fprintf(fd,"</transponder>\n");
	    }
	  fprintf(fd, "</satellite>\n");
	}
       scanchannels.clear();
       scantransponders.clear();
       }

	if (do_diseqc & 18)
	{
		curr_sat = 18;
		strcpy(satName, "THOR");
		eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName)+ 1 );

		printf("[scan.cpp] SCANNING THOR\n");
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
		get_sdts();

		if (!scantransponders.empty())
		{
			if (fd == NULL)
			{
				fd = fopen(services_xml.c_str(), "w" );
					if (fd == NULL)
				{
					perror("[scan.cpp] open services.xml");
					scan_runs = 0;
					pthread_exit(0);
				}
					fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
				fprintf(fd,"<ZAPIT>\n");
			}
				fprintf(fd, "<satellite name=\"Thor 0.8W\" diseqc=\"3\">\n");
			for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
			{
				fprintf(fd, "<transponder transportID=\"%d\" networkID=\"0\">\n", tI->second.tsid);
				write_transponder(tI->second.tsid,fd);
				fprintf(fd, "</transponder>\n");
			}
			fprintf(fd, "</satellite>\n");
		}
			scanchannels.clear();
		scantransponders.clear();
	}

      if (do_diseqc & 8)
      {
      	curr_sat = 8;
      	strcpy(satName, "TÜRKSAT");
    	eventServer->sendEvent(CZapitClient::EVT_SCAN_SATELLITE, CEventServer::INITID_ZAPIT, &satName, strlen(satName)+ 1 );

      printf("[scan.cpp] SCANNING TÜRKSAT\n");
      get_nits(10985, 23420, 0, 3, 3);
      get_nits(11015, 41790, 0, 3, 3);
      get_nits(11028, 35720, 0, 5, 3);
      get_nits(11037, 23420, 0, 5, 3);
      get_nits(11054, 70000, 1, 3, 3);
      get_nits(11088, 56320, 1, 3, 3);
      get_nits(11100, 56320, 1, 3, 3);
      get_nits(11110, 56320, 1, 3, 3);
      get_nits(11117, 56320, 1, 3, 3);
      get_nits(11117, 30550, 1, 3, 3);
      get_nits(11133, 45550, 1, 5, 3);
      get_nits(11134, 26000, 0, 7, 3);
      get_nits(11137, 31500, 0, 5, 3);
      get_nits(11156, 21730, 1, 5, 3);
      get_nits(11160, 21730, 1, 5, 3);
      get_nits(11162, 21730, 1, 5, 3);
      get_nits(11166, 56320, 1, 5, 3);
      get_nits(11168, 21730, 1, 5, 3);
      get_nits(11172, 21730, 1, 5, 3);
      get_nits(11193, 54980, 1, 5, 3);
      get_nits(11453, 19830, 0, 7, 3);
      get_nits(11567, 20000, 0, 3, 3);
      get_sdts();

       if (!scantransponders.empty())
	{
		if (fd == NULL)
	  {
	  	fd = fopen(services_xml.c_str(), "w" );
	  	if (fd == NULL)
      		{
			perror("[scan.cpp] open services.xml");
      			scan_runs = 0;
  			pthread_exit(0);
  		}
	  	fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
      	  	fprintf(fd,"<ZAPIT>\n");
      	}
	fprintf(fd, "<satellite name=\"Türksat\" diseqc=\"3\">\n");
	  for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
	    {
	    fprintf(fd, "<transponder transportID=\"%d\" networkID=\"0\">\n", tI->second.tsid);
	      write_transponder(tI->second.tsid,fd);
	      fprintf(fd,"</transponder>\n");
	    }
	  fprintf(fd, "</satellite>\n");
	}
       scanchannels.clear();
       scantransponders.clear();
       }



      }
	

	write_fake_bouquets(fd);
	fprintf(fd,"</ZAPIT>\n");
	fclose(fd);

	write_bouquets(do_diseqc);

	if (prepare_channels() < 0)
	{
		printf("[scan.cpp] Error parsing Services\n");
		pthread_exit(0);
	}

	printf("[scan.cpp] Channels have been loaded succesfully\n");

	scan_runs = 0;
	eventServer->sendEvent(CZapitClient::EVT_SCAN_COMPLETE, CEventServer::INITID_ZAPIT );
	pthread_exit(0);
}

