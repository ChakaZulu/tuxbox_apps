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

void addlist(chanptr item)
{
	if (top != NULL) {
		//cur_c = top;
		if (strcasecmp(cur_c->name,item->name) < 0) 
		{
			//printf("Going forward\n");
			
			while (strcasecmp(cur_c->name,item->name) < 0 && cur_c->next != NULL)
			{
				//printf("Going forward\n");
				cur_c = cur_c->next;
			}
			
				if (cur_c == top)
				{
					//printf("inserting at top\n");
					if (top->next != NULL)
					{
						item->next = top->next;
						top->next->prev = item;
					}
					
					top->next = item;
					item->prev = top;
					//printf("Inserted %d %s at top\n", item->chan_nr, item->name);
				}
				else if (cur_c->next != NULL && cur_c->prev != NULL)
				{
					item->next = cur_c;
					item->prev = cur_c->prev;
					cur_c->prev->next = item;
					cur_c->prev = item;
				}
				else if (cur_c->next == NULL && strcasecmp(cur_c->name, item->name) >= 0)
				{
					item->prev = cur_c->prev;
					cur_c->prev->next = item;
					cur_c->prev = item;
					item->next = cur_c;
				}
				else
				{
					item->next = NULL;
					cur_c->next = item;
					item->prev = cur_c;
					
				//printf("inserting last item\n");
				}
				
					
		}
		else
		{
			//printf("Going backwards\n");
			
			while (strcasecmp(cur_c->name,item->name) >= 0 && cur_c != top)
			{
				//printf("Going backwards\n");
				cur_c = cur_c->prev;
			}
	
			if (cur_c == top && strcasecmp(cur_c->name,item->name) >= 0)
			{
			
				top->prev = item;
				item->next = top;
				top = item;
				top->prev = NULL;
				//printf("Inserted %d %s at top\n", item->chan_nr, item->name);
			}
			else if (cur_c == top && strcasecmp(cur_c->name,item->name) < 0)
			{
		  		item->next = top->next;
		  		if (top->next != NULL)
		  			top->next->prev = item;
		  		item->prev = top;
		  	}														
			else if (cur_c->next != NULL && cur_c->prev != NULL)
			{
				item->prev = cur_c;
				item->next = cur_c->next;
				cur_c->next->prev = item;
				cur_c->next = item;
			}
			else
			{
				item->prev = cur_c;
				item->next = NULL;
				cur_c->next = item;
				//printf("inserting last item\n");
				}
		}	
	}
	else{
		//printf("Inserting first item\n");
		top = item;
		top->prev = NULL;
		top->next = NULL;
		cur_c = top;
	}
}


void ParseTransponder(XMLTreeNode *transponder) {
  uint curr_freq = 0;
  uint curr_symbolrate = 0;	
  ushort curr_polarity = 0;
  ushort curr_fec = 0;
  uint curr_servid = 0;
  
  for (XMLTreeNode *services=transponder->GetChild(); services; services=services->GetNext())
    {
      chanptr tmp_chan = NULL;
      char *type = services->GetType();
      
      if (!strcmp("cable", type)){
	//printf("Frequency: %s\n", services->GetAttributeValue("frequency"));
	sscanf(services->GetAttributeValue("frequency"),"%u", &curr_freq);
	sscanf(services->GetAttributeValue("symbolRate"), "%u", &curr_symbolrate);
	sscanf(services->GetAttributeValue("Fec"), "%hu", &curr_fec);
	curr_symbolrate = curr_symbolrate * 1000;
      }
      else if (!strcmp("sat", type)){
	//printf("In sat-section\n");
	sscanf(services->GetAttributeValue("frequency"),"%u", &curr_freq);
	sscanf(services->GetAttributeValue("symbolRate"), "%u", &curr_symbolrate);
	curr_symbolrate = curr_symbolrate * 1000;
	sscanf(services->GetAttributeValue("Polarity"), "%hu", &curr_polarity);
	sscanf(services->GetAttributeValue("Fec"), "%hu", &curr_fec);
      }				
      else if (!strcmp("channel", type)){
	if (atoi(services->GetAttributeValue("serviceType")) == serv_mode) {
		char *name = services->GetAttributeValue("Name");
		if (*name != ' ') {
	  
	    //printf("Will build a channel of type %d.\n",serv_mode);
	    tmp_chan = (chanptr) malloc(sizeof(channel));
	    memset(tmp_chan, 0, sizeof(channel));
	    if (tmp_chan == NULL){
	    	printf("tmp_chan konnte nicht erstellt werden.\n");
	    	exit(0);
	}
	    services = services->GetChild();
	    sscanf(services->GetAttributeValue("vpid"), "%x", &tmp_chan->vpid);	
	    sscanf(services->GetAttributeValue("apid"), "%x", &tmp_chan->apid);		
	    services = services->GetParent();
	    
	    //sscanf(services->GetAttributeValue("channelNR"), "%x", &channel_nr);
	    sscanf(services->GetAttributeValue("serviceID"), "%x", &curr_servid); 
	    sscanf(services->GetAttributeValue("onid"), "%x", &tmp_chan->onid);

	    sscanf(services->GetAttributeValue("pmt"), "%x", &tmp_chan->pmt);
	    sscanf(services->GetAttributeValue("ecmpid"), "%x", &tmp_chan->ecmpid);
	    sscanf(services->GetAttributeValue("tsid"), "%x", &tmp_chan->tsid);
	    tmp_chan->prev = NULL;
	    tmp_chan->next = NULL;
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
	    //cur_c->next = tmp_chan;
	    //cur_c = tmp_chan;
	    addlist(tmp_chan);
	 
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

void correct_numbers()
{
	int number = 0;
	
	cur_c = top;
	do 
	{
		cur_c->chan_nr = ++number;
		cur_c = cur_c->next;
	} while (cur_c->next != NULL);
	cur_c = top;
}
	
chanptr LoadServices(int mode)
{
  top = NULL;
  cur_c = NULL;
  channel_nr = 0;
  curr_diseqc = 0;
  
  XMLTreeParser *parser=new XMLTreeParser("ISO-8859-1");
  FILE *in=fopen("/var/zapit/services.xml", "r");
  if (!in)
    {
      perror("/var/zapit/services.xml");
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
  //top = (chanptr) malloc(sizeof(channel));
  //current = (chanptr) malloc(sizeof(channel));
  cur_c = top;
  if (parser->RootNode()) 
    FindTransponder(parser->RootNode());
  
  
  delete parser;
  
  fclose(in);
  //cur_c = top->next;
  top->prev = 0;
  correct_numbers();
  return top;
}

