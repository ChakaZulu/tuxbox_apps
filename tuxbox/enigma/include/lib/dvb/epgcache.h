#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <list>
#include <ext/hash_map>
#include <ext/hash_set>

#include <errno.h>

#include "si.h"
#ifdef ENABLE_MHW_EPG
#include "lowlevel/mhw.h"
#endif
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

struct hash_uniqueEPGKey
{
	inline size_t operator()( const uniqueEPGKey &x) const
	{
		return (x.tsid << 16) | x.sid;
	}
};

#define tidMap std::set<__u32>
#define descriptorPair std::pair<__u16,__u8*>

#if defined(__GNUC__) && ((__GNUC__ == 3 && __GNUC_MINOR__ >= 1) || __GNUC__ == 4 )  // check if gcc version >= 3.1
	#define eventCache __gnu_cxx::hash_map<uniqueEPGKey, std::pair<eventMap, timeMap>, hash_uniqueEPGKey, uniqueEPGKey::equal>
	#define updateMap __gnu_cxx::hash_map<uniqueEPGKey, time_t, hash_uniqueEPGKey, uniqueEPGKey::equal >
	#define descriptorMap __gnu_cxx::hash_map<__u32, descriptorPair >
	#ifdef ENABLE_PRIVATE_EPG
		#define contentTimeMap __gnu_cxx::hash_map<time_t, std::pair<time_t, __u16> >
		#define contentMap __gnu_cxx::hash_map<int, contentTimeMap >	    
		#define contentMaps __gnu_cxx::hash_map<uniqueEPGKey, contentMap, hash_uniqueEPGKey, uniqueEPGKey::equal >
	#endif
#else // for older gcc use following
	#define eventCache std::hash_map<uniqueEPGKey, std::pair<eventMap, timeMap>, hash_uniqueEPGKey, uniqueEPGKey::equal >
	#define updateMap std::hash_map<uniqueEPGKey, time_t, hash_uniqueEPGKey, uniqueEPGKey::equal >
	#define descriptorMap std::hash_map<__u32, descriptorPair, hash_descriptor >
	#ifdef ENABLE_PRIVATE_EPG
		#define contentTimeMap std::hash_map<time_t, std::pair<time_t, __u16> >
		#define contentMap std::hash_map<int, contentTimeMap >
		#define contentMaps std::hash_map<uniqueEPGKey, contentMap, hash_uniqueEPGKey, uniqueEPGKey::equal>
	#endif
#endif

class eventData
{
 	friend class eEPGCache;
private:
	__u8* EITdata;
	__u8 ByteSize;
	static descriptorMap descriptors;
	static __u8 data[4108];
	void init_eventData(const eit_event_struct* e, int size, int type);
#ifdef ENABLE_FREESAT_EPG
	__u8* DecodeFreesat(__u8* descr,int descr_len);
	const char* freesat_huffman_to_string(const char *src, uint size);
#endif
public:
	__u8 type;
	static int CacheSize;
	static void load(FILE *);
	static void save(FILE *);
	eventData(const eit_event_struct* e, int size, int type);
	~eventData();
	const eit_event_struct* get() const;
	operator const eit_event_struct*() const
	{
		return get();
	}
	int getEventID()
	{
		return (EITdata[0] << 8) | EITdata[1];
	}
	time_t getStartTime()
	{
		return parseDVBtime(EITdata[2], EITdata[3], EITdata[4], EITdata[5], EITdata[6]);
	}
	int getDuration()
	{
		return fromBCD(EITdata[7])*3600+fromBCD(EITdata[8])*60+fromBCD(EITdata[9]);
	}
	bool search(int tsidonid, const eString &search, int intExactMatch, int intCaseSensitive, int genre, int Range);
};

class eEPGCache;

class eSchedule: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eSchedule()
	{}
	inline void start()
	{ // 0x50 .. 0x5F
		setFilter(0x12, 0x50, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0);
	}
};

class eScheduleOther: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eScheduleOther()
	{}
	inline void start()
	{ // 0x60 .. 0x6F
		setFilter(0x12, 0x60, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0);
	}
};

#ifdef ENABLE_MHW_EPG
class eScheduleMhw: public eSection
{
	friend class eEPGCache;
	std::vector<mhw_channel_name_t> channels;
	std::map<__u8, mhw_theme_name_t> themes;
	std::map<__u32, mhw_title_t> titles;
	std::map<__u32, __u32> program_ids;
	time_t tnew_summary_read;
	time_t tnew_title_read;
	
