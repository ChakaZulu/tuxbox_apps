#include "xml/xmltree.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <map>
#include <string>

#include "getservices.h"


void ParseTransponder(XMLTreeNode *xmltransponder);
void ParseRoot(XMLTreeNode *root);
void FindTransponder(XMLTreeNode *root);
int LoadServices(void);

uint curr_tsid = 0;
uint curr_diseqc;
int serv_mode;
typedef std::map<uint, transponder>::iterator titerator;

void ParseTransponder(XMLTreeNode *xmltransponder) {
  
  for (XMLTreeNode *services=xmltransponder->GetChild(); services; services=services->GetNext())
    {
      char *type = services->GetType();
      
      if (!strcmp("cable", type)){
	//printf("Frequency: %s\n", services->GetAttributeValue("frequency"));
	if (transponders.count(curr_tsid) == 0)
	  {
	    printf("No transponder with that tsid found\n");
	    exit(0);
	  }
	titerator trans = transponders.find(curr_tsid);
	sscanf(services->GetAttributeValue("frequency"),"%u", &trans->second.frequency);
	sscanf(services->GetAttributeValue("symbolRate"), "%u", &trans->second.symbolrate);
	sscanf(services->GetAttributeValue("Fec"), "%hu", &trans->second.fec);
      }
      else if (!strcmp("sat", type)){
	//printf("In sat-section\n");
	if (transponders.count(curr_tsid) == 0)
	  {
	    printf("No transponder with that tsid found\n");
	    exit(0);
	  }
	titerator trans = transponders.find(curr_tsid);
	sscanf(services->GetAttributeValue("frequency"),"%u", &trans->second.frequency);
	sscanf(services->GetAttributeValue("symbolRate"), "%u", &trans->second.symbolrate);
	sscanf(services->GetAttributeValue("Polarity"), "%hu", &trans->second.polarity);
	sscanf(services->GetAttributeValue("Fec"), "%hu", &trans->second.fec);
	trans->second.diseqc = curr_diseqc;
      }				
      else if (!strcmp("channel", type))
	{
	  int sm = atoi(services->GetAttributeValue("serviceType"));
	  if ( (sm == 1) || ( (sm==4) || (sm==2) ) ) {
	    std::string name = services->GetAttributeValue("Name");
	    if (name[0] != ' ') {
	      uint cnr, sid, pmt, ecmpid, tsid, onid;
	      std::string cname;
	      
	      //printf("Will build a channel of type: %d\n", sm);
	           
	      sscanf(services->GetAttributeValue("channelNR"), "%d", &cnr);
	      sscanf(services->GetAttributeValue("serviceID"), "%x", &sid);
	      sscanf(services->GetAttributeValue("onid"), "%x", &onid);
	      
	      sscanf(services->GetAttributeValue("pmt"), "%x", &pmt);
	      sscanf(services->GetAttributeValue("ecmpid"), "%x", &ecmpid);
	      sscanf(services->GetAttributeValue("tsid"), "%x", &tsid);
	      
	      //printf("%d Kanalname: %s, Pmt: %04x\n",tmp_chan->chan_nr, tmp_chan->name.c_str(), tmp_chan->pmt);
	     
	      if (sm == 2)
		{
		  allchans_radio.insert(std::pair<uint, channel>((onid<<16)+sid, channel(name, 0, 0, 0, pmt, ecmpid, sid, tsid, onid,sm)));
		  
		  if (cnr > 0)
		    {
		      numchans_radio.insert(std::pair<uint, uint>(cnr, (onid<<16)+sid));
		    }
		  else
		    {
		      namechans_radio.insert(std::pair<std::string, uint>(name, (onid<<16)+sid));
		    } 
		}
	      else
		{
		  allchans_tv.insert(std::pair<uint, channel>((onid<<16)+sid, channel(name, 0,0,0, pmt, ecmpid, sid, tsid,onid,sm)));
		  
		  if (cnr > 0)
		    {
		      numchans_tv.insert(std::pair<uint, uint>(cnr, (onid<<16)+sid));
		    }
		  else
		    {
		      namechans_tv.insert(std::pair<std::string, uint>(name, (onid<<16)+sid));
		    } 
		}
	    } else { 
	      //printf("Channelname is empty\n");
	    }
	  } else {
	    //printf("Skipping channel %s which is not mode: %d .\n", services->GetAttributeValue("name"),serv_mode);
	  }
	}
      else printf("Not known. Skipping %s\n", services->GetType());	
    }
}



