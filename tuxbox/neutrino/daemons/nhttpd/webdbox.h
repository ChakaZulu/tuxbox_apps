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
//#include "controldclient.h"

#include "../controld/clientlib/controldclient.h"
#include "../../sections/clientlib/sectionsdclient.h"
#include "../../zapit/clientlib/zapitclient.h"
#include "../timerd/clientlib/timerdclient.h"


using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

extern string b64decode(char *s);
extern string itoh(unsigned int conv);
extern string itoa(unsigned int conv);


class CWebserver;
class CWebserverRequest;
class CWebserverCGI;

//-------------------------------------------------------------------------


class TWebDbox
{
	CWebserver * Parent;

	// Clientlibs
	CControldClient *controld;
	CSectionsdClient *sectionsd;
	CZapitClient *zapit;
	CTimerdClient *timerd;

	CZapitClient::BouquetChannelList ChannelList;					// complete channellist
	map<unsigned, CChannelEvent *> ChannelListEvents;				// events of actual channel
	map<int, CZapitClient::BouquetChannelList> BouquetsList;		// List of available bouquets
	CZapitClient::BouquetList BouquetList;							// List of current bouquet


	bool standby_mode;

	// some constants
	string Dbox_Hersteller[4];
	string videooutput_names[3];
	string videoformat_names[3];
	string audiotype_names[5];
	map<unsigned, string> TimerEventNames;							



public:
	TWebDbox(CWebserver * server);
	~TWebDbox();

	CChannelEventList eList;


// get functions to collect data
	void GetChannelEvents();
	bool GetStreamInfo(int bitinfo[10]);
	string GetServiceName(int onid_sid);
	bool GetBouquets(void);
	bool GetBouquet(unsigned int BouquetNr);
	bool GetChannelList(void);

	bool ExecuteCGI(CWebserverRequest* request);
// send functions for ExecuteCGI (controld api)
	void SendEventList(CWebserverRequest *request,unsigned onidSid);
	void SendcurrentVAPid(CWebserverRequest* request);
	void SendSettings(CWebserverRequest* request);
	void SendStreamInfo(CWebserverRequest* request);
	void SendBouquets(CWebserverRequest *request);
	void SendBouquet(CWebserverRequest *request,int BouquetNr);
	void SendChannelList(CWebserverRequest *request);


	bool Execute(CWebserverRequest* request);
// show functions for Execute (web api)
	void ShowDboxMenu(CWebserverRequest* request);
	void ShowTimerList(CWebserverRequest* request);
	void ShowEventList(CWebserverRequest* request, unsigned onidSid);
	void ShowBouquet(CWebserverRequest *request,int BouquetNr = -1);
	void ShowBouquets(CWebserverRequest *request, int BouquetNr = 0);
	bool ShowControlpanel(CWebserverRequest* request);
	void ShowCurrentStreamInfo(CWebserverRequest* request);
	bool ShowEpg(CWebserverRequest* request,string EpgID,string Startzeit = "");
	void ShowEPG(CWebserverRequest *request,string Title, string Info1, string Info2);
	bool ShowActualEpg(CWebserverRequest *request);


	bool ExecuteBouquetEditor(CWebserverRequest* request);
// BouquetEditor functions

// support functions
	void ZapTo(string target);
	void UpdateBouquets(void);

};

#endif
