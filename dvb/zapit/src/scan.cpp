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

#define FRONT_DEV "/dev/ost/qpskfe0"
#define DEMUX_DEV "/dev/ost/demux0"
#define SEC_DEV   "/dev/ost/sec0"




typedef std::map<int, scanchannel>::iterator sciterator;
typedef std::map<int, transpondermap>::iterator stiterator;
typedef std::multimap<std::string, bouquet_mulmap>::iterator sbiterator;
#ifdef NVOD_HACK
std::string curr_chan;
#endif
std::string services_xml = "/var/zapit/services.xml";
int fake_pat(std::map<int,transpondermap> *tmap, int freq, int sr);
int finaltune(int freq, int symbolrate, int polarity, int fec,int diseq);
int nit(int diseqc);
int sdt(uint osid, bool scan_mode);
int prepare_channels();
short scan_runs;     	

int issatbox()
{
	return atoi(getenv("fe"));
}

void get_nits(int freq, int symbolrate, int polarity, int fec,int diseq)
{
  finaltune(freq,symbolrate,polarity,fec,diseq);
  nit(diseq);
  //else
    //printf("No signal found on transponder\n");
}

void get_sdts()
{
  for (stiterator tI = scantransponders.begin(); tI != scantransponders.end(); tI++)
    {
      finaltune(tI->second.freq,tI->second.symbolrate,tI->second.polarization,tI->second.fec_inner,0);
        //if (pat(tI->second.freq,tI->second.symbolrate) >0)
	//{
	  printf("GETTING SDT FOR TSID: %04x\n",tI->second.tsid);
	  while (sdt(tI->second.tsid,true) == -2);
	//}
    }
}



void write_fake_bouquets(FILE *fd)
{
  fprintf(fd, "<Bouquet name=\"dummy\">\n\t<channel id=\"0000\"/>\n</Bouquet>\n");
}


void write_bouquets()
{
	FILE *bouq_fd;
	std::string oldname = "";
	
	bouq_fd = fopen("/var/zapit/bouquets.xml", "r");
	
	if (bouq_fd != NULL || scanbouquets.empty())
		return;
	else
	{
		bouq_fd = fopen("/var/zapit/bouquets.xml", "w");
		
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
	  char tsid[5];
	  char pmt[5];
	  char onid[5];
	  char service_type[5];
	  
	  sprintf(sid, "%04x", cI->second.sid);
	  sprintf(tsid, "%04x", cI->second.tsid);
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
	      transponder += "\" tsid=\"";
	      transponder += tsid;
	      transponder += "\" serviceType=\"";
	      transponder += service_type;
	      transponder += "\" channelNR=\"0\" ecmpid=\"0\">\n";
	      transponder += "\t\t\t<standard vpid=\"1fff\" apid=\"8191\" />\n";
	      transponder += "\t\t</channel>\n";
	      
	      
	      //printf("%30s tsid: %04x sid: %04x pmt: %04x onid: %04x\n", cI->second.name.c_str(),cI->second.tsid, cI->second.sid, cI->second.pmt, cI->second.onid);
	    }
	}
    }
  fprintf(fd,"%s\n",transponder.c_str()); 
}

void *start_scanthread(void *)
{
  FILE *fd = NULL;
  std::string transponder;
  
  
  
  scan_runs = 1;
  if (!issatbox())
    {
      int symbolrate = 6900;
      for (int freq = 3300; freq<=4500; freq +=80)
	{
	  //get_nits(freq,symbolrate,0,0,0);
	  //printf("get_nits() done\n");
	  finaltune(freq,symbolrate,0,0,0);
    	  fake_pat(&scantransponders, freq, symbolrate);
  	  //else
    		//printf("No signal found on transponder\n"); 
	}
	get_sdts();
      if (!scantransponders.empty())
      {
      	fd = fopen(services_xml.c_str(), "w" );
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
      
      
      printf("---------------------------\nSCANNING ASTRA\n---------------------------\n");
      //printf("This is a sat-box\n");
      get_nits(11797, 27500, 0, 0, 0);
      //printf("\n\nNext base-transponder\n\n");
      get_nits(12551, 22000, 1, 5, 0);
      //printf("\n\nNext base-transponder\n\n");
      get_nits(12168, 27500, 1, 3, 0);
      //printf("\n\nNext base-transponder\n\n");
      get_nits(12692, 22000, 0, 5, 0);
      get_nits(11913,27500,0,3,0);
      get_nits(11954,27500,0,3,0);
      get_nits(12051,27500,1,3,0);

      get_sdts();

      if (!scantransponders.empty())
	{
	  if (fd == NULL)
	  {
	  	fd = fopen(services_xml.c_str(), "w" );
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
      
      printf("---------------------------\nSCANNING HOTBIRD\n---------------------------\n");
      get_nits(12692,27500,0,3,1);
      get_nits(12539,27500,0,3,1);
      get_nits(11746,27500,0,3,1);
      get_nits(12168,27500,0,3,1);
      get_nits(12034,27500,1,3,1);
      get_nits(11919,27500,1,2,1);
      get_nits(11804,27500,1,2,1);
      get_nits(12169,27500,0,3,1);
      get_nits(12539,27500,0,3,1);
      get_nits(12111,27500,1,3,1);
      get_sdts();
      
      if (!scantransponders.empty())
	{
	if (fd == NULL)
	  {
	  	fd = fopen(services_xml.c_str(), "w" );
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

      printf("---------------------------\nSCANNING KOPERNIKUS\n---------------------------\n");
      get_nits(12655,27500,1,3,0);
      get_nits(12521,27500,1,3,0);
      get_sdts();

       if (!scantransponders.empty())
	{
		if (fd == NULL)
	  {
	  	fd = fopen(services_xml.c_str(), "w" );
	  	fprintf(fd,"<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
      	  	fprintf(fd,"<ZAPIT>\n");
      	}
	fprintf(fd, "<satellite name=\"Kopernikus\" diseqc=\"0\">\n");
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
      write_fake_bouquets(fd);
  fprintf(fd,"</ZAPIT>\n"); 
  fclose(fd);

  write_bouquets();  
        	
  if (prepare_channels() <0) 
  {
    printf("Error parsing Services\n");
    exit(-1);
   }
  printf("Channels have been loaded succesfully\n");
  
  scan_runs = 0;
  pthread_exit(0);
}
