#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <ext/hash_map>

#include "si.h"
#include "dvb.h"

#define eventMap std::map<int, eventData*>
#define eventCache std::hash_map<sref, eventMap >

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
	eventData(const eit_event_struct* e, int size)
	{
		ByteSize=size;
		EITdata = new char[size];
		memcpy(EITdata, (char*) e, size);
	}
	eventData(const eventData& Q)
	:ByteSize(Q.ByteSize)
	{
		EITdata = new char[ByteSize];
		memcpy(EITdata, Q.EITdata, ByteSize);
	}
	~eventData()
	{
		delete [] EITdata;
	}	
	operator const eit_event_struct*() const
	{
		return (const eit_event_struct*) EITdata;
	}
};

class eEPGCache: public eSection
{
	Q_OBJECT
private:
	int sectionRead(__u8 *data);
	static eEPGCache *instance;
	eventCache eventDB;
public slots:
	void enterTransponder();
	void leaveTransponder();
public:
	eEPGCache();
	~eEPGCache();
	static eEPGCache *getInstance() { return instance; }
	EITEvent *lookupEvent(int original_network_id, int service_id, int event_id);
	EITEvent *lookupCurrentEvent(int original_network_id, int service_id);
	const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id);
};

#endif