	void cleanup();
	__u8 *delimitName( __u8 *in, __u8 *out, int len_in );
	void eScheduleMhw::timeMHW2DVB( u_char hours, u_char minutes, u_char *return_time);
	void eScheduleMhw::timeMHW2DVB( int minutes, u_char *return_time);
	void eScheduleMhw::timeMHW2DVB( u_char day, u_char hours, u_char minutes, u_char *return_time);
	void eScheduleMhw::storeTitle(std::map<__u32, mhw_title_t>::iterator itTitle, 
		eString sumText, __u8 *data);
	int sectionRead(__u8 *data);
	void sectionFinish(int);
	int start()
	{
		return setFilter( 0xD3, 0x91, -1, -1, SECREAD_NOTIMEOUT, 0xFF );
	}
	eScheduleMhw()	{}
};
#endif

#ifdef ENABLE_FREESAT_EPG
class eScheduleFreesat: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eScheduleFreesat()
	{}
	int start()
	{// 0x60 , 0x61
		return setFilter( 3842, 0x60, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xFE );
	}
};
#endif

class eNowNext: public eSection
{
	friend class eEPGCache;
	inline int sectionRead(__u8 *data);
	inline void sectionFinish(int);
	eNowNext()
	{}
	inline void start()
	{  // 0x4E, 0x4F
		setFilter(0x12, 0x4E, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xFE);
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
#ifdef ENABLE_MHW_EPG
	friend class eScheduleMhw;
#endif
#ifdef ENABLE_FREESAT_EPG
	friend class eScheduleFreesat;
#endif
#ifdef ENABLE_PRIVATE_EPG
	friend class ePrivateContent;
#endif
	enum {
#ifdef ENABLE_PRIVATE_EPG
		PRIVATE=0,
#endif
		NOWNEXT=1,
		SCHEDULE=2,
		SCHEDULE_OTHER=4
#ifdef ENABLE_MHW_EPG
		,SCHEDULE_MHW=8
#endif
#ifdef ENABLE_FREESAT_EPG
		,SCHEDULE_FREESAT=16
#endif
	};
	friend class eSchedule;
	friend class eScheduleOther;
	friend class eNowNext;
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
	// needed for caching current service until the thread has started
	// (valid transponder time received)
	eServiceReferenceDVB cached_service;
	int cached_err;

	static pthread_mutex_t cache_lock;
	uniqueEPGKey current_service;
	int paused;
	int isLoading;

	int state;
	__u8 isRunning, firstStart, haveData;
	bool FixOverlapping(std::pair<eventMap,timeMap> &, time_t, int, const timeMap::iterator &, const uniqueEPGKey &);
	int sectionRead(__u8 *data, int source);
	static eEPGCache *instance;

	eventCache eventDB;
	updateMap serviceLastUpdated;
	tmpMap temp;
	nvodMap NVOD;
	tidMap seenSections[3], calcedSections[3];
	eSchedule scheduleReader;
	eScheduleOther scheduleOtherReader;
#ifdef ENABLE_MHW_EPG
	eScheduleMhw scheduleMhwReader;
#endif
#ifdef ENABLE_FREESAT_EPG
	eScheduleFreesat scheduleFreesatReader;
#endif
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
	void init_eEPGCache();
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

#ifdef ENABLE_FREESAT_EPG
inline int eScheduleFreesat::sectionRead(__u8 *data)
{
	return eEPGCache::getInstance()->sectionRead(data, eEPGCache::SCHEDULE_FREESAT);
}
#endif

inline void eSchedule::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & eEPGCache::SCHEDULE) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		eDebug("[EPGC] stop schedule");
		e->isRunning &= ~eEPGCache::SCHEDULE;
		if (e->haveData)
			e->finishEPG();
	}
}

inline void eScheduleOther::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & eEPGCache::SCHEDULE_OTHER) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		eDebug("[EPGC] stop schedule other");
		e->isRunning &= ~eEPGCache::SCHEDULE_OTHER;
		if (e->haveData)
			e->finishEPG();
	}
}

#ifdef ENABLE_FREESAT_EPG
inline void eScheduleFreesat::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & eEPGCache::SCHEDULE_FREESAT) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		eDebug("[EPGC] stop schedule freesat");
		e->isRunning &= ~eEPGCache::SCHEDULE_FREESAT;
		if (e->haveData)
			e->finishEPG();
	}
}
#endif

inline void eNowNext::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if ( (e->isRunning & eEPGCache::NOWNEXT) && (err == -ETIMEDOUT || err == -ECANCELED ) )
	{
		eDebug("[EPGC] stop nownext");
		e->isRunning &= ~eEPGCache::NOWNEXT;
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
