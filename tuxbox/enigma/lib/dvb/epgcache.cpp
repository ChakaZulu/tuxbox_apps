#include <time.h>
#include "epgcache.h"
#include "init.h"
#include "edvb.h"
#include "lowlevel/dvb.h"
#include "si.h"

//int eventData::refcount=0;

eEPGCache *eEPGCache::instance;

#define HILO(x) (x##_hi << 8 | x##_lo)

//eEPGCache::eEPGCache():eSection(0x12, 0x40, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xE0)
eEPGCache::eEPGCache():eSection(0x12, 0x50, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0)
{
	qDebug("[EPGC] Initialized EPGCache");
	zapTimer.stop();
	IdleTimer.start(0);
	connect(eDVB::getInstance(), SIGNAL(enterTransponder(eTransponder*)), SLOT(enterTransponder()));
	connect(eDVB::getInstance(), SIGNAL(leaveTransponder(eTransponder*)), SLOT(leaveTransponder()));
	connect(eDVB::getInstance(), SIGNAL(timeUpdated()), SLOT(timeUpdated()));	
	connect(&IdleTimer, SIGNAL(timeout()), SLOT(cleanLoop()));	
	connect(&zapTimer, SIGNAL(timeout()), SLOT(ZapDelay()));
	instance=this;
}

int eEPGCache::sectionRead(__u8 *data)
{
//	qDebug("[EPGC] got EGP section, tableid: %02x", data[0]);
	eit_t *eit = (eit_t*) data;
	int service_id=HILO(eit->service_id);
//	int version_number=eit->version_number;
//	int current_next_indicator=eit->current_next_indicator;
	int transport_stream_id=HILO(eit->transport_stream_id);
	int original_network_id=HILO(eit->original_network_id);
	int len=HILO(eit->section_length)-1;//+3-4;
	int ptr=EIT_SIZE;
	while (ptr<len)
	{
		eit_event_struct* eit_event = (eit_event_struct*) (data+ptr);
		int eit_event_size=HILO(eit_event->descriptors_loop_length)+EIT_LOOP_SIZE;

		int duration=	fromBCD(eit_event->duration_1)*3600+
									fromBCD(eit_event->duration_2)*60+
									fromBCD(eit_event->duration_3);

		time_t TM = parseDVBtime( eit_event->start_time_1,
															eit_event->start_time_2,
															eit_event->start_time_3,
															eit_event->start_time_4,
															eit_event->start_time_5);

		if ( (time(0)+eDVB::getInstance()->time_difference) <= (TM+duration))  // old events should not be cached
		{
//			int event_id=HILO(eit_event->event_id);

			eventMap &service = eventDB[sref(original_network_id,service_id)];
			eventMap::iterator event=service.find(TM);

			if (event==service.end())   // event still not cached
				eventDB[sref(original_network_id,service_id)].insert(std::pair<int,eventData*>(TM, new eventData(eit_event, eit_event_size)));
		}

		ptr+=eit_event_size;
	}
	return 0;
}

void eEPGCache::timeUpdated()
{
	qDebug("[EPGC] time is now valid... start caching events");
	start();
}

void eEPGCache::enterTransponder()
{
// Set Two Seconds Delay for fast Zapping
	if (zapTimer.isActive())
		zapTimer.stop();
	
	zapTimer.start(2000, true);
}

void eEPGCache::leaveTransponder()
{
	if (eDVB::getInstance()->time_difference)
		abort();
}

void eEPGCache::ZapDelay()
{
	if (eDVB::getInstance()->time_difference)	
	{
		qDebug("[EPGC] zap delay is over... start caching events");
		start();
	}
}

void eEPGCache::cleanLoop()
{
	if ( !eventDB.empty() )   // Elements in EPGCache ?
	{
		static eventCache::iterator DBIt = eventDB.begin();  // change to first onid/sid map
		static eventMap::iterator It=DBIt->second.begin();  // start on first onid/sid

		if (It == DBIt->second.end())    // end of this map
		{
			if (++DBIt == eventDB.end())   // change to next map... was last map ?
				DBIt = eventDB.begin();			 // change to first onid/sid map
		
			It = DBIt->second.begin();     // start on first entry
		}
	
		while (It != DBIt->second.end())
		{
			const eit_event_struct* cur_event = (*It->second).get();

			int duration=	fromBCD(cur_event->duration_1)*3600+
										fromBCD(cur_event->duration_2)*60+
										fromBCD(cur_event->duration_3);
 	
			time_t TM = parseDVBtime( cur_event->start_time_1,
																cur_event->start_time_2,
																cur_event->start_time_3,
																cur_event->start_time_4,
																cur_event->start_time_5);

			if ( (time(0)+eDVB::getInstance()->time_difference) > (TM+duration))  // outdated entry ?
			{
				qDebug("[EPGC] delete old event");
				delete It->second;				// release Heap Memory for this entry   (new ....)
				DBIt->second.erase(It);   // remove entry from map
				It=DBIt->second.begin();  // start at begin
			}
			else  // valid entry in map
				It=DBIt->second.end();  // ends this clean loop
		}
	}
}

eEPGCache::~eEPGCache()
{
	eventCache::iterator DBIt = eventDB.begin();
	eventMap::iterator It;
	for (;DBIt != eventDB.end(); DBIt++)
	{
		It = DBIt->second.begin();
		for (;It != DBIt->second.end(); It++)
			delete It->second;
	}
}

/*EITEvent *eEPGCache::lookupEvent(int original_network_id, int service_id, int event_id)
{
	eventMap &service=eventDB[sref(original_network_id,service_id)];
	eventMap::iterator event=service.find(event_id);
	if (event==service.end())
		return 0;
	return new EITEvent(*event->second);
} */

EITEvent *eEPGCache::lookupCurrentEvent(int original_network_id, int service_id)
{
	eventMap &service=eventDB[sref(original_network_id,service_id)];
	eventMap::iterator event=service.begin();
	if (event==service.end())
		return 0;
	return new EITEvent(*event->second);
}

const eventMap& eEPGCache::getEventMap(int original_network_id, int service_id)
{
	return eventDB[sref(original_network_id,service_id)];
}

eAutoInitP0<eEPGCache> init_eEPGCacheInit(6, "EPG cache");

