
#ifndef __sectionsdclient__
#define __sectionsdclient__

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>


#include <string>
#include <vector>

#include "../sectionsdMsg.h"
#include "eventserver.h"

using namespace std;

class CShortEPGData
{
	public:
		string 		title;
		string 		info1;
		string 		info2;
	CShortEPGData(){	title	= "";	info1 	= "";	info2 	= ""; };
};

class CEPGData
{
	public:
		unsigned long long			eventID;
		sectionsd::sectionsdTime	epg_times;
		string 		title;
		string 		info1;
		string 		info2;
		string 		date;
		string 		start;
		string 		end;
		int			done;
		char		fsk;
		string 		contentClassification;
		string 		userClassification;

	CEPGData(){
		eventID = 0;
		title	= "";
		info1 	= "";
		info2 	= "";
		date 	= "";
		start 	= "";
		end 	= "";
		done 	= -1;
		fsk	= 0;
		contentClassification	= "";
		userClassification		= "";
	};

};

class CChannelEvent
{
	public:
		unsigned	serviceID;
		unsigned long long	eventID;
		string		description;
		string		text;
		time_t		startTime;
		unsigned	duration;
};

typedef vector<CChannelEvent> CChannelEventList;

class CSectionsdClient
{
        int sock_fd;
		bool sectionsd_connect();
		bool send(char* data, int size);
		bool receive(char* data, int size);
		int readResponse(char* data = NULL, int size= 0);
		bool sectionsd_close();

	public:
		enum events
		{
			EVT_TIMESET,
			EVT_GOT_CN_EPG
		};

		bool getComponentTagsUniqueKey( unsigned long long uniqueKey, sectionsd::ComponentTagList& tags );

		bool getLinkageDescriptorsUniqueKey( unsigned long long uniqueKey, sectionsd::LinkageDescriptorList& descriptors );

		bool getNVODTimesServiceKey( unsigned serviceKey, sectionsd::NVODTimesList& nvod_list );

		bool getCurrentNextServiceKey( unsigned serviceKey, sectionsd::responseGetCurrentNextInfoChannelID& current_next );

		bool getIsTimeSet();

		void setPauseScanning( bool doPause );

		void setPauseSorting( bool doPause );

		void setServiceChanged( unsigned ServiceKey, bool requestEvent );

		CChannelEventList getChannelEvents();

		bool CSectionsdClient::getEPGid( unsigned long long eventid,time_t starttime,CEPGData * epgdata);

		bool CSectionsdClient::getEPGidShort( unsigned long long eventid,CShortEPGData * epgdata);


		/*
			ein beliebiges Event anmelden
		*/
		void registerEvent(unsigned int eventID, unsigned int clientID, string udsName);

		/*
			ein beliebiges Event abmelden
		*/
		void unRegisterEvent(unsigned int eventID, unsigned int clientID);

		/* construktor */
		CSectionsdClient();


};

#endif
