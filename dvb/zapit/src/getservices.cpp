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
chanptr top_num;
chanptr cur_c;
chanptr cur_c_num;
int serv_mode;
uint curr_diseqc;
uint count = 0;

void addlistnum(chanptr item)
{
	//printf("Inserting %s\n", item->name);
	if (top_num != NULL) {
		if (cur_c_num->chan_nr < item->chan_nr) 
		{
			//printf("Going forward\n");
			
			while (cur_c_num->chan_nr < item->chan_nr && cur_c_num->next != NULL)
			{
				//printf("Going forward\n");
				cur_c_num = cur_c_num->next;
			}
			
				if (cur_c_num == top_num)
				{
					//printf("inserting at top\n");
					if (top_num->next != NULL)
					{
						item->next = top_num->next;
						top_num->next->prev = item;
					}
					
					top_num->next = item;
					item->prev = top_num;
					//printf("Inserted %d %s at top\n", item->chan_nr, item->name);
				}
				else if (cur_c_num->next != NULL && cur_c_num->prev != NULL)
				{
					item->next = cur_c_num;
					item->prev = cur_c_num->prev;
					cur_c_num->prev->next = item;
					cur_c_num->prev = item;
				}
				else if (cur_c_num->next == NULL && cur_c_num->chan_nr >= item->chan_nr)
				{
					item->prev = cur_c_num->prev;
					cur_c_num->prev->next = item;
					cur_c_num->prev = item;
					item->next = cur_c_num;
				}
				else
				{
					item->next = NULL;
					cur_c_num->next = item;
					item->prev = cur_c_num;
					
				//printf("inserting last item\n");
				}
				
					
		}
		else
		{
			//printf("Going backwards\n");
			
			while (cur_c_num->chan_nr >= item->chan_nr && cur_c_num != top_num)
			{
				//printf("Going backwards\n");
				cur_c_num = cur_c_num->prev;
			}
	
			if (cur_c_num == top_num && cur_c_num->chan_nr >= item->chan_nr)
			{
			
				top_num->prev = item;
				item->next = top_num;
				top_num = item;
				top_num->prev = NULL;
				cur_c_num = top_num;
				//printf("Inserted %d %s at top\n", item->chan_nr, item->name);
			}
			else if (cur_c_num == top_num && cur_c_num->chan_nr < item->chan_nr)
			{
		  		item->next = top_num->next;
		  		if (top_num->next != NULL)
		  			top_num->next->prev = item;
		  		item->prev = top_num;
		  		top_num->next = item;
		  	}														
			else if (cur_c_num->next != NULL && cur_c_num->prev != NULL)
			{
				item->prev = cur_c_num;
				item->next = cur_c_num->next;
				cur_c_num->next->prev = item;
				cur_c_num->next = item;
			}
			else
			{
				item->prev = cur_c_num;
				item->next = NULL;
				cur_c_num->next = item;
				//printf("inserting last item\n");
				}
		}	
	}
	else{
		//printf("Inserting first item\n");
		top_num = item;
		top_num->prev = NULL;
		top_num->next = NULL;
		cur_c_num = top_num;
	}
}
void addlist(chanptr item)
{
	//printf("Inserting %s\n", item->name);
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
					cur_c = top;
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
				cur_c = top;
				//printf("Inserted %d %s at top\n", item->chan_nr, item->name);
			}
			else if (cur_c == top && strcasecmp(cur_c->name,item->name) < 0)
			{
		  		item->next = top->next;
		  		if (top->next != NULL)
		  			top->next->prev = item;
		  		item->prev = top;
		  		top->next = item;
		  		//printf("Inserted %d %s behind top\n", item->chan_nr, item->name);
		  	}														
			else if (cur_c->next != NULL && cur_c->prev != NULL)
			{
				item->prev = cur_c;
				item->next = cur_c->next;
				cur_c->next->prev = item;
				cur_c->next = item;
				//printf("Inserted %d %s in the middle\n", item->chan_nr, item->name);
			}
			else
			{
				item->prev = cur_c;
				item->next = NULL;
				cur_c->next = item;
				//printf("Inserted %d %s at tail\n", item->chan_nr, item->name);
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
      else if (!strcmp("channel", type))
      {
        int sm = atoi(services->GetAttributeValue("serviceType"));
  	   if ( (sm == serv_mode) || ( (sm==4) && (serv_mode==1) ) ) {
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
	    
	    sscanf(services->GetAttributeValue("channelNR"), "%d", &tmp_chan->chan_nr);
	    sscanf(services->GetAttributeValue("serviceID"), "%x", &tmp_chan->sid);
	    sscanf(services->GetAttributeValue("onid"), "%x", &tmp_chan->onid);

	    sscanf(services->GetAttributeValue("pmt"), "%x", &tmp_chan->pmt);
	    sscanf(services->GetAttributeValue("ecmpid"), "%x", &tmp_chan->ecmpid);
	    sscanf(services->GetAttributeValue("tsid"), "%x", &tmp_chan->tsid);
	    tmp_chan->prev = NULL;
	    tmp_chan->next = NULL;
	    tmp_chan->frequency = curr_freq;
	    tmp_chan->symbolrate = curr_symbolrate;
	    tmp_chan->Diseqc = curr_diseqc;
	    tmp_chan->Polarity = curr_polarity;
	    tmp_chan->Fec = curr_fec;
	    tmp_chan->last_update = 0;
	    
	    tmp_chan->name = (char*) malloc(30);
	    strncpy(tmp_chan->name, services->GetAttributeValue("name"),30);
	    //printf("%d Kanalname: %s, Pmt: %04x\n",tmp_chan->chan_nr, tmp_chan->name, tmp_chan->pmt);
	    //cur_c->next = tmp_chan;
	    //cur_c = tmp_chan;
	    ++count;
	    if (tmp_chan->chan_nr > 0)
	    	addlistnum(tmp_chan);
	    else
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
	cur_c->chan_nr = ++number;
	cur_c = top;
}
	
chanptr LoadServices(int mode)
{
  top = NULL;
  cur_c = NULL;
  top_num = NULL;
  cur_c_num = NULL;
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
  //printf("Concatenating lists\n");
  if (top_num != NULL)
  {
  	cur_c_num = top_num;
  	while (cur_c_num->next != NULL)
  		cur_c_num = cur_c_num->next;
  	
  	if (top != NULL)
  	{
  		top->prev = cur_c_num;
  		cur_c_num->next = top;
  	}
  	top = top_num;
  }

  
  
  if (top != NULL)
  {
  	//printf("Correcting numbers\n");
  	top->prev = NULL;
  	correct_numbers();
  }
  else
  {
  	top = (chanptr) malloc(sizeof(channel));
  	
  	top->next = NULL;
  	top->prev = NULL;
  	top->vpid = 0x1fff;
  	top->apid = 0x8191;
  	top->chan_nr = 1;
  	top->name = (char*) malloc(30);
  	top->name = "No channels. Don´t zap";
  }

  //printf("Returning channels\n");
  printf("%d channels added\nPlease report to faralla@gmx.de if you see less in your neutrino-channellist\n",count);
  return top;
}


