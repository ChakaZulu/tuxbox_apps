#ifndef __webdbox__
#define __webdbox__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include <vector>

#include <map>
#include "../../zapit/getservices.h"
#include "request.h"
#include "helper.h"
#include "controldclient.h"
#include "../controld/clientlib/controldclient.h"
//#include "../../sections/sectionsdMsg.h"
#include "../../sections/clientlib/sectionsdclient.h"
#include "../../zapit/clientlib/zapitclient.h"


using namespace std;

#define EZAP_PORT 1505

#define SA struct sockaddr
#define SAI struct sockaddr_in


struct st_rmsg {
  		unsigned char version;
  		unsigned char cmd;
  		unsigned char param;
  		unsigned short param2;
  		char param3[30];
};


class TWebserver;
class TWebserverRequest;
class CWebserverCGI;

class TListItem
{
	public:
	TListItem *Next;
};
class TList
{
public:
	TListItem *Head;
	int Count;
	TList(){Head = NULL; Count=0;}
	~TList()
	{
		if(Head && (Count > 0))
		{
			TListItem *next,*t = Head;
			do
			{
				next = t->Next;
				delete t;
				t = next;
			}while(next);
		}
	}
	void Add(TListItem *channel,TListItem *last)
	{
		if(channel)
		{
			if(last)
				last->Next = channel;
			else
				Head = channel;
			Count++;
		}
	}

	TListItem *GetValue(int index)
	{
	TListItem *channel;
	int i;
		for(channel = Head,i=0;(i < index)  && (i < Count);i++)
			channel = channel->Next;
		return(channel);
	}

};
//-------------------------------------------------------------------------
class TChannel
{
public:
	TChannel *Next;
	int Number;
	unsigned int onid_tsid;
	TString *EPG;
	TString *ExtendedEPG;
	time_t Starttime;
	long Duration;
	char Name[30];
	TChannel(int number,unsigned int tsid,char * name)
	{
		Number = number;
		onid_tsid = tsid;
		strncpy(Name,name,sizeof(Name));
		Next = NULL; 
		EPG= NULL;
		ExtendedEPG=NULL;
		Starttime = 0;
		Duration = 0;
	};
	~TChannel()
	{
		if(EPG) delete EPG;
		if(ExtendedEPG) delete ExtendedEPG;
	}
};
//-------------------------------------------------------------------------

class TChannelList
{
public:
	TChannel *Head;
	int Count;
	TChannelList(){Head = NULL; Count=0;}
	~TChannelList()
	{
		if(Head && (Count > 0))
		{
			TChannel *next,*t = Head;
			do
			{
				next = t->Next;
				delete t;
				t = next;
			}while(next);
		}
	}
	void Add(TChannel *channel,TChannel *last)
	{
		if(channel)
		{
			if(last)
				last->Next = channel;
			else
				Head = channel;
			Count++;
		}
	}

	TChannel *GetValue(int index)
	{
	TChannel *channel;
	int i;
		for(channel = Head,i=0;(i < index)  && (i < Count);i++)
			channel = channel->Next;
		return(channel);
	}

};
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
class TEvent
{
public:
	TEvent *Next;
	unsigned int ONID_TSID;
	TString *EEPG_ID;
	TString *EDate;
	TString *ETime;
	TString *EDuration;
	TString *EName;
	time_t Starttime;
	long Duration;
	char Name[30];
	TEvent(unsigned int onid_tsid,char *eepg_id,char *edate,char *etime,char *eduration,char *ename)
	{
		ONID_TSID = onid_tsid; 
		EEPG_ID = new TString(eepg_id);
		EDate = new TString(edate);
		ETime = new TString(etime);
		EDuration = new TString(eduration);
		EName = new TString(ename);
		Next = NULL; 
		Starttime = 0;
		Duration = 0;
		Name[0] = 0;
	};
	~TEvent()
	{
		if(EEPG_ID) delete EEPG_ID;
		if(EDate) delete EDate;
		if(ETime) delete ETime;
		if(EDuration) delete EDuration;
		if(EName) delete EName;
	}
};
//-------------------------------------------------------------------------

class TEventList
{
public:
	TEvent *Head;
	int Count;
	TEventList(){Head = NULL; Count=0;}
	~TEventList()
	{
		if(Head && (Count > 0))
		{
			TEvent *next,*t = Head;
			do
			{
				next = t->Next;
				delete t;
				t = next;
			}while(next);
		}
	}
	void Add(TEvent *event,TEvent *last)
	{
		if(event)
		{
			if(last)
				last->Next = event;
			else
				Head = event;
			Count++;
		}
	}

	TEvent *GetValue(int index)
	{
	TEvent *event;
	int i;
		for(event = Head,i=0;(i <= index)  && (i < Count);i++)
			event = event->Next;
		return(event);
	}

};
//-------------------------------------------------------------------------


class TWebDbox
{
	TWebserver * Parent;
	TChannelList *ChannelList;
	CControldClient controld;
	CSectionsdClient sectionsd;
	CZapitClient zapit;
	map<unsigned, char*> ChannelListEvents;

	time_t EPGDate;

public:
	TWebDbox(TWebserver * server);
	~TWebDbox();

	bool GetChannelList();
	bool PublishChannelList(TWebserverRequest* request);

	void ShowSettings(TWebserverRequest* request);
	void ShowChannelList(TWebserverRequest* request);
	bool GetBouquetList();

	void ShowTimerList(TWebserverRequest *request);
	char *GetServiceName(int onidsid);
	void GetCurrentEPG(TWebserverRequest *request,unsigned onidSid);
	unsigned GetcurrentONIDSID();
	void SetZapitMode(int mode);
	bool Execute(TWebserverRequest* request);
	bool ExecuteCGI(TWebserverRequest* request);

	void Streaminfo(TWebserverRequest* request);



	void GetEventList(TWebserverRequest* request, TEventList *Events,unsigned onidSid, bool cgi = false);
	void GetEPG(TWebserverRequest *request,long long epgid,bool cgi=false);

	void GetEPGList();
	void ZapTo(char* target);
	void StopEPGScanning(TWebserverRequest* request, bool off);
	void GetcurrentVAPid(TWebserverRequest* request);
	void StopPlayback(TWebserverRequest* request); 

	void ParseString(TWebserverRequest *request,char *str,TParameterList * Parameter);

	void updateEvents(void);

};

#endif
