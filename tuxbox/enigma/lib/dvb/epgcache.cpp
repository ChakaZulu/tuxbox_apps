#include <time.h>
#include "epgcache.h"

#define HILO(x) (x##_hi << 8 | x##_lo) 
#include "si/dvb.h"

eCachedEvent::eCachedEvent(int service_id, int transport_stream_id, int original_network_id, eit_event_struct *event)
	: EITEvent(event), service_id(service_id), transport_stream_id(transport_stream_id), original_network_id(original_network_id)
{
	qDebug("[EPGC] created new event");
	time(&timestamp);
}

eEPGCache::eEPGCache(): eSection(0x12, 0x40, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xE0)
{
	qDebug("[EPGC] Initialized EPGCache");
	events.setAutoDelete(true);
//	start();
}

int eEPGCache::sectionRead(__u8 *data)
{
	qDebug("[EPGC] got EGP section, tableid: %02x", data[0]);

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
		eCachedEvent *ce=new eCachedEvent(service_id, transport_stream_id, original_network_id, (eit_event_struct*)(data+ptr));
		for (events.first(); events.current();)
		{
			if ((events.current()->original_network_id == original_network_id) && 
			    (events.current()->service_id==service_id) &&
			    (events.current()->event_id==ce->event_id))
			{
				qDebug("remove old %04x:%04x:%04x\n", original_network_id, service_id, ce->event_id);
				events.remove();
			} else
				events.next();
		}
		
		ptr+=HILO(((eit_event_struct*)(data+ptr))->descriptors_loop_length)+EIT_LOOP_SIZE;
	}
	return 0;
}
