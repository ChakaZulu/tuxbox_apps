#include <lib/dvb/epgcache.h>

#include <time.h>
#include <unistd.h>  // for usleep
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/dvb/lowlevel/dvb.h>
#include <lib/dvb/si.h>
#include <lib/dvb/service.h>
#include <lib/dvb/dvbservice.h>

int eventData::CacheSize=0;

eEPGCache *eEPGCache::instance;

eEPGCache::eEPGCache()
	:paused(0), firstStart(1), CleanTimer(eApp), zapTimer(eApp)
{
	eDebug("[EPGC] Initialized EPGCache");
	isRunning=0;

	CONNECT(eDVB::getInstance()->switchedService, eEPGCache::enterService);
	CONNECT(eDVB::getInstance()->leaveService, eEPGCache::abortEPG);
	CONNECT(eDVB::getInstance()->timeUpdated, eEPGCache::timeUpdated);
	CONNECT(zapTimer.timeout, eEPGCache::startEPG);
	CONNECT(CleanTimer.timeout, eEPGCache::cleanLoop);
	instance=this;
}

void eEPGCache::timeUpdated()
{
	CleanTimer.start(CLEAN_INTERVAL);
}

int eEPGCache::sectionRead(__u8 *data, int source)
{
	eit_t *eit = (eit_t*) data;
	uniqueEPGKey service( HILO(eit->service_id), HILO(eit->original_network_id), current_service.opos );
	int len=HILO(eit->section_length)-1;//+3-4;
	int ptr=EIT_SIZE;
	eit_event_struct* eit_event = (eit_event_struct*) (data+ptr);
	int eit_event_size;
	int duration;

	time_t TM = parseDVBtime( eit_event->start_time_1, eit_event->start_time_2,	eit_event->start_time_3, eit_event->start_time_4, eit_event->start_time_5);
	time_t now = time(0)+eDVB::getInstance()->time_difference;

	tmpMap::iterator it = temp.find( service );
	if ( it == temp.end() ||
				( source == SCHEDULE &&
					it->second.second == NOWNEXT )
			)
			temp[service] = std::pair< time_t, int> (now, source);

	if ( TM > now-24*60*60 && TM < now+14*24*60*60 )
	{
		uniqueEvent event( TM, HILO(eit_event->event_id), service );
		if ( source == SCHEDULE )
		{
			if ( !firstScheduleEvent.valid() )
			{
				eDebug("[EPGC] schedule data begin");
				firstScheduleEvent=event;
			}
			else if ( firstScheduleEvent == event )  // epg around
			{
				eDebug("[EPGC] schedule data ready");
				return -ECANCELED;
			}
		}
		else // if ( source == NOWNEXT )
		{
			if ( !firstNowNextEvent.valid() )
			{
				eDebug("[EPGC] nownext data begin");
				firstNowNextEvent = event;
			}
			else if ( firstNowNextEvent == event ) // now next ready
			{
				eDebug("[EPGC] nownext data ready");
				return -ECANCELED;
			}
		}
	}

	while (ptr<len)
	{
		eit_event_size = HILO(eit_event->descriptors_loop_length)+EIT_LOOP_SIZE;

		duration = fromBCD(eit_event->duration_1)*3600+fromBCD(eit_event->duration_2)*60+fromBCD(eit_event->duration_3);
		TM = parseDVBtime(
			eit_event->start_time_1,
			eit_event->start_time_2,
			eit_event->start_time_3,
			eit_event->start_time_4,
			eit_event->start_time_5);

		if ( (TM+duration < now || TM > now+14*24*60*60) && TM != 3599 )
			goto next;

		if ( now <= (TM+duration) || TM == 3599 /*NVOD Service*/ )  // old events should not be cached
		{
			// hier wird immer eine eventMap zurück gegeben.. entweder eine vorhandene..
			// oder eine durch [] erzeugte
			std::pair<eventMap,timeMap> &servicemap = eventDB[service];

			// look in 1st descriptor tag.. i hope all time shifted events have the
			// time shifted event descriptor at the begin of the descriptors..
			if ( ((unsigned char*)eit_event)[12] == 0x4F ) // TIME_SHIFTED_EVENT_DESCR
			{
				// get Service ID of NVOD Service ( parent )
				int sid = ((unsigned char*)eit_event)[14] << 8 | ((unsigned char*)eit_event)[15];
				uniqueEPGKey parent( sid, HILO(eit->original_network_id), current_service.opos );
				// check that this nvod reference currently is not in the NVOD List
				std::list<NVODReferenceEntry>::iterator it( NVOD[parent].begin() );
				for ( ; it != NVOD[parent].end(); it++ )
					if ( it->service_id == HILO(eit->service_id) && it->original_network_id == HILO( eit->original_network_id ) )
						break;
				if ( it == NVOD[parent].end() )  // not in list ?
					NVOD[parent].push_back( NVODReferenceEntry( HILO(eit->transport_stream_id), HILO(eit->original_network_id), HILO(eit->service_id) ) );
			}

// delete first the old event...
			__u16 event_id = HILO(eit_event->event_id);
			eventMap::iterator it = servicemap.first.find(event_id);
			if ( it != servicemap.first.end() )
			{
				servicemap.second.erase(it->second->getStartTime());
				delete it->second;
			}

			eventData *evt = new eventData(eit_event, eit_event_size, source);
// add new...
			servicemap.first[event_id] = evt;
			servicemap.second[TM] = evt;
		}
next:
		ptr += eit_event_size;
		((__u8*)eit_event)+=eit_event_size;
	}
	return 0;
}

