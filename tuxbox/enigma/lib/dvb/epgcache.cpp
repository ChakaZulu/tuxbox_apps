#include <time.h>
#include "epgcache.h"
#include "init.h"
#include "edvb.h"

eEPGCache *eEPGCache::instance;

#define HILO(x) (x##_hi << 8 | x##_lo)
#include "lowlevel/dvb.h"


//eEPGCache::eEPGCache():eSection(0x12, 0x40, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xE0)
eEPGCache::eEPGCache():eSection(0x12, 0x50, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0)
{
	qDebug("[EPGC] Initialized EPGCache");
	connect(eDVB::getInstance(), SIGNAL(enterTransponder(eTransponder*)), SLOT(enterTransponder()));
	connect(eDVB::getInstance(), SIGNAL(leaveTransponder(eTransponder*)), SLOT(leaveTransponder()));
	instance=this;
}

int eEPGCache::sectionRead(__u8 *data)
{
//	qDebug("[EPGC] got EGP section, tableid: %02x", data[0]);

	eit_t *eit=(eit_t*)data;
	int service_id=HILO(eit->service_id);
//	int version_number=eit->version_number;
//	int current_next_indicator=eit->current_next_indicator;
	int transport_stream_id=HILO(eit->transport_stream_id);
	int original_network_id=HILO(eit->original_network_id);
	int len=HILO(eit->section_length)+3-4;
	int ptr=EIT_SIZE;
	while (ptr<len)
	{
		eit_event_struct *eit_event=(eit_event_struct*)(data+ptr);
		int event_id=HILO(eit_event->event_id);

		eventMap &service=eventDB[sref(original_network_id,service_id)];
		eventMap::iterator event=service.find(event_id);
//		qDebug("Incoming Event: %x:%x:%x", original_network_id, service_id, event_id);
		if (event==service.end())
		{
			eventData ed(
				eventData::iterator((__u8*)(data+ptr)), 
				eventData::iterator((__u8*)(data+ptr+HILO(((eit_event_struct*)(data+ptr))->descriptors_loop_length)+EIT_LOOP_SIZE))
			);
			eventDB[sref(original_network_id,service_id)].insert(std::pair<int,eventData>(event_id, ed));
			qDebug("hey, new: %x:%x:%x (on %x)", original_network_id, service_id, event_id, *data);
		}
		ptr+=HILO(((eit_event_struct*)(data+ptr))->descriptors_loop_length)+EIT_LOOP_SIZE;
	}
	return 0;
}

void eEPGCache::enterTransponder()
{
	start();
}

void eEPGCache::leaveTransponder()
{
	abort();
}

EITEvent *eEPGCache::lookupEvent(int original_network_id, int service_id, int event_id)
{
	eventMap &service=eventDB[sref(original_network_id,service_id)];
	eventMap::iterator event=service.find(event_id);
	if (event==service.end())
		return 0;
	return new EITEvent((const eit_event_struct*)&(*event->second.begin()));
}

EITEvent *eEPGCache::lookupCurrentEvent(int original_network_id, int service_id)
{
	eventMap &service=eventDB[sref(original_network_id,service_id)];
	eventMap::iterator event=service.begin();
	if (event==service.end())
		return 0;
	return new EITEvent((const eit_event_struct*)&(*event->second.begin()));
}

const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id)
{
	return eventDB[sref(original_network_id,service_id)];
}


eAutoInitP0<eEPGCache> init_eEPGCacheInit(6, "EPG cache");

