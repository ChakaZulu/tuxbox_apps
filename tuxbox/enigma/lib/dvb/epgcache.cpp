#include <time.h>
#include "epgcache.h"
#include "init.h"
#include "lowlevel/dvb.h"
#include "si.h"

int eventData::CacheSize=0;

eEPGCache *eEPGCache::instance;

#define HILO(x) (x##_hi << 8 | x##_lo)

eEPGCache::eEPGCache():eSection(0x12, 0x50, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0)
{
	qDebug("[EPGC] Initialized EPGCache");
	isRunning=0;
	connect(eDVB::getInstance(), SIGNAL(switchedService(eService*, int)), SLOT(enterService(eService*, int)));
	connect(eDVB::getInstance(), SIGNAL(leaveService(eService*)), SLOT(stopEPG()));
	connect(eDVB::getInstance(), SIGNAL(timeUpdated()), SLOT(timeUpdated()));
	connect(&zapTimer, SIGNAL(timeout()), SLOT(startEPG()));
	connect(&CleanTimer, SIGNAL(timeout()), SLOT(cleanLoop()));	
	instance=this;
}

void eEPGCache::timeUpdated()
{
	CleanTimer.start(CLEAN_INTERVAL);
}

int eEPGCache::sectionRead(__u8 *data)
{
		eit_t *eit = (eit_t*) data;
		int service_id=HILO(eit->service_id);
		int original_network_id=HILO(eit->original_network_id);
		int len=HILO(eit->section_length)-1;//+3-4;
		int ptr=EIT_SIZE;
		eit_event_struct* eit_event = (eit_event_struct*) (data+ptr);
		int eit_event_size;
		int duration;
		sref SREF = sref(original_network_id,service_id);
		time_t TM;

		if (temp[SREF] <= 0)
		  temp[SREF] = time(0)+eDVB::getInstance()->time_difference;

		if (firstEventId == HILO( eit_event->event_id ) )  // EPGCache around....
		{
			stopEPG();
			zapTimer.start(UPDATE_INTERVAL, 1);
			qDebug("[EPGC] next update in %i min", UPDATE_INTERVAL / 60000);
			
			updateMap::iterator It = temp.begin();

			while (It != temp.end())
				serviceLastUpdated.insert(*(It++));

			// diese SREF verdeckt hier die obige.. deshalb klapp das
			sref SREF=sref(current_service->original_network_id, current_service->service_id);

			if (temp[SREF])
			  serviceLastUpdated[SREF] = time(0)+eDVB::getInstance()->time_difference;
				// ich verstehe nicht, warum ich diesen einen Eintrag in serviceLastUpdated manuell machen muss
				// eigentlich müsste dieser Eintrage mit in obiger while schleife erzeugt werden.
				// Komischerweise wird er aber nicht in der while schleife erzeugt......

			if (!eventDB[SREF].empty())
		  	emit EPGAvail(1);

			return -1;
		}
		else
			if (!firstEventId)
					firstEventId = HILO( eit_event->event_id );

		while (ptr<len)
		{
			eit_event_size = HILO(eit_event->descriptors_loop_length)+EIT_LOOP_SIZE;

			duration = fromBCD(eit_event->duration_1)*3600+fromBCD(eit_event->duration_2)*60+fromBCD(eit_event->duration_3);
			TM = parseDVBtime( eit_event->start_time_1, eit_event->start_time_2,	eit_event->start_time_3, eit_event->start_time_4,	eit_event->start_time_5);

			if ( (time(0)+eDVB::getInstance()->time_difference) <= (TM+duration))  // old events should not be cached
			{
				eventMap &service = eventDB[SREF];

				if (service.find(TM) == service.end())   // event still not cached
					eventDB[SREF][TM]=new eventData(eit_event, eit_event_size);
			}
			ptr += eit_event_size;
			((__u8*)eit_event)+=eit_event_size;
		}
		return 0;
}

void eEPGCache::cleanLoop()
{
	if (!eventDB.empty())
	{
		qDebug("[EPGC] start cleanloop");
		const eit_event_struct* cur_event;
		int duration;
		time_t TM;

		for (eventCache::iterator DBIt = eventDB.begin(); DBIt != eventDB.end(); DBIt++)
			for (eventMap::iterator It = DBIt->second.begin(); It != DBIt->second.end();)
			{
				cur_event = (*It->second).get();
		
				duration = fromBCD( cur_event->duration_1)*3600 + fromBCD(cur_event->duration_2)*60 + fromBCD(cur_event->duration_3);
				TM = parseDVBtime( cur_event->start_time_1, cur_event->start_time_2,cur_event->start_time_3,cur_event->start_time_4,cur_event->start_time_5);

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

		qDebug("[EPGC] stop cleanloop");
		qDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
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

eAutoInitP0<eEPGCache> init_eEPGCacheInit(5, "EPG cache");