bool eEPGCache::finishEPG()
{
	if (!isRunning)  // epg ready
	{
		eDebug("[EPGC] stop caching events");
		zapTimer.start(UPDATE_INTERVAL, 1);
		eDebug("[EPGC] next update in %i min", UPDATE_INTERVAL / 60000);

		tmpMap::iterator It = temp.begin();

//		eDebug("tempmap size=%d", temp.size() );
		while (It != temp.end())
		{
//		eDebug("sid = %02x, onid = %02x, opos = %d", It->first.sid, It->first.onid, It->first.opos);
			if ( It->second.second == SCHEDULE || ( It->second.second == NOWNEXT && !firstScheduleEvent.valid() ) )
			{
//				eDebug("ADD to last updated Map");
				serviceLastUpdated[It->first]=It->second.first;
			}
			if ( eventDB.find( It->first ) == eventDB.end() )
			{
//				eDebug("REMOVE from update Map");
				temp.erase(It++->first);
			}
			else
				It++;
		}
		if (!eventDB[current_service].first.empty())
			/*emit*/ EPGAvail(1);

		/*emit*/ EPGUpdated( &temp );

		return true;
	}
	return false;
}

void eEPGCache::flushEPG(const eServiceReferenceDVB& s)
{
	if (s)  // clear only this service
	{
		eventCache::iterator it = eventDB.find(s);
		if ( it != eventDB.end() )
		{
			eventMap &evMap = it->second.first;
			timeMap &tmMap = it->second.second;
			tmMap.clear();
			for (eventMap::iterator i = evMap.begin(); i != evMap.end(); ++i)
				delete i->second;
			evMap.clear();
			eventDB.erase(it);
			updateMap::iterator u = serviceLastUpdated.find(s);
			if ( u != serviceLastUpdated.end() )
				serviceLastUpdated.erase(u);
			startEPG();
		}
		return;
	}
	else // clear complete EPG Cache
	{
		for (eventCache::iterator it(eventDB.begin());
			it != eventDB.end(); ++it)
		{
			eventMap &evMap = it->second.first;
			timeMap &tmMap = it->second.second;
			for (eventMap::iterator i = evMap.begin(); i != evMap.end(); ++i)
				delete i->second;
			evMap.clear();
			tmMap.clear();
			serviceLastUpdated.clear();
			startEPG();
		}
		eventDB.clear();
	}
}

