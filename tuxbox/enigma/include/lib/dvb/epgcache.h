#ifndef __epgcache_h_
#define __epgcache_h_

#include "si.h"

class eCachedEvent: public EITEvent
{
public:
	eCachedEvent(int service_id, int transport_stream_id, int original_network_id, eit_event_struct *event);
	int service_id, transport_stream_id, original_network_id;
	time_t timestamp;		// acquired at ...
};

class eEPGCache: public eSection
{
	Q_OBJECT
	int sectionRead(__u8 *data);
public:
	eEPGCache();

	QList<eCachedEvent> events;
};

#endif
