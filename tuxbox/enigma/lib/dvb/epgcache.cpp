#include "epgcache.h"

#include <time.h>
#include <core/system/init.h>
#include <core/dvb/lowlevel/dvb.h>
#include <core/dvb/si.h>

int eventData::CacheSize=0;

eEPGCache *eEPGCache::instance;

#define HILO(x) (x##_hi << 8 | x##_lo)
eEPGCache::eEPGCache():
			eSection(0x12, 0x50, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xF0),
			CleanTimer(eApp), zapTimer(eApp)
//eEPGCache::eEPGCache():eSection(0x12, 0x40, -1, -1, SECREAD_CRC|SECREAD_NOTIMEOUT, 0xC0)
{
	eDebug("[EPGC] Initialized EPGCache");
	isRunning=0;

	CONNECT(eDVB::getInstance()->switchedService, eEPGCache::enterService);
	CONNECT(eDVB::getInstance()->leaveService, eEPGCache::stopEPG);
	CONNECT(eDVB::getInstance()->timeUpdated, eEPGCache::timeUpdated);
	CONNECT(zapTimer.timeout, eEPGCache::startEPG);
	CONNECT(CleanTimer.timeout, eEPGCache::cleanLoop);
	instance=this;
}

void eEPGCache::timeUpdated()
{
	CleanTimer.start(CLEAN_INTERVAL);
}

int eEPGCache::sectionRead(__u8 *data)
{
/*		if (*data >= 0x70 || *data < 0x50)
			return 0;

		eventData::TYP type = (*data < 0x60)?eventData::FULL:eventData::SHORT;*/

		eit_t *eit = (eit_t*) data;
		eServiceID service_id(HILO(eit->service_id));
		eOriginalNetworkID original_network_id(HILO(eit->original_network_id));
		int len=HILO(eit->section_length)-1;//+3-4;
		int ptr=EIT_SIZE;
		eit_event_struct* eit_event = (eit_event_struct*) (data+ptr);
		int eit_event_size;
		int duration;
		eServiceReference service(eServiceReference::idDVB, -1, original_network_id, service_id, -1);
		time_t TM;
		updateMap::iterator It;
		
/*		if (type == eventData::FULL)  // only sections with full descr update in 60 min	
		{*/
			It = temp.find(service);
	
			if (It == temp.end())
			  temp[service] = time(0)+eDVB::getInstance()->time_difference;
//		}

		if (firstEventId == HILO( eit_event->event_id ) )  // EPGCache around....
		{
			stopEPG(*(eServiceReference*)0);
			zapTimer.start(UPDATE_INTERVAL, 1);
			eDebug("[EPGC] next update in %i min", UPDATE_INTERVAL / 60000);
			
			It = temp.begin();

			while (It != temp.end())
				serviceLastUpdated.insert(*(It++));

			if (!eventDB[current_service].empty())
		  	/*emit*/ EPGAvail(1);

			return -1;
		}
		else if (!firstEventId)
			firstEventId = HILO( eit_event->event_id );

		while (ptr<len)
		{
			eit_event_size = HILO(eit_event->descriptors_loop_length)+EIT_LOOP_SIZE;

			duration = fromBCD(eit_event->duration_1)*3600+fromBCD(eit_event->duration_2)*60+fromBCD(eit_event->duration_3);
			TM = parseDVBtime( eit_event->start_time_1, eit_event->start_time_2,	eit_event->start_time_3, eit_event->start_time_4,	eit_event->start_time_5);

			if ( (time(0)+eDVB::getInstance()->time_difference) <= (TM+duration))  // old events should not be cached
			{
				// hier wird immer eine eventMap zurück gegeben.. entweder eine vorhandene..
				// oder eine durch [] erzeugte
				eventMap &servicemap = eventDB[service];
				
				eventMap::iterator It = servicemap.find(TM);

				if (It == servicemap.end())   // event still not cached
					eventDB[service][TM]=new eventData(eit_event, eit_event_size/*, type*/);
/*				else
					if (type == eventData::FULL && It->second->type == eventData::SHORT)
					{   // old cached SHORT event should now updated to FULL Event
						delete It->second;
						It->second = new eventData(eit_event, eit_event_size, type);
					}*/
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
		eDebug("[EPGC] start cleanloop");
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
					eDebug("[EPGC] delete old event");
					delete It->second;				// release Heap Memory for this entry   (new ....)
					DBIt->second.erase(It);   // remove entry from map
					It=DBIt->second.begin();  // start at begin
				}
				else  // valid entry in map
					It=DBIt->second.end();  // ends this clean loop
			}

		eDebug("[EPGC] stop cleanloop");
		eDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
	}
}

eEPGCache::~eEPGCache()
{
	for (eventCache::iterator evIt = eventDB.begin(); evIt != eventDB.end(); evIt++)
		for (eventMap::iterator It = evIt->second.begin(); It != evIt->second.end(); It++)
			delete It->second;
}

/*EITEvent *eEPGCache::lookupEvent(const eServiceReference &service, int event_id)
{
	eventMap &service=eventDB[service];
	eventMap::iterator event=service.find(event_id);
	if (event==service.end())
		return 0;
	return new EITEvent(*event->second);
} */

EITEvent *eEPGCache::lookupCurrentEvent(const eServiceReference &service)
{
	eventCache::iterator It =	eventDB.find(service);
	if (It != eventDB.end())
		return It->second.empty()? 0 : new EITEvent ( *It->second.begin()->second );
	else
		return 0;
}

eAutoInitP0<eEPGCache> init_eEPGCacheInit(5, "EPG cache");

