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
#include "../../sections/sectionsdMsg.h"


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
		for(channel = Head,i=0;(i <= index)  && (i < Count);i++)
			channel = channel->Next;
		return(channel);
	}

};
//-------------------------------------------------------------------------


class TWebDbox
{
	TWebserver * Parent;
	TChannelList *ChannelList;
	CControldClient controld;
	map<unsigned, char*> ChannelListEvents;

	time_t EPGDate;

public:
	TWebDbox(TWebserver * server);
	~TWebDbox();

	bool GetChannelList();
	bool PublishChannelList(TWebserverRequest* request);

	void ShowSettings(TWebserverRequest* request);
	void ShowChannelList(TWebserverRequest* request);
	void ShowTimerList(TWebserverRequest *request);
	char *GetServiceName(int onidsid);
	void GetCurrentEPG(TWebserverRequest *request,unsigned onidSid);
	unsigned GetcurrentONIDSID();
	void SetZapitMode(int mode);
	bool Execute(TWebserverRequest* request);
	bool ExecuteCGI(TWebserverRequest* request);

	void Streaminfo(TWebserverRequest* request);



	void GetEventList(TWebserverRequest* request, unsigned onidSid, bool cgi = false);
	void GetEPG(TWebserverRequest *request,long long epgid,bool cgi=false);

	void GetEPGList();
	void ZapTo(char* target);
	void StopEPGScanning(TWebserverRequest* request, bool off);
	void GetcurrentVAPid(TWebserverRequest* request);
	void StopPlayback(TWebserverRequest* request);

	void ParseString(TWebserverRequest *request,char *str,TParameterList * Parameter);

};


class CWebserverCGI
{
	public:
		virtual bool testHandler(string){return false;};
		virtual void exec(TWebserverRequest* request){};
};

#endif
