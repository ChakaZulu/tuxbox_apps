
#ifndef __sectionsdclient__
#define __sectionsdclient__

#include <string>
#include <vector>

#include <connection/basicclient.h>
#include <zapit/client/zapittypes.h>  // t_channel_id


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

		bool getNVODTimesServiceKey(const t_channel_id channel_id, sectionsd::NVODTimesList& nvod_list );

		bool getCurrentNextServiceKey(const t_channel_id channel_id, sectionsd::responseGetCurrentNextInfoChannelID& current_next );

		bool getIsTimeSet();

		void setEventsAreOldInMinutes(const unsigned short minutes);

		void setPauseScanning(const bool doPause);

		void setPauseSorting(const bool doPause);

		void setServiceChanged(const t_channel_id channel_id, const bool requestEvent);

		CChannelEventList getChannelEvents();

		CChannelEventList getEventsServiceKey(const t_channel_id channel_id);

		bool getEPGid( unsigned long long eventid, time_t starttime, CEPGData * epgdata);

		bool getActualEPGServiceKey(const t_channel_id channel_id, CEPGData * epgdata);

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
