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
class CWebserverRequest;
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

	bool ExecuteCGI(CWebserverRequest* request);

// get functions to collect data
	void GetChannelEvents();
	void GetEventList(CWebserverRequest* request, unsigned onidSid, bool cgi = false);
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
	void SendcurrentVAPid(CWebserverRequest* request);
	void SendSettings(CWebserverRequest* request);
	void SendStreaminfo(CWebserverRequest* request);
	void SendBouquets(CWebserverRequest *request);
	void SendBouquet(CWebserverRequest *request,int BouquetNr);
	void SendChannelList(CWebserverRequest *request);


	bool Execute(CWebserverRequest* request);
// show functions for Execute (web api)
//	void ShowChannelList(CWebserverRequest* request,int BouquetNr = -1);
	void ShowBouquet(CWebserverRequest *request,int BouquetNr = -1);
	void ShowBouquets(CWebserverRequest *request);
	void ShowControlpanel(CWebserverRequest* request);
	void ShowSettings(CWebserverRequest *request);

// support functions
	void ZapTo(char* target);
	void UpdateBouquets(void);


// alt
	void GetEPG(CWebserverRequest *request,unsigned long long epgid, time_t *,bool cgi=false);
};

#endif