void eEPGCache::cleanLoop()
{
	if (!eventDB.empty() && !paused )
	{
		eDebug("[EPGC] start cleanloop");
		const eit_event_struct* cur_event;
		int duration;

		time_t TM,
					 now = time(0)+eDVB::getInstance()->time_difference;

		tmpMap temp;

		for (eventCache::iterator DBIt = eventDB.begin(); DBIt != eventDB.end(); DBIt++)
		{
			for (timeMap::iterator It = DBIt->second.second.begin(); It != DBIt->second.second.end();)
			{
				cur_event = (*It->second).get();

				duration = fromBCD( cur_event->duration_1)*3600 + fromBCD(cur_event->duration_2)*60 + fromBCD(cur_event->duration_3);
				TM = (*It->second).getStartTime();

				if ( TM == 3599 )  // check if NVOD Entry valid..
				{ 
					for ( std::list<NVODReferenceEntry>::iterator it( NVOD[DBIt->first].begin() ); it != NVOD[DBIt->first].end(); it++ )
					{
						eventCache::iterator evIt = eventDB.find(uniqueEPGKey( it->service_id, it->original_network_id, current_service.opos ));
						if ( evIt != eventDB.end() )
						{
							for ( eventMap::iterator emIt( evIt->second.first.begin() ); emIt != evIt->second.first.end(); emIt++)
							{
								EITEvent refEvt(*emIt->second);
								for (ePtrList<Descriptor>::iterator d(refEvt.descriptor); d != refEvt.descriptor.end(); ++d)
								{
									Descriptor *descriptor=*d;
									if (descriptor->Tag()==DESCR_TIME_SHIFTED_EVENT)
									{
										if ( ((TimeShiftedEventDescriptor*)descriptor)->reference_event_id == HILO(cur_event->event_id))
										{
											// event is valid...
											// we must check all other NVOD Events too
											++It;
											eDebug("hold valid NVOD Entry");
											goto nextEvent;
										}
									}
								}
							}
						}
					}
					eDebug("delete no more used NVOD Entry");
					goto removeEntry;
				}
				else if ( now > (TM+duration) )  // outdated normal entry (nvod references to)
				{
removeEntry:
					eDebug("[EPGC] delete old event");
					// remove entry from eventMap
					DBIt->second.first.erase(It->second->getEventID());
					// release Heap Memory for this entry   (new ....)
					delete It->second;
					// remove entry from timeMap
					DBIt->second.second.erase(TM);
					// add this (changed) service to temp map...
					temp[DBIt->first]=std::pair<time_t, int>(now, NOWNEXT);
					It=DBIt->second.second.begin();  // start at begin
				}
				else  // valid entry found
							// we must not check any other event in this map
							// beginTime is sort key...
					break;
nextEvent:
				;
			}
		}

		if (temp.size())
			/*emit*/ EPGUpdated( &temp );

		eDebug("[EPGC] stop cleanloop");
		eDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
	}
}

eEPGCache::~eEPGCache()
{
	for (eventCache::iterator evIt = eventDB.begin(); evIt != eventDB.end(); evIt++)
		for (eventMap::iterator It = evIt->second.first.begin(); It != evIt->second.first.end(); It++)
			delete It->second;
}

EITEvent *eEPGCache::lookupEvent(const eServiceReferenceDVB &service, int event_id, bool plain)
{
	uniqueEPGKey key( service );

	eventCache::iterator It = eventDB.find( key );
	if ( It != eventDB.end() && !It->second.first.empty() ) // entrys cached?
	{
		eventMap::iterator i( It->second.first.find( event_id ));
		if ( i != It->second.first.end() )
		{
			if ( service.getServiceType() == 4 ) // nvod ref
				return lookupEvent( service, i->second->getStartTime(), plain );
			else if ( plain )
		// get plain data... not in EITEvent Format !!!
		// before use .. cast it to eit_event_struct*
				return (EITEvent*) i->second->get();
			else
				return new EITEvent( *i->second );
		}
		else
			eDebug("event %04x not found in epgcache", event_id);
	}
	return 0;
}

