#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <list>
#include <ext/hash_map>
#include <ext/hash_set>

#include <errno.h>

#include "si.h"
#include "dvb.h"
#include "edvb.h"
#include <lib/base/ebase.h>
#include <lib/base/thread.h>
#include <lib/base/message.h>

#define CLEAN_INTERVAL 60000    //  1 min
#define UPDATE_INTERVAL 3600000  // 60 min
#define ZAP_DELAY 2000          // 2 sek

#define HILO(x) (x##_hi << 8 | x##_lo)

class eventData;
class eServiceReferenceDVB;

struct uniqueEPGKey
{
	int sid, onid, tsid;
	uniqueEPGKey( const eServiceReferenceDVB &ref )
		:sid( ref.type != eServiceReference::idInvalid ? ref.getServiceID().get() : -1 )
		,onid( ref.type != eServiceReference::idInvalid ? ref.getOriginalNetworkID().get() : -1 )
		,tsid( ref.type != eServiceReference::idInvalid ? ref.getTransportStreamID().get() : -1 )
	{
	}
	uniqueEPGKey()
		:sid(-1), onid(-1), tsid(-1)
	{
	}
	uniqueEPGKey( int sid, int onid, int tsid )
		:sid(sid), onid(onid), tsid(tsid)
	{
	}
	bool operator <(const uniqueEPGKey &a) const
	{
		return memcmp( &sid, &a.sid, sizeof(int)*3)<0;
	}
	operator bool() const
	{ 
		return !(sid == -1 && onid == -1 && tsid == -1); 
	}
	bool operator==(const uniqueEPGKey &a) const
	{
		return !memcmp( &sid, &a.sid, sizeof(int)*3);
	}
	struct equal
	{
		bool operator()(const uniqueEPGKey &a, const uniqueEPGKey &b) const
		{
			return !memcmp( &a.sid, &b.sid, sizeof(int)*3);
		}
	};
};

//eventMap is sorted by event_id
#define eventMap std::map<__u16, eventData*>
//timeMap is sorted by beginTime
#define timeMap std::map<time_t, eventData*>

#define tmpMap std::map<uniqueEPGKey, std::pair<time_t, int> >
#define nvodMap std::map<uniqueEPGKey, std::list<NVODReferenceEntry> >

#ifdef ENABLE_PRIVATE_EPG
#define contentMap std::map<int, std::map<time_t, std::pair<time_t, __u16> > >
#define contentMaps std::map<uniqueEPGKey, contentMap>
#endif

struct hash_32
{
	inline size_t operator()( const __u32 &x) const
	{
		return (x >> 8)&0xFFFF;
	}
};

struct equal_32
{
	inline size_t operator()(const __u32 &x, const __u32 &y) const
	{
		return x == y;
	}
};

struct hash_uniqueEPGKey
{
	inline size_t operator()( const uniqueEPGKey &x) const
	{
		return (x.onid << 16) | x.tsid;
	}
};

#if defined(__GNUC__) && ((__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || __GNUC__ == 4 )  // check if gcc version >= 3.1
	#define eventCache __gnu_cxx::hash_map<uniqueEPGKey, std::pair<eventMap, timeMap>, hash_uniqueEPGKey, uniqueEPGKey::equal>
	#define updateMap __gnu_cxx::hash_map<uniqueEPGKey, time_t, hash_uniqueEPGKey, uniqueEPGKey::equal >
	#define tidMap __gnu_cxx::hash_set<__u32, hash_32, equal_32>
#else // for older gcc use following
	#define eventCache std::hash_map<uniqueEPGKey, std::pair<eventMap, timeMap>, hash_uniqueEPGKey, uniqueEPGKey::equal >
	#define updateMap std::hash_map<uniqueEPGKey, time_t, hash_uniqueEPGKey, uniqueEPGKey::equal >
	#define tidMap std::hash_map<__u32, hash_32, equal_32>
#endif

class eventData
{
 	friend class eEPGCache;
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
		if (e)
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
	int getEventID()
	{
		return HILO( ((const eit_event_struct*) EITdata)->event_id );
	}
	time_t getStartTime()
	{
		return parseDVBtime(
			((const eit_event_struct*)EITdata)->start_time_1,
			((const eit_event_struct*)EITdata)->start_time_2,
			((const eit_event_struct*)EITdata)->start_time_3,
			((const eit_event_struct*)EITdata)->start_time_4,
			((const eit_event_struct*)EITdata)->start_time_5);
	}
};

class eEPGCache;

class eSchedule: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eSchedule()  // 0x50 .. 0x5F
			:eSection(0x12, 0x50, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0)
	{
	}
};

class eScheduleOther: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eScheduleOther()  // 0x60 .. 0x6F
			:eSection(0x12, 0x60, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0)
	{
	}
};

class eNowNext: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eNowNext()  // 0x4E, 0x4F
		:eSection(0x12, 0x4E, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xFE)
	{
	}
};

