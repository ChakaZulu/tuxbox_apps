#ifndef __webdbox__
#define __webdbox__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include <vector>
#include <map>

#include "request.h"
#include "helper.h"
#include "controldclient.h"

#include "../controld/clientlib/controldclient.h"
#include "../../sections/clientlib/sectionsdclient.h"
#include "../../zapit/clientlib/zapitclient.h"


using namespace std;

#define EZAP_PORT 1505

#define SA struct sockaddr
#define SAI struct sockaddr_in


class TWebserver;
class TWebserverRequest;
class CWebserverCGI;

//-------------------------------------------------------------------------


class TWebDbox
{
	TWebserver * Parent;
	CControldClient controld;
	CSectionsdClient sectionsd;
	CZapitClient zapit;
	CZapitClient::BouquetChannelList ChannelList;
	map<unsigned, CChannelEvent *> ChannelListEvents;
	map<int, CZapitClient::BouquetChannelList> BouquetsList;
	CZapitClient::BouquetList BouquetList;


public:
	TWebDbox(TWebserver * server);
	~TWebDbox();

	CChannelEventList eList;

	bool ExecuteCGI(TWebserverRequest* request);

// get functions to collect data
	void GetChannelEvents();
	void GetEventList(TWebserverRequest* request, unsigned onidSid, bool cgi = false);
	char* GetServiceName(int onid_sid);

	bool GetBouquets(void)
	{
		BouquetList.clear();
		zapit.getBouquets(BouquetList); 
		return true;
	};

	bool GetBouquet(unsigned int BouquetNr)
	{
		BouquetsList[BouquetNr].clear();
		zapit.getBouquetChannels(BouquetNr,BouquetsList[BouquetNr]);
		return true;
	};

	bool GetChannelList(void)
	{
		ChannelList.clear();
		zapit.getChannels(ChannelList);
		return true;
	};



// send functions for ExecuteCGI (controld api)
	void SendcurrentVAPid(TWebserverRequest* request);
	void SendSettings(TWebserverRequest* request);
	void SendStreaminfo(TWebserverRequest* request);
	void SendBouquets(TWebserverRequest *request);
	void SendBouquet(TWebserverRequest *request,int BouquetNr);
	void SendChannelList(TWebserverRequest *request);


	bool Execute(TWebserverRequest* request);
// show functions for Execute (web api)
	void ShowChannelList(TWebserverRequest* request,CZapitClient::BouquetChannelList channellist);
	void ShowBouquet(TWebserverRequest *request,int BouquetNr);
	void ShowBouquets(TWebserverRequest *request);
	void ShowControlpanel(TWebserverRequest* request);

// support functions
	void ZapTo(char* target);
	void UpdateBouquets(void);


// alt
	void GetEPG(TWebserverRequest *request,unsigned long long epgid, time_t *,bool cgi=false);
};

#endif
