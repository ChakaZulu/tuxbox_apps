#ifndef __controlapi__
#define __controlapi__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include "request.h"
#include "webdbox.h"


using namespace std;



class CControlAPI
{
	private:
		CWebDbox * Parent;

// send functions for ExecuteCGI (controld api)
		void SendEventList(CWebserverRequest *request,t_channel_id channel_id);
		void SendcurrentVAPid(CWebserverRequest* request);
		void SendSettings(CWebserverRequest* request);
		void SendStreamInfo(CWebserverRequest* request);
		void SendBouquets(CWebserverRequest *request);
		void SendBouquet(CWebserverRequest *request,int BouquetNr);
		void SendChannelList(CWebserverRequest *request);
		void SendTimers(CWebserverRequest* request);


// CGI functions for ExecuteCGI
		bool TimerCGI(CWebserverRequest *request);
		bool SetModeCGI(CWebserverRequest *request);
		bool StandbyCGI(CWebserverRequest *request);
		bool GetDateCGI(CWebserverRequest *request);
		bool GetTimeCGI(CWebserverRequest *request);
		bool SettingsCGI(CWebserverRequest *request);
		bool GetServicesxmlCGI(CWebserverRequest *request);
		bool GetBouquetsxmlCGI(CWebserverRequest *request);
		bool GetChannel_IDCGI(CWebserverRequest *request);
		bool MessageCGI(CWebserverRequest *request);
		bool InfoCGI(CWebserverRequest *request);
		bool ShutdownCGI(CWebserverRequest *request);
		bool VolumeCGI(CWebserverRequest *request);
		bool ChannellistCGI(CWebserverRequest *request);
		bool GetBouquetCGI(CWebserverRequest *request);
		bool GetBouquetsCGI(CWebserverRequest *request);
		bool EpgCGI(CWebserverRequest *request);
		bool VersionCGI(CWebserverRequest *request);
		bool ZaptoCGI(CWebserverRequest *request);

	public:
		CControlAPI(CWebDbox *parent){Parent = parent;};
		~CControlAPI(){};
		bool Execute(CWebserverRequest* request);
};

#endif