#ifdef ENABLE_PRIVATE_EPG
class ePrivateContent: public eSection
{
	friend class eEPGCache;
	int sectionRead(__u8 *data);
	std::set<__u8> seenSections;
	ePrivateContent()
	{}
	void stop()
	{
		if ( pid )
		{
			abort();
			pid = 0;
		}
	}
	void start( int pid )
	{
		if ( pid != this->pid )
			start_filter(pid,-1);
	}
	void restart()
	{
		if ( pid )
			start_filter(pid);
	}
	void start_filter(int pid, int version=-1)
	{
		eDebug("[EPGC] start private content filter pid %04x, version %d", pid, version);
		seenSections.clear();
		setFilter(pid, 0xA0, -1, version, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xFF);
	}
};
#endif

class eEPGCache: public eMainloop, private eThread, public Object
{
public:
	enum {NOWNEXT, SCHEDULE, SCHEDULE_OTHER};
	friend class eSchedule;
	friend class eScheduleOther;
	friend class eNowNext;
#ifdef ENABLE_PRIVATE_EPG
	friend class ePrivateContent;
#endif
	struct Message
	{
		enum
		{
			flush,
			enterService,
			leaveService,
			pause,
			restart,
			updated,
			isavail,
			quit,
			timeChanged,
			content_pid,
			save,
			load
		};
		int type;
		uniqueEPGKey service;
		union {
			int err;
			int pid;
			time_t time;
			bool avail;
		};
		Message()
			:type(0), time(0) {}
		Message(int type)
			:type(type) {}
		Message(int type, int pid)
			:type(type), pid(pid) {}
		Message(int type, bool b)
			:type(type), avail(b) {}
		Message(int type, const eServiceReferenceDVB& service, int err=0)
			:type(type), service(service), err(err) {}
		Message(int type, time_t time)
			:type(type), time(time) {}
	};
	eFixedMessagePump<Message> messages;
private:
	static pthread_mutex_t cache_lock;
	uniqueEPGKey current_service;
	int paused;

	int state;
	__u8 isRunning, firstStart, haveData;
	int sectionRead(__u8 *data, int source);
	static eEPGCache *instance;

	eventCache eventDB;
	updateMap serviceLastUpdated;
	tmpMap temp;
	nvodMap NVOD;
	tidMap seenSections, calcedSections;
	eSchedule scheduleReader;
	eScheduleOther scheduleOtherReader;
	eNowNext nownextReader;
#ifdef ENABLE_PRIVATE_EPG
	contentMaps content_time_tables;
	ePrivateContent contentReader;
	void setContentPid(int pid);
#endif
	eTimer CleanTimer;
	eTimer zapTimer;
	eTimer abortTimer;
	bool finishEPG();
	void abortNonAvail();
	void flushEPG(const uniqueEPGKey & s=uniqueEPGKey());
	void startEPG();

	void changedService(const uniqueEPGKey &, int);
	void abortEPG();

	// called from other thread context !!
	void enterService(const eServiceReferenceDVB &, int);
	void leaveService(const eServiceReferenceDVB &);

	void cleanLoop();
	void pauseEPG();
	void restartEPG();
	void thread();
	void gotMessage(const Message &message);
	void timeUpdated();
	void save();
	void load();
public:
	eEPGCache();
	~eEPGCache();
	static eEPGCache *getInstance() { return instance; }

	inline void Lock();
	inline void Unlock();

	const eventMap* getEventMap(const eServiceReferenceDVB &service);
	const timeMap* getTimeMap(const eServiceReferenceDVB &service);
	tmpMap* getUpdatedMap() { return &temp; }
	const std::list<NVODReferenceEntry>* getNVODRefList(const eServiceReferenceDVB &service);

	EITEvent *lookupEvent(const eServiceReferenceDVB &service, int event_id, bool plain=false );
	EITEvent *lookupEvent(const eServiceReferenceDVB &service, time_t=0, bool plain=false );

	Signal1<void, bool> EPGAvail;
	Signal0<void> EPGUpdated;
};

inline const std::list<NVODReferenceEntry>* eEPGCache::getNVODRefList(const eServiceReferenceDVB &service)
{
	nvodMap::iterator It = NVOD.find( service );
	if ( It != NVOD.end() && It->second.size() )
		return &(It->second);
	else
		return 0;
}

inline const eventMap* eEPGCache::getEventMap(const eServiceReferenceDVB &service)
{
	eventCache::iterator It = eventDB.find( service );
	if ( It != eventDB.end() && It->second.first.size() )
		return &(It->second.first);
	else
		return 0;
}

inline const timeMap* eEPGCache::getTimeMap(const eServiceReferenceDVB &service)
{
	eventCache::iterator It = eventDB.find( service );
	if ( It != eventDB.end() && It->second.second.size() )
		return &(It->second.second);
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

inline int eScheduleOther::sectionRead(__u8 *data)
{
	return eEPGCache::getInstance()->sectionRead(data, eEPGCache::SCHEDULE_OTHER);
}

inline void eSchedule::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & 1) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		e->isRunning &= ~1;
		if (e->haveData)
			e->finishEPG();
	}
}

inline void eScheduleOther::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & 4) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		e->isRunning &= ~4;
		if (e->haveData)
			e->finishEPG();
	}
}

inline void eNowNext::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & 2) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		e->isRunning &= ~2;
		if (e->haveData)
			e->finishEPG();
	}
}

inline void eEPGCache::Lock()
{
	pthread_mutex_lock(&cache_lock);
}

inline void eEPGCache::Unlock()
{
	pthread_mutex_unlock(&cache_lock);
}

#endif