EITEvent *eEPGCache::lookupEvent(const eServiceReferenceDVB &service, time_t t, bool plain )
// if t == 0 we search the current event...
{
	uniqueEPGKey key(service);

	// check if EPG for this service is ready...
	eventCache::iterator It = eventDB.find( key );
	if ( It != eventDB.end() && !It->second.first.empty() ) // entrys cached ?
	{
		if (!t)
			t = time(0)+eDVB::getInstance()->time_difference;

		if (service.getServiceType() == 4)// NVOD
		{
			// get NVOD Refs from this NVDO Service
			for ( std::list<NVODReferenceEntry>::iterator it( NVOD[key].begin() ); it != NVOD[key].end(); it++ )
			{
				eventCache::iterator evIt = eventDB.find(uniqueEPGKey( it->service_id, it->original_network_id, key.opos ));
				if ( evIt != eventDB.end() )
				{
					for ( eventMap::iterator emIt( evIt->second.first.begin() ); emIt != evIt->second.first.end(); emIt++)
					{
						EITEvent refEvt(*emIt->second);
						if ( t >= refEvt.start_time && t <= refEvt.start_time+refEvt.duration)
						{ // found the current start_time..
							for (ePtrList<Descriptor>::iterator d(refEvt.descriptor); d != refEvt.descriptor.end(); ++d)
							{
								Descriptor *descriptor=*d;
								if (descriptor->Tag()==DESCR_TIME_SHIFTED_EVENT)
								{
									int event_id = ((TimeShiftedEventDescriptor*)descriptor)->reference_event_id;
									for ( eventMap::iterator i( It->second.first.begin() ); i != It->second.first.end(); i++)
									{
										if ( HILO( i->second->get()->event_id ) == event_id )
										{
											if ( plain )
												// get plain data... not in EITEvent Format !!!
												// before use .. cast it to eit_event_struct*
												return (EITEvent*) i->second->get();
											EITEvent *evt = new EITEvent( *i->second );
											evt->start_time = refEvt.start_time;
											evt->duration = refEvt.duration;
											evt->event_id = refEvt.event_id;
											evt->free_CA_mode = refEvt.free_CA_mode;
											evt->running_status = refEvt.running_status;
											return evt;
										}
									}
								}
							}
						}
					}
				}
			}
		}

		timeMap::iterator i = It->second.second.lower_bound(t);
		if ( i != It->second.second.end() )
		{
			i--;
			if ( i != It->second.second.end() )
			{
				const eit_event_struct* eit_event = i->second->get();
				int duration = fromBCD(eit_event->duration_1)*3600+fromBCD(eit_event->duration_2)*60+fromBCD(eit_event->duration_3);
				if ( t <= i->first+duration )
				{
					if ( plain )
						// get plain data... not in EITEvent Format !!!
						// before use .. cast it to eit_event_struct*
						return (EITEvent*) i->second->get();
					return new EITEvent( *i->second );
				}
			}
		}

		for ( eventMap::iterator i( It->second.first.begin() ); i != It->second.first.end(); i++)
		{
			const eit_event_struct* eit_event = i->second->get();
			int duration = fromBCD(eit_event->duration_1)*3600+fromBCD(eit_event->duration_2)*60+fromBCD(eit_event->duration_3);
			time_t begTime = parseDVBtime( eit_event->start_time_1, eit_event->start_time_2,	eit_event->start_time_3, eit_event->start_time_4,	eit_event->start_time_5);
			if ( t >= begTime && t <= begTime+duration) // then we have found
			{
				if ( plain )
					// get plain data... not in EITEvent Format !!!
					// before use .. cast it to eit_event_struct*
					return (EITEvent*) i->second->get();
				return new EITEvent( *i->second );
			}
		}
	}
	return 0;
}

