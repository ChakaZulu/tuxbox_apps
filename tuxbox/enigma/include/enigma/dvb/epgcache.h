#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <ext/hash_map>

#include "si.h"
#include "dvb.h"
#include "edvb.h"

class eventData;

typedef std::map<int, eventData*> eventMap;
typedef std::hash_map<sref, eventMap > eventCache;

namespace std
{
struct hash<sref>
{
	size_t operator()(const sref &x) const
	{
		int v=(x.first^x.second);
		v^=v>>8;
		return v&0xFF;
	}
};
}

class eventData
{
	char* EITdata;
	int ByteSize;
public:
//	static int refcount;
	eventData(const eit_event_struct* e, int size)
	:ByteSize(size)
	{
//		refcount++;
		EITdata = new char[size];
		memcpy(EITdata, (char*) e, size);
	}
	eventData(const eventData& Q)
	:ByteSize(Q.ByteSize)
	{
//		refcount++;
		EITdata = new char[ByteSize];
		memcpy(EITdata, Q.EITdata, ByteSize);
	}
	~eventData()
	{
/*		qDebug("[EPGD] %i event(s) cached", refcount);
		refcount--;*/
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
	Q_OBJECT
private:
  eService* current_service;
	int current_sid;
	int isRunning;
	int sectionRead(__u8 *data);
	static eEPGCache *instance;
	eventCache eventDB;
	QTimer IdleTimer;
	QTimer zapTimer;
public slots:
	inline void startEPG();
	inline void stopEPG();
	inline void enterTransponder();
	inline void enterService();
	void cleanLoop();
public:
	eEPGCache();
	~eEPGCache();
	static eEPGCache *getInstance() { return instance; }
//	EITEvent *lookupEvent(int original_network_id, int service_id, int event_id);
	EITEvent *lookupCurrentEvent(int original_network_id, int service_id);
	inline const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id);
};

inline void eEPGCache::enterService()
{
	zapTimer.start(3000, 1);
}

inline void eEPGCache::enterTransponder()
{
	if (!zapTimer.isActive())
		zapTimer.start(3000, 1);
}

inline void eEPGCache::startEPG()
{
	if (eDVB::getInstance()->time_difference)	
	{
		qDebug("[EPGC] start caching events");
		start();
		isRunning=1;
	}
	else
		zapTimer.start(1000, 1); // restart Timer
}

inline void eEPGCache::stopEPG()
{
	if (isRunning)
	{
		qDebug("[EPGC] stop caching events");
		isRunning=0;
		abort();
		zapTimer.stop();
	}
}

inline const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id)
{
	return eventDB[sref(original_network_id,service_id)];
}


#endif
