#include "xml/xmltree.h"


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#include "getservices.h"

extern "C" {
  chanptr channels;
  chanptr LoadServices(int mode);
};

void ParseTransponder(XMLTreeNode *transponder);
void ParseRoot(XMLTreeNode *root);
void FindTransponder(XMLTreeNode *root);
chanptr LoadServices(int mode);



chanptr top;
chanptr cur_c;
uint channel_nr;
int serv_mode;
uint curr_diseqc;


void ParseTransponder(XMLTreeNode *transponder) {
  uint curr_freq = 0;
  uint curr_symbolrate = 0;	
  ushort curr_polarity = 0;
  ushort curr_fec = 0;
  
  for (XMLTreeNode *services=transponder->GetChild(); services; services=services->GetNext())
    {
      chanptr tmp_chan = NULL;
      char *type = services->GetType();
      
      if (!strcmp("cable", type)){
	//printf("Frequency: %s\n", services->GetAttributeValue("frequency"));
	sscanf(services->GetAttributeValue("frequency"),"%u", &curr_freq);
	sscanf(services->GetAttributeValue("symbolRate"), "%u", &curr_symbolrate);
	sscanf(services->GetAttributeValue("Fec"), "%u", &curr_fec);
	curr_symbolrate = curr_symbolrate * 1000;
      }
      else if (!strcmp("sat", type)){
	//printf("In sat-section\n");
	sscanf(services->GetAttributeValue("frequency"),"%u", &curr_freq);
	sscanf(services->GetAttributeValue("symbolRate"), "%u", &curr_symbolrate);
	curr_symbolrate = curr_symbolrate * 1000;
	sscanf(services->GetAttributeValue("Polarity"), "%u", &curr_polarity);
	sscanf(services->GetAttributeValue("Fec"), "%u", &curr_fec);
      }				
      else if (!strcmp("channel", type)){
	if (atoi(services->GetAttributeValue("serviceType")) == serv_mode) {
		char *name = services->GetAttributeValue("Name");
		if (*name != ' ') {
	  if (atoi(services->GetAttributeValue("serviceID")) > 0) { 
	    //printf("Will build a channel of type %d.\n",serv_mode);
	    tmp_chan = (chanptr) malloc(sizeof(channel));
	    memset(tmp_chan, 0, sizeof(channel));
	    if (tmp_chan == NULL) printf("tmp_chan konnte nicht erstellt werden.\n");
	    
	    channel_nr++;
	    
	    //printf("Now getting standards\n");	
	    //				services = services->GetChild();
	    //printf("Bin in standard.\n");
	    //sscanf(services->GetAttributeValue("vpid"),"%x", &tmp_chan->vpid);
	    //sscanf(services->GetAttributeValue("apid"),"%x", &tmp_chan->apid);
	    
	    //services = services->GetParent();
	    //printf("Bin wieder in transponder\n");
	    
	    //printf("PMT wird geparst\n");
	    sscanf(services->GetAttributeValue("pmt"), "%x", &tmp_chan->pmt);
	    tmp_chan->prev = cur_c;
	    //sscanf(services->GetAttributeValue("id"),"%u", &channel_nr);
	    tmp_chan->chan_nr = channel_nr;
	    tmp_chan->frequency = curr_freq;
	    tmp_chan->symbolrate = curr_symbolrate;
	    tmp_chan->Diseqc = curr_diseqc;
	    tmp_chan->Polarity = curr_polarity;
	    tmp_chan->Fec = curr_fec;
	    tmp_chan->last_update = 0;
	    
	    tmp_chan->name = (char*) malloc(30);
	    strncpy(tmp_chan->name, services->GetAttributeValue("name"),30);
	    //printf("%d Kanalname: %s, Pmt: %04x\n",channel_nr, tmp_chan->name, tmp_chan->pmt);
	    cur_c->next = tmp_chan;
	    cur_c = tmp_chan;
	  }
	} else { 
		//printf("Channelname is empty\n");
	}
	} else {
	  //printf("Skipping channels which are not mode: %d .\n",serv_mode);
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
      //printf("Scanning a cable section\n");
      while (strcmp(search->GetType(), "transponder")){
	search = search->GetChild();
      }
      ParseRoot(search);
      search = search->GetParent();
    } else if (!(strcmp(search->GetType(), "satellite"))) {
      //printf("Scanning a satellite section\n");
      while (!(strcmp(search->GetType(), "satellite"))) {
	sscanf(search->GetAttributeValue("diseqc"),"%i",&curr_diseqc);
	printf("Going to parse Satellite %s\n", search->GetAttributeValue("name"));
	search = search->GetChild();
	ParseRoot(search);
	search = search->GetParent();
	search = search->GetNext();
      }
    } else {
    	if (channel_nr == 0)
    	      printf("No cable or satellite found\n");
    }
    search = search->GetNext();
  }
}

chanptr LoadServices(int mode)
{
  top = NULL;
  cur_c = NULL;
  channel_nr = 0;
  curr_diseqc = 0;
  
  XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");
  FILE *in=fopen("/etc/services.xml", "r");
  if (!in)
    {
      perror("services.xml");
      return NULL;
    }
  
  char buf[2048];
  //printf("Selected mode: %d\n",mode);
  //strcpy(serv_mode,mode);
  serv_mode = mode;
  //printf("Selected mode: %d\n",serv_mode);
  
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
	  return NULL;
	}
    } while (!done);
  top = (chanptr) malloc(sizeof(channel));
  //current = (chanptr) malloc(sizeof(channel));
  cur_c = top;
  if (parser->RootNode()) 
    FindTransponder(parser->RootNode());
  
  
  delete parser;
  
  fclose(in);
  cur_c = top->next;
  return cur_c;
}


/*
  int main(void) {
  printf("getservices is starting\n");
  chanptr servicelist = LoadServices(1);
  printf("LoadServices() ist vorbei\n");
  cur_c = top;
  if (top->next ==  NULL) printf("top = NULL\n");
  if (cur_c->next ==  NULL) printf("cur_c->next = NULL\n");
  while (cur_c->next) {
  //printf("Ausgabe sollte kommen: \n");
  printf("Current Nr: %u, ", servicelist->chan_nr);
  printf("Name: %s, ", servicelist->name);
  printf("VPID: %x, ", servicelist->vpid);
  printf("APID: %x, ", servicelist->apid);
  printf("Frequency: %u, ", servicelist->frequency);
  printf("Symbolrate: %u\n", servicelist->symbolrate);
  servicelist = servicelist->next;
  }
  
  }
  
*/