void eEPGCache::enterService(const eServiceReferenceDVB &service, int err)
{
	if ( service.path )
	{
		eDebug("[EPGC] dont start ... its a replay");
		/*emit*/ EPGAvail(0);
		return;
	}

	current_service = service;

	updateMap::iterator It = serviceLastUpdated.find( current_service );

	int update;

// check if this is a subservice and this is only a dbox2
// then we dont start epgcache on subservice change..
// ever and ever..
	if ( service.getServiceType() == 7 )
		switch ( eSystemInfo::getInstance()->getHwType() )
		{
			case eSystemInfo::dbox2Nokia:
			case eSystemInfo::dbox2Sagem:
			case eSystemInfo::dbox2Philips:
				err = 1111; //faked
			default:
				break;
		}

	if ( !err || err == -ENOCASYS || err == -ENVOD )
	{
		update = ( It != serviceLastUpdated.end() ? ( UPDATE_INTERVAL - ( (time(0)+eDVB::getInstance()->time_difference-It->second) * 1000 ) ) : ZAP_DELAY );

		if (update < ZAP_DELAY)
			update = ZAP_DELAY;

		zapTimer.start(update, 1);
		if (update >= 60000)
			eDebug("[EPGC] next update in %i min", update/60000);
		else if (update >= 1000)
			eDebug("[EPGC] next update in %i sec", update/1000);
	}

	if (It != serviceLastUpdated.end() && !eventDB[current_service].first.empty())
	{
		eDebug("[EPGC] yet cached");
		/*emit*/ EPGAvail(1);
	}
	else
	{
		eDebug("[EPGC] not cached yet");
		/*emit*/ EPGAvail(0);
	}
}

void eEPGCache::pauseEPG()
{
	if (!paused)
	{
		abortEPG();
		eDebug("[EPGC] paused]");
		paused=1;
	}
}

void eEPGCache::restartEPG()
{
	if (paused)
	{
		isRunning=0;
		eDebug("[EPGC] restarted");
		paused--;
		if (paused)
		{
			paused = 0;
			startEPG();   // updateEPG
		}
		cleanLoop();
	}
}

void eEPGCache::startEPG()
{
	if (paused)  // called from the updateTimer during pause...
	{
		paused++;
		return;
	}

	if (eDVB::getInstance()->time_difference)
	{
		if ( firstStart )
		{
			eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
			if ( !sapi || !sapi->transponder )
				return;
			std::map<tsref,int>::iterator it = eTransponderList::getInstance()->TimeOffsetMap.find(*sapi->transponder);
			if ( it == eTransponderList::getInstance()->TimeOffsetMap.end() )
			{
//				eDebug("TRANSPONDER NOT IN TIMEOFFSETMAP");
				/* emit */ timeNotValid( *sapi->transponder );
			}
			firstStart=0;
		}

		temp.clear();
		eDebug("[EPGC] start caching events");
		firstScheduleEvent.invalidate();
		firstNowNextEvent.invalidate();
		scheduleReader.start();
		isRunning |= 1;
		nownextReader.start();
		isRunning |= 2;
	}
	else
	{
		eDebug("[EPGC] wait for clock update");
		zapTimer.start(1000, 1); // restart Timer
	}
}

void eEPGCache::abortEPG(const eServiceReferenceDVB&)
{
	zapTimer.stop();
	if (isRunning)
	{
		if (isRunning & 1)
		{
			isRunning &= ~1;
			scheduleReader.abort();
		}
		if (isRunning & 2)
		{
			isRunning &= ~2;
			nownextReader.abort();
		}
		eDebug("[EPGC] abort caching events !!");
	}
}


eAutoInitP0<eEPGCache> init_eEPGCacheInit(eAutoInitNumbers::dvb+1, "EPG cache");