void ParseRoot(XMLTreeNode *root)
{
  for (XMLTreeNode *c=root; c; c=c->GetNext())
    {
      
      //printf("Der Typ ist: %s\n", c->GetType());
      
      if (!strcasecmp(c->GetType(), "transponder"))
	{
	  //printf("Now going to parse the transponder\n");
	  sscanf(c->GetAttributeValue("transportID"), "%d", &curr_tsid);
	  //printf("TSID = %04x\n", curr_tsid);
	  transponders.insert(std::pair<uint, transponder>(curr_tsid, transponder(curr_tsid, 0,0,0,0,0,0)));
	  ParseTransponder(c);
	} else {
	  printf("ignoring %s\n", c->GetType());
	}
      
      
    }
  //printf("ParseRoot() ist durch\n");
}

void FindTransponder(XMLTreeNode *root)
{
  XMLTreeNode *search=root->GetChild();
  while ((strcmp(search->GetType(), "cable")) && (strcmp(search->GetType(), "satellite"))) {
    search = search->GetChild();
  }
  while (search) {
    if (!(strcmp(search->GetType(), "cable"))) {
      printf("Scanning a cable section\n");
      while (strcmp(search->GetType(), "transponder")){
	search = search->GetChild();
      }
      ParseRoot(search);
      search = search->GetParent();
    } else if (!(strcmp(search->GetType(), "satellite"))) {
      printf("Scanning a satellite section\n");
      while (!(strcmp(search->GetType(), "satellite"))) {
	sscanf(search->GetAttributeValue("diseqc"),"%i",&curr_diseqc);
	printf("Going to parse Satellite %s\n", search->GetAttributeValue("name"));
	search = search->GetChild();
	ParseRoot(search);
	search = search->GetParent();
	search = search->GetNext();
      }
    } else {
      //if (channel_nr == 0)
      //      printf("No cable or satellite found\n");
    }
    search = search->GetNext();
  }
}

int LoadServices(void)
{
  curr_diseqc = 0;
  
  XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");
  FILE *in=fopen("/var/zapit/services.xml", "r");
  if (!in)
    {
      perror("/var/zapit/services.xml");
      return -1;
    }
  
  char buf[2048];
  
  int done;
  do
    {
      unsigned int len=fread(buf, 1, sizeof(buf), in);
      done=len<sizeof(buf);
      if (!parser->Parse(buf, len, done))
	{
	  printf("parse error: %s at line %d\n", 
		 parser->ErrorString(parser->GetErrorCode()),
		 parser->GetCurrentLineNumber());
	  fclose(in);
	  delete parser;
	  return -1;
	}
    } while (!done);
  
  if (parser->RootNode()) 
    FindTransponder(parser->RootNode());
  
  
  delete parser;
  
  fclose(in);
  
  //printf("Returning channels\n");
  //printf("%d channels added\nPlease report to faralla@gmx.de if you see less in your neutrino-channellist\n",count);
  return 23;
}

/*
int main(void)
{
  LoadServices();

  for (std::map<uint, uint>::iterator I = numchans_tv.begin(); I != numchans_tv.end(); I++)
    {
      std::map<uint, channel>::iterator cI = allchans_tv.find(I->second);
      printf("name: %s, tsid: %04x, sid: %04x\n", cI->second.name.c_str(), cI->second.tsid, cI->second.sid);
    }
}
*/
