#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <list>
#include <ext/hash_map>

#include "si.h"
#include "dvb.h"
#include "edvb.h"

#define CLEAN_INTERVAL 60000    //  1 min
#define UPDATE_INTERVAL 3600000  // 60 min
#define ZAP_DELAY 4000          // 4 sek

class eventData;

typedef std::map<int, eventData*> eventMap;
typedef std::hash_map<sref, eventMap > eventCache;
typedef std::hash_map<sref, time_t > updateMap;

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
	static int CacheSize;
	eventData(const eit_event_struct* e, int size)
	:ByteSize(size)
	{
		CacheSize+=size;
		EITdata = new char[size];
		memcpy(EITdata, (char*) e, size);
	}
	eventData(const eventData& Q)
	:ByteSize(Q.ByteSize)
	{
		CacheSize+=ByteSize;
		EITdata = new char[ByteSize];
		memcpy(EITdata, Q.EITdata, ByteSize);
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
	Q_OBJECT
private:
	eService* current_service;
	int current_sid;
	int firstEventId;
	int isRunning;
	int sectionRead(__u8 *data);
	static eEPGCache *instance;

	eventCache eventDB;
	updateMap serviceLastUpdated;
	updateMap temp;

	QTimer CleanTimer;
	QTimer zapTimer;
	QTimer EPGUpdate;
public slots:
	inline void startEPG();
	inline void stopEPG();
	inline void enterService(eService*, int);
	void cleanLoop();
	void timeUpdated();
public:
	eEPGCache();
	~eEPGCache();
	static eEPGCache *getInstance() { return instance; }
//	EITEvent *lookupEvent(int original_network_id, int service_id, int event_id);
	EITEvent *lookupCurrentEvent(int original_network_id, int service_id);
	inline const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id);
signals:
	void EPGAvail(bool);
};

inline void eEPGCache::enterService(eService* service, int err)
{
	current_service = service;
	firstEventId = 0;
	time_t t = serviceLastUpdated[sref(service->original_network_id,service->service_id)];

	if (!err)
	{
		int update = ( t ? ( UPDATE_INTERVAL - ( (time(0)+eDVB::getInstance()->time_difference-t) * 1000 ) ) : ZAP_DELAY );
		zapTimer.start(update, 1);
		if (update >= 60000)
			qDebug("[EPGC] next update in %i min", update/60000);
		else if (update >= 1000)
			qDebug("[EPGC] next update in %i sec", update/1000);
	}

	if (t)
	{
		qDebug("[EPGC] service has EPG");
		emit EPGAvail(1);
	}
	else
	{
		qDebug("[EPGC] service has no EPG");
		emit EPGAvail(0);
	}
}

inline void eEPGCache::startEPG()
{
	if (eDVB::getInstance()->time_difference)	
	{
		temp.clear();
		qDebug("[EPGC] start caching events");
		firstEventId=0;
		start();
		isRunning=1;
	}
	else
	{
		qDebug("[EPGC] wait for clock update");
		zapTimer.start(1000, 1); // restart Timer
	}
}

inline void eEPGCache::stopEPG()
{
	if (isRunning)
	{
		qDebug("[EPGC] stop caching events");
		isRunning=0;
		timeout();
		zapTimer.stop();
	}
}

inline const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id)
{
	return eventDB[sref(original_network_id,service_id)];
}

#endif
