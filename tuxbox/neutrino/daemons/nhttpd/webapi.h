#ifndef __webapi__
#define __webapi__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/timeb.h>

#include "request.h"
#include "webdbox.h"


using namespace std;

//-------------------------------------------------------------------------
class CWebAPI
{
	private:
		CWebDbox * Parent;

// show functions for Execute (web api)
		bool ShowDboxMenu(CWebserverRequest* request);
		bool ShowTimerList(CWebserverRequest* request);
		bool ShowEventList(CWebserverRequest* request, t_channel_id channel_id);
		bool ShowBouquet(CWebserverRequest *request,int BouquetNr = -1);
		bool ShowBouquets(CWebserverRequest *request, unsigned int BouquetNr = 0);
		bool ShowControlpanel(CWebserverRequest* request);
		bool ShowCurrentStreamInfo(CWebserverRequest* request);
		bool ShowEpg(CWebserverRequest* request,string EpgID,string Startzeit = "");
		bool ShowEPG(CWebserverRequest *request,string Title, string Info1, string Info2);
		bool ShowActualEpg(CWebserverRequest *request);

		bool Test(CWebserverRequest* request);
		bool Timer(CWebserverRequest* request);
		bool Info(CWebserverRequest* request);
		bool Dbox(CWebserverRequest* request);
		bool Bouquetlist(CWebserverRequest* request);
		bool Channellist(CWebserverRequest* request);
		bool Controlpanel(CWebserverRequest* request);
		bool ActualEPG(CWebserverRequest* request);
		bool EPG(CWebserverRequest* request);
		bool Switch(CWebserverRequest* request);

		void loadTimerMain(CWebserverRequest* request);
		void correctTime(struct tm *zt);
		void showTimer(CWebserverRequest *request);
		void modifyTimerForm(CWebserverRequest *request, unsigned timerId);
		void doModifyTimer(CWebserverRequest *request);
		void newTimerForm(CWebserverRequest *request);
		void doNewTimer(CWebserverRequest *request);


	public:
		CWebAPI(CWebDbox *parent){Parent = parent;};
		~CWebAPI(){};
		bool Execute(CWebserverRequest* request);
};

#endif
