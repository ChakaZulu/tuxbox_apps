#ifndef __epgcache_h_
#define __epgcache_h_

#include <vector>
#include <ext/hash_map>

#include "si.h"
#include "dvb.h"

#define eventData std::vector<__u8>
#define eventMap std::map<int, eventData>
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
	static eEPGCache *getInstance() { return instance; }
	EITEvent *lookupEvent(int original_network_id, int service_id, int event_id);
	EITEvent *lookupCurrentEvent(int original_network_id, int service_id);
	const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id);
};

#endif
