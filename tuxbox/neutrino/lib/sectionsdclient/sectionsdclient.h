
#ifndef __sectionsdclient__
#define __sectionsdclient__

#include <string>
#include <vector>

#include <zapit/client/basicclient.h>


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
		char		fsk;
		string 		contentClassification;
		string 		userClassification;

	CEPGData(){
		eventID = 0;
		title	= "";
		info1 	= "";
		info2 	= "";
		fsk	= 0;
		contentClassification	= "";
		userClassification		= "";
	};

};

class CChannelEvent
{
	public:
		unsigned			serviceID( void ) const { return ( eventID>>16 ); }
		unsigned long long	eventID;
		string		description;
		string		text;
		time_t		startTime;
		unsigned	duration;
};

typedef vector<CChannelEvent> CChannelEventList;

class CSectionsdClient : private CBasicClient
{
 private:

	int readResponse(char* data = NULL, int size= 0);
	bool send(const unsigned char command, char* data, const unsigned int size, const unsigned char version);

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

		void setEventsAreOldInMinutes(const unsigned short minutes);

		void setPauseScanning(const bool doPause);

		void setPauseSorting(const bool doPause);

		void setServiceChanged(const unsigned ServiceKey, const bool requestEvent);

		CChannelEventList getChannelEvents();

		CChannelEventList getEventsServiceKey( unsigned serviceKey );

		bool getEPGid( unsigned long long eventid, time_t starttime, CEPGData * epgdata);

		bool getActualEPGServiceKey( unsigned serviceKey, CEPGData * epgdata);

		bool getEPGidShort( unsigned long long eventid,CShortEPGData * epgdata);


		/*
			ein beliebiges Event anmelden
		*/
		void registerEvent(unsigned int eventID, unsigned int clientID, string udsName);

		/*
			ein beliebiges Event abmelden
		*/
		void unRegisterEvent(unsigned int eventID, unsigned int clientID);

};

#endif
