#ifndef __webdbox__
#define __webdbox__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include <vector>
#include <map>

#include <zapit/zapitclient.h>
#include <controldclient/controldclient.h>
#include <sectionsdclient/sectionsdclient.h>
#include <timerdclient/timerdclient.h>
#include <eventserver.h>

#include "request.h"


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

	void timerEventType2Str(CTimerd::CTimerEventTypes type, char *str,int len);
	void timerEventRepeat2Str(CTimerd::CTimerEventRepeat rep, char *str,int len);


public:
	CWebDbox(CWebserver * server);
	~CWebDbox();

	CChannelEventList eList;

	CControlAPI		*ControlAPI;
	CWebAPI			*WebAPI;
	CBouqueteditAPI	*BouqueteditAPI;

	friend class CWebAPI;
	friend class CControlAPI;
	friend class CBouqueteditAPI;
};

#endif
