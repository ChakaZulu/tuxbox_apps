#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <list>
#include <ext/hash_map>
#include <ext/stl_hash_fun.h>
#include <errno.h>

#include "si.h"
#include "dvb.h"
#include "edvb.h"

#define CLEAN_INTERVAL 60000    //  1 min
#define UPDATE_INTERVAL 3600000  // 60 min
#define ZAP_DELAY 4000          // 4 sek

class eventData;
class eServiceReferenceDVB;

struct uniqueEPGKey
{
	int sid, onid;
	uniqueEPGKey()
		:sid(0), onid(0)
	{
	}
	uniqueEPGKey( int sid, int onid )
		:sid(sid), onid(onid)
	{
	}
	struct equal
	{
		bool operator()(const uniqueEPGKey &a, const uniqueEPGKey &b) const
		{
			return (a.sid == b.sid && a.onid == b.onid);
		}
	};
};

class uniqueEvent
{
public:
	time_t beginTime;
	int event_id;
	uniqueEPGKey service;		
	uniqueEvent()
		:beginTime(-1), event_id(0)
	{
	}
	uniqueEvent( time_t t, int id, uniqueEPGKey key )
		:beginTime(t), event_id(id), service(key)
	{
	}
	bool valid()
	{
		return beginTime > -1;
	}
	bool operator == ( const uniqueEvent& e )
	{
		return (beginTime == e.beginTime) && (event_id == e.event_id) && (service.sid == e.service.sid) && (service.onid == e.service.onid);
	}
	uniqueEvent& operator = ( const uniqueEvent& e )
	{
		beginTime = e.beginTime;
		event_id = e.event_id;
		service = e.service;
		return *this;
	}
	void invalidate()
	{
		beginTime=-1;
	}
};


#define eventMap std::map<int, eventData*>

#if defined(__GNUC__) && __GNUC__ >= 3 && __GNUC_MINOR__ >= 1  // check if gcc version >= 3.1
	#define eventCache __gnu_cxx::hash_map<uniqueEPGKey, eventMap, __gnu_cxx::hash<uniqueEPGKey>, uniqueEPGKey::equal>
	#define updateMap __gnu_cxx::hash_map<uniqueEPGKey, time_t, __gnu_cxx::hash<uniqueEPGKey>, uniqueEPGKey::equal >
	#define tmpMap __gnu_cxx::hash_map<uniqueEPGKey, std::pair<time_t, int>, __gnu_cxx::hash<uniqueEPGKey>, uniqueEPGKey::equal >
	namespace __gnu_cxx
#else																													// for older gcc use following
	#define eventCache std::hash_map<uniqueEPGKey, eventMap, std::hash<uniqueEPGKey>, uniqueEPGKey::equalONIDSID >
	#define updateMap __gnu_cxx::hash_map<uniqueEPGKey, time_t, __gnu_cxx::hash<uniqueEPGKey>, uniqueEPGKey::equal >
	#define tmpMap std::hash_map<uniqueEPGKey, std::pair<time_t, int>, std::hash<uniqueEPGKey>, uniqueEPGKey::equalONIDSID >
	namespace std
#endif
{
struct hash<uniqueEPGKey>
{
	inline size_t operator()(const uniqueEPGKey &x) const
	{
		int v=(x.onid^x.sid);
		v^=v>>8;
		return v&0xFF;
	}
};
}

class eventData
{
private:
	__u8* EITdata;
	int ByteSize;
public:
	int type;
	static int CacheSize;
	eventData(const eit_event_struct* e, int size, int type)
	:ByteSize(size), type(type)
	{
		CacheSize+=size;
		EITdata = new __u8[size];
		memcpy(EITdata, (__u8*) e, size);
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

class eEPGCache;

class eSchedule: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eSchedule()  // 0x50, Filter 0xF0
//		:eSection(0x12, 80, -1, -1, SECREAD_CRC, 240)
			:eSection(0x12, 0x50, -1, -1, SECREAD_CRC, 0xF0)
	{
	}
};

class eNowNext: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eNowNext()  // 0x4E, 0x4F
//		:eSection(0x12, 78 , -1, -1, SECREAD_CRC, 254)
		:eSection(0x12, 0x4E, -1, -1, SECREAD_CRC, 0xFE)
	{
	}
};

class eEPGCache: public Object
{
public:
	enum {SCHEDULE, NOWNEXT};

	friend class eSchedule;
	friend class eNowNext;
private:
	uniqueEPGKey current_service;
	int current_sid;
	uniqueEvent firstScheduleEvent,
							firstNowNextEvent;
	int isRunning;
	int paused;
	int sectionRead(__u8 *data, int source);
	static eEPGCache *instance;

	eventCache eventDB;
	updateMap serviceLastUpdated;
	tmpMap temp;
	eSchedule scheduleReader;
	eNowNext nownextReader;
	eTimer CleanTimer;
	eTimer zapTimer;
	bool finishEPG();
public:
	void startEPG();
	void abortEPG(const eServiceReferenceDVB& s=eServiceReferenceDVB());
	void enterService(const eServiceReferenceDVB &, int);
	void cleanLoop();
	void timeUpdated();
	void pauseEPG();
	void restartEPG();
	eEPGCache();
	~eEPGCache();
	static eEPGCache *getInstance() { return instance; }
	EITEvent *lookupEvent(const eServiceReferenceDVB &service, int event_id);
	EITEvent *lookupEvent(const eServiceReferenceDVB &service, time_t=0);
	inline const eventMap* eEPGCache::getEventMap(const eServiceReferenceDVB &service);

	Signal1<void, bool> EPGAvail;
	Signal1<void, const tmpMap*> EPGUpdated;
};

inline const eventMap* eEPGCache::getEventMap(const eServiceReferenceDVB &service)
{
	eventCache::iterator It = eventDB.find( uniqueEPGKey( service.getServiceID().get(), service.getOriginalNetworkID().get() ) );
	if ( It != eventDB.end() && It->second.size() )
		return &(It->second);
	else
		return 0;
}

inline int eNowNext::sectionRead(__u8 *data)
{
	return eEPGCache::getInstance()->sectionRead(data, eEPGCache::NOWNEXT);
}

inline int eSchedule::sectionRead(__u8 *data)
{
	return eEPGCache::getInstance()->sectionRead(data, eEPGCache::SCHEDULE);
}

inline void eSchedule::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & 1) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		e->isRunning &= ~1;
		if ( e->firstScheduleEvent.valid() || e->firstNowNextEvent.valid() )
			e->finishEPG();
	}
}

inline void eNowNext::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & 2) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		e->isRunning &= ~2;
		if ( e->firstScheduleEvent.valid() || e->firstNowNextEvent.valid() )
			e->finishEPG();
	}
}

#endif
