#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <list>
#include <ext/hash_map>
#include <ext/stl_hash_fun.h>

#include "si.h"
#include "dvb.h"
#include "edvb.h"

#define CLEAN_INTERVAL 60000    //  1 min
#define UPDATE_INTERVAL 3600000  // 60 min
#define ZAP_DELAY 4000          // 4 sek

class eventData;
class eServiceReferenceDVB;

#define eventMap std::map<int, eventData*>

#if defined(__GNUC__) && __GNUC__ >= 3 && __GNUC_MINOR__ >= 1  // check if gcc version >= 3.1
	#define eventCache __gnu_cxx::hash_map<eServiceReferenceDVB, eventMap, __gnu_cxx::hash<eServiceReferenceDVB>, eServiceReferenceDVB::equalONIDSID>
	#define updateMap __gnu_cxx::hash_map<eServiceReferenceDVB, time_t, __gnu_cxx::hash<eServiceReferenceDVB>, eServiceReferenceDVB::equalONIDSID >
	namespace __gnu_cxx
#else																													// for older gcc use following
	#define eventCache std::hash_map<eServiceReferenceDVB, eventMap, std::hash<eServiceReferenceDVB>, eServiceReferenceDVB::equalONIDSID >
	#define updateMap std::hash_map<eServiceReferenceDVB, time_t, std::hash<eServiceReferenceDVB>, eServiceReferenceDVB::equalONIDSID >
	namespace std
#endif
{
struct hash<eServiceReferenceDVB>
{
	inline size_t operator()(const eServiceReferenceDVB &x) const
	{
		int v=(x.getOriginalNetworkID().get()^x.getServiceID().get());
		v^=v>>8;
		return v&0xFF;
	}
};
}

class eventData
{
/*public:
	enum TYP {SHORT, FULL};*/
private:
	char* EITdata;
	int ByteSize;
public:
//	TYP type;
	static int CacheSize;
	eventData(const eit_event_struct* e, int size/*, enum TYP t*/)
	:ByteSize(size)//, type(t)
	{
		CacheSize+=size;
		EITdata = new char[size];
		memcpy(EITdata, (char*) e, size);
	}
	~eventData()
	{
		CacheSize-=ByteSize;
		delete [] EITdata;
	}	
	operator const eit_event_struct*() const
	{
		return (const eit_event_struct*) EITdata;
	}
	const eit_event_struct* get() const
	{
		return (const eit_event_struct*) EITdata;
	}
};

class eEPGCache: public eSection
{
private:
	eServiceReferenceDVB current_service;
	int current_sid;
	int firstEventId;
	int isRunning;
	int sectionRead(__u8 *data);
	static eEPGCache *instance;

	eventCache eventDB;
	updateMap serviceLastUpdated;
	updateMap temp;

	eTimer CleanTimer;
	eTimer zapTimer;
public:
	inline void startEPG();
	inline void stopEPG(const eServiceReferenceDVB &);
	inline void enterService(const eServiceReferenceDVB &, int);
	void cleanLoop();
	void timeUpdated();
public:
	eEPGCache();
	~eEPGCache();
	static eEPGCache *getInstance() { return instance; }
//	EITEvent *lookupEvent(const eServiceReferenceDVB &service, int event_id);
	EITEvent *lookupCurrentEvent(const eServiceReferenceDVB &service);
	inline const eventMap* eEPGCache::getEventMap(const eServiceReferenceDVB &service);

	Signal1<void, bool> EPGAvail;
};

inline void eEPGCache::enterService(const eServiceReferenceDVB &service, int err)
{
	current_service = service;
	firstEventId = 0;

	updateMap::iterator It = serviceLastUpdated.find(service);

	int update;

	if (!err)
	{
		update = ( It != serviceLastUpdated.end() ? ( UPDATE_INTERVAL - ( (time(0)+eDVB::getInstance()->time_difference-It->second) * 1000 ) ) : ZAP_DELAY );

		if (update < ZAP_DELAY)
			update = ZAP_DELAY;

		zapTimer.start(update, 1);
		if (update >= 60000)
			eDebug("[EPGC] next update in %i min", update/60000);
		else if (update >= 1000)
			eDebug("[EPGC] next update in %i sec", update/1000);
	}

	if (It != serviceLastUpdated.end() && !eventDB[service].empty())
	{
		eDebug("[EPGC] service has EPG");
		/*emit*/ EPGAvail(1);
	}
	else
	{
		eDebug("[EPGC] service has no EPG");
		/*emit*/ EPGAvail(0);
	}
}

inline void eEPGCache::startEPG()
{
	if (eDVB::getInstance()->time_difference)	
	{
		temp.clear();
		eDebug("[EPGC] start caching events");
		firstEventId=0;
		start();
		isRunning=1;
	}
	else
	{
		eDebug("[EPGC] wait for clock update");
		zapTimer.start(1000, 1); // restart Timer
	}
}

inline void eEPGCache::stopEPG(const eServiceReferenceDVB&)
{
	zapTimer.stop();
	if (isRunning)
	{
		abort();
		isRunning=0;
		eDebug("[EPGC] stop caching events");
	}
}

inline const eventMap* eEPGCache::getEventMap(const eServiceReferenceDVB &service)
{
	eventCache::iterator It = eventDB.find(service);
	return (It != eventDB.end())?(&(It->second)):0;
}


#endif
