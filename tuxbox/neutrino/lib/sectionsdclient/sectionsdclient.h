
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

		void setServiceChanged( unsigned ServiceKey, bool requestEvent );

		CChannelEventList getChannelEvents();


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
