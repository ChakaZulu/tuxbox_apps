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
#include "controldclient.h"

#include "../controld/clientlib/controldclient.h"
#include "../../sections/clientlib/sectionsdclient.h"
#include "../../zapit/clientlib/zapitclient.h"
#include "../timerd/clientlib/timerdclient.h"


using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

extern string b64decode(char *s);



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
	CTimerdClient timerd;
	CZapitClient::BouquetChannelList ChannelList;
	map<unsigned, CChannelEvent *> ChannelListEvents;
	map<int, CZapitClient::BouquetChannelList> BouquetsList;
	CZapitClient::BouquetList BouquetList;


public:
	TWebDbox(TWebserver * server);
	~TWebDbox();

	CChannelEventList eList;


// get functions to collect data
	void GetChannelEvents();

	string GetServiceName(int onid_sid);

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

	bool ExecuteCGI(CWebserverRequest* request);
// send functions for ExecuteCGI (controld api)
	void SendEventList(CWebserverRequest *request,unsigned onidSid);
	void SendcurrentVAPid(CWebserverRequest* request);
	void SendSettings(CWebserverRequest* request);
	void SendStreaminfo(CWebserverRequest* request);
	void SendBouquets(CWebserverRequest *request);
	void SendBouquet(CWebserverRequest *request,int BouquetNr);
	void SendChannelList(CWebserverRequest *request);


	bool Execute(CWebserverRequest* request);
// show functions for Execute (web api)
	void ShowEventList(CWebserverRequest* request, unsigned onidSid);
	void ShowBouquet(CWebserverRequest *request,int BouquetNr = -1);
	void ShowBouquets(CWebserverRequest *request);
	bool ShowControlpanel(CWebserverRequest* request);
	void ShowSettings(CWebserverRequest *request);

// support functions
	void ZapTo(string target);
	void UpdateBouquets(void);

	bool Authenticate(CWebserverRequest* request);
	bool CheckAuth(CWebserverRequest* request);

// alt
	void GetEPG(CWebserverRequest *request,unsigned long long epgid, time_t *,bool cgi=false);
};

#endif
