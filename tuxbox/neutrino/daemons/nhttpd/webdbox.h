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
#include "../libevent/eventserver.h"



using namespace std;

#define SA struct sockaddr
#define SAI struct sockaddr_in

extern string b64decode(char *s);
extern string itoh(unsigned int conv);
extern string itoa(unsigned int conv);


class CWebserver;
class CWebserverRequest;
class CControlAPI;
class CWebAPI;
class CBouqueteditAPI;
class CWebDbox;

#include "controlapi.h"
#include "bouqueteditapi.h"
#include "webapi.h"

//-------------------------------------------------------------------------


class CWebDbox
{
	CWebserver * Parent;

	// Clientlibs
	CControldClient	*Controld;
	CSectionsdClient *Sectionsd;
	CZapitClient	*Zapit;
	CTimerdClient	*Timerd;


	CZapitClient::BouquetChannelList ChannelList;					// complete channellist
	map<unsigned, CChannelEvent *> ChannelListEvents;				// events of actual channel
	map<int, CZapitClient::BouquetChannelList> BouquetsList;		// List of available bouquets
	CZapitClient::BouquetList BouquetList;							// List of current bouquet
	CEventServer	*EventServer;


//	bool standby_mode;

	// some constants
	string Dbox_Hersteller[4];
	string videooutput_names[3];
	string videoformat_names[3];
	string audiotype_names[5];
//	map<unsigned, string> TimerEventNames;		
//	map<unsigned, string> TimerEventStateNames;
/*
// send functions for ExecuteCGI (controld api)
	void SendEventList(CWebserverRequest *request,t_channel_id channel_id);
	void SendcurrentVAPid(CWebserverRequest* request);
	void SendSettings(CWebserverRequest* request);
	void SendStreamInfo(CWebserverRequest* request);
	void SendBouquets(CWebserverRequest *request);
	void SendBouquet(CWebserverRequest *request,int BouquetNr);
	void SendChannelList(CWebserverRequest *request);
// CGI functions for ExecuteCGI
	bool TimerCGI();
	bool SetModeCGI();
	bool StandbyCGI();
	bool GetDateCGI();
	bool GetTimeCGI();
	bool SettingsCGI();
	bool GetServicesxmlCGI();
	bool GetBouquetsxmlCGI();
	bool GetChannel_IDCGI();
	bool MessageCGI();
	bool InfoCGI();
	bool ShutdownCGI();
	bool VolumeCGI();
	bool ChannellistCGI();
	bool GetBouquetCGI();
	bool GetBouquetsCGI();
	bool EpgCGI();
	bool VersionCGI();
	bool ZaptoCGI();

// show functions for Execute (web api)
	void ShowDboxMenu(CWebserverRequest* request);
	void ShowTimerList(CWebserverRequest* request);
	void ShowEventList(CWebserverRequest* request, t_channel_id channel_id);
	void ShowBouquet(CWebserverRequest *request,int BouquetNr = -1);
	void ShowBouquets(CWebserverRequest *request, unsigned int BouquetNr = 0);
	bool ShowControlpanel(CWebserverRequest* request);
	void ShowCurrentStreamInfo(CWebserverRequest* request);
	bool ShowEpg(CWebserverRequest* request,string EpgID,string Startzeit = "");
	void ShowEPG(CWebserverRequest *request,string Title, string Info1, string Info2);
	bool ShowActualEpg(CWebserverRequest *request);
*/
// get functions to collect data
	void GetChannelEvents();
	bool GetStreamInfo(int bitinfo[10]);
	string GetServiceName(t_channel_id channel_id);
	bool GetBouquets(void);
	bool GetBouquet(unsigned int BouquetNr);
	bool GetChannelList(void);

// support functions
	void ZapTo(string target);
	void UpdateBouquets(void);

	void timerEventType2Str(CTimerEvent::CTimerEventTypes type, char *str,int len);
	void timerEventRepeat2Str(CTimerEvent::CTimerEventRepeat rep, char *str,int len);


public:
	CWebDbox(CWebserver * server);
	~CWebDbox();

	CChannelEventList eList;

	CControlAPI		*ControlAPI;
	CWebAPI			*WebAPI;
	CBouqueteditAPI	*BouqueteditAPI;


//	bool ExecuteCGI(CWebserverRequest* request);


//	bool Execute(CWebserverRequest* request);



	friend class CWebAPI;
	friend class CControlAPI;
	friend class CBouqueteditAPI;
};

#endif
