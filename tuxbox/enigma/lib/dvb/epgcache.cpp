#include <lib/dvb/epgcache.h>

#undef NVOD   

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

#ifdef ZAP_DELAY
#undef ZAP_DELAY
#define ZAP_DELAY 2000
#endif

eEPGCache *eEPGCache::instance;
pthread_mutex_t eEPGCache::cache_lock=
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

eEPGCache::eEPGCache()
	:messages(this,1), paused(0), firstStart(1)
	,CleanTimer(this), zapTimer(this), abortTimer(this)
{
	eDebug("[EPGC] Initialized EPGCache");
	isRunning=0;
	scheduleReader.setContext(this);
	scheduleOtherReader.setContext(this);
	nownextReader.setContext(this);

	CONNECT(messages.recv_msg, eEPGCache::gotMessage);
	CONNECT(eDVB::getInstance()->switchedService, eEPGCache::enterService);
	CONNECT(eDVB::getInstance()->leaveService, eEPGCache::leaveService);
	CONNECT(eDVB::getInstance()->timeUpdated, eEPGCache::timeUpdated);
	CONNECT(zapTimer.timeout, eEPGCache::startEPG);
	CONNECT(CleanTimer.timeout, eEPGCache::cleanLoop);
	CONNECT(abortTimer.timeout, eEPGCache::abortNonAvail);
	instance=this;
}

void eEPGCache::timeUpdated()
{
	if ( !thread_running() )
	{
		eDebug("[EPGC] time updated.. start EPG Mainloop");
		run();
	}
	else
		messages.send(Message(Message::timeChanged));
}

int eEPGCache::sectionRead(__u8 *data, int source)
{
	if ( !data )
		return -ECANCELED;
	eit_t *eit = (eit_t*) data;
	int len=HILO(eit->section_length)-1;//+3-4;
	int ptr=EIT_SIZE;
	if ( ptr >= len )
		return 0;
	uniqueEPGKey service( HILO(eit->service_id), HILO(eit->original_network_id), HILO(eit->transport_stream_id) );
	eit_event_struct* eit_event = (eit_event_struct*) (data+ptr);
	int eit_event_size;
	int duration;

	time_t TM = parseDVBtime( eit_event->start_time_1, eit_event->start_time_2,	eit_event->start_time_3, eit_event->start_time_4, eit_event->start_time_5);
	time_t now = time(0)+eDVB::getInstance()->time_difference;

	if ( TM != 3599 )
	{
		uniqueEvent event( TM, HILO(eit_event->event_id), service );
		switch (source)
		{
			case SCHEDULE:
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
				break;
			case SCHEDULE_OTHER:
				if ( !firstScheduleOtherEvent.valid() )
				{
					eDebug("[EPGC] schedule other data begin");
					firstScheduleOtherEvent=event;
				}
				else if ( firstScheduleOtherEvent == event )  // epg around
				{
					eDebug("[EPGC] schedule other data ready");
					return -ECANCELED;
				}
				break;
			case NOWNEXT:
				if ( !firstNowNextEvent.valid() )
				{
					eDebug("[EPGC] nownext data begin");
					firstNowNextEvent = event;
				}
				else if ( firstNowNextEvent == event ) // now next ready
				{
					eDebug("[EPGC] nownext data ready");
					abortTimer.start(3000,true);
					return -ECANCELED;
				}
				break;
		}
	}

	Lock();
	// hier wird immer eine eventMap zurück gegeben.. entweder eine vorhandene..
	// oder eine durch [] erzeugte
	std::pair<eventMap,timeMap> &servicemap = eventDB[service];
	eventMap::iterator prevEventIt = servicemap.first.end();
	timeMap::iterator prevTimeIt = servicemap.second.end();

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

#ifndef NVOD
		if ( TM == 3599 )
			goto next;
#endif

		if ( TM != 3599 && (TM+duration < now || TM > now+14*24*60*60) )
			goto next;

		if ( now <= (TM+duration) || TM == 3599 /*NVOD Service*/ )  // old events should not be cached
		{
#ifdef NVOD
			// look in 1st descriptor tag.. i hope all time shifted events have the
			// time shifted event descriptor at the begin of the descriptors..
			if ( ((unsigned char*)eit_event)[12] == 0x4F ) // TIME_SHIFTED_EVENT_DESCR
			{
				// get Service ID of NVOD Service ( parent )
				int sid = ((unsigned char*)eit_event)[14] << 8 | ((unsigned char*)eit_event)[15];
				uniqueEPGKey parent( sid, HILO(eit->original_network_id), HILO(eit->transport_stream_id) );
				// check that this nvod reference currently is not in the NVOD List
				std::list<NVODReferenceEntry>::iterator it( NVOD[parent].begin() );
				for ( ; it != NVOD[parent].end(); it++ )
					if ( it->service_id == HILO(eit->service_id) && it->original_network_id == HILO( eit->original_network_id ) )
						break;
				if ( it == NVOD[parent].end() )  // not in list ?
					NVOD[parent].push_back( NVODReferenceEntry( HILO(eit->transport_stream_id), HILO(eit->original_network_id), HILO(eit->service_id) ) );
			}
#endif
			__u16 event_id = HILO(eit_event->event_id);
//			eDebug("event_id is %d sid is %04x", event_id, service.sid);

// search in timemap
			timeMap::iterator It =
				servicemap.second.find(TM);

// search in eventmap
			eventMap::iterator it =
				servicemap.first.find(event_id);

			eventData *evt = 0;

			bool debug=false;
			// entry with this event_id is already exist ?
			if ( it != servicemap.first.end() )
			  // we can update existing entry in 
			  // event_id map (do not rebuild sorted binary tree)
			{
				prevEventIt = it;

				if ( source > it->second->type )  // update needed ?
					goto next; // when not.. the skip this entry

				if ( It == servicemap.second.end() )
				{
					debug=true;
					time_t oldTM = it->second->getStartTime();
					eDebug("time changed old %d new %d", oldTM, TM );
					// when event_time has changed we must remove the old entry from time map
					It = servicemap.second.find(oldTM);
					if ( It == servicemap.second.end() )
						eFatal("!!!!No old event found sid %04x tsid %04x onid %04x size %d size2 %d, eventid %d, oldtime %d, newtime %d", 
							service.sid, service.tsid, service.onid, 
							servicemap.first.size(), servicemap.second.size(),
							event_id, oldTM, TM );
					else
						servicemap.second.erase(It);
					prevTimeIt=It=servicemap.second.end();
				}
				delete it->second;
				ASSERT(it->second=evt=new eventData(eit_event, eit_event_size, source));
				if ( debug )
					eDebug("update in map %d==%d %d", it->first, event_id, TM);
			}
			else // we must add new event.. ( in maps this is really slow.. )
			{
				// event with same start-time already in timemap?
				if ( It != servicemap.second.end() )
				{
					if ( source > It->second->type )
					{
//						eDebug("skip %d - %d", It->second->type, source );
						goto next;
					}
					bool bla=false;
//					eDebug("update %d -> %d", It->second->type, source );
					// we must search this event in servicemap ( realy slow :( )
					for (eventMap::iterator it(servicemap.first.begin())
						; it != servicemap.first.end(); ++it )
					{
						if ( it->second->getStartTime() == TM )
						{
							bla=true;
							delete it->second;
							servicemap.first.erase(it);
							prevEventIt=servicemap.first.end();
							break;
						}
					}
					if (!bla) 
						eFatal("old event in eventmap not found sid %04x tsid %04x onid %04x size %d size2 %d, eventid %d, time_begin %d", 
							service.sid, service.tsid, service.onid, 
							servicemap.first.size(), servicemap.second.size(),
							event_id, TM);
				}
				if (debug)
					eDebug("add new event_map entry time %d, event_id %d", TM, event_id);
				ASSERT(evt=new eventData(eit_event, eit_event_size, source));
				prevEventIt=servicemap.first.insert( prevEventIt, std::pair<const __u16, eventData*>( event_id, evt) );
			}
 
 // update only data pointer in timemap.. 
			if ( It != servicemap.second.end() ) 
			{
				if (debug)
					eDebug("update timemap entry time %d", TM);
				It->second=evt;
				prevTimeIt=It;
			}
			else  // add new entry to timemap
			{
#ifdef NVOD
				if ( TM != 3599 )
#endif
					prevTimeIt=servicemap.second.insert( prevTimeIt, std::pair<const time_t, eventData*>( TM, evt ) );
					if (debug)
						eDebug("add new time_map entry time %d", TM);
			}
			if ( servicemap.first.size() != servicemap.second.size() )
			{
				eFatal("(1)map sizes not equal :( sid %04x tsid %04x onid %04x size %d size2 %d, eventid %d, time_begin %d", 
					service.sid, service.tsid, service.onid, 
					servicemap.first.size(), servicemap.second.size(),
					event_id, TM );
			}
		}
next:
		ptr += eit_event_size;
		eit_event=(eit_event_struct*)(((__u8*)eit_event)+eit_event_size);
	}

	tmpMap::iterator it = temp.find( service );
	if ( it != temp.end() )
	{
		it->second.first=now;
		it->second.second=source;
	}
	else
		temp[service] = std::pair< time_t, int> (now, source);

	Unlock();
	return 0;
}

bool eEPGCache::finishEPG()
{
	if (!isRunning)  // epg ready
	{
		eDebug("[EPGC] stop caching events");
		zapTimer.start(UPDATE_INTERVAL, 1);
		eDebug("[EPGC] next update in %i min", UPDATE_INTERVAL / 60000);

		singleLock l(cache_lock);
		tmpMap::iterator It = temp.begin();

		while (It != temp.end())
		{
//		eDebug("sid = %02x, onid = %02x, opos = %d", It->first.sid, It->first.onid, It->first.opos);
			if ( It->second.second == SCHEDULE
				|| ( It->second.second == NOWNEXT && !firstScheduleEvent.valid() ) 
				)
			{
//				eDebug("ADD to last updated Map");
				serviceLastUpdated[It->first]=It->second.first;
			}
			if ( eventDB.find( It->first ) == eventDB.end() )
			{
//				eDebug("REMOVE from update Map");
				temp.erase(It);
				It = temp.begin();
			}
			else
				It++;
		}
		if (!eventDB[current_service].first.empty())
			/*emit*/ EPGAvail(1);

		/*emit*/ EPGUpdated();

		return true;
	}
	return false;
}

void eEPGCache::flushEPG(const uniqueEPGKey & s)
{
	eDebug("[EPGC] flushEPG %d", (int)(bool)s);
	Lock();
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
		}
		serviceLastUpdated.clear();
		eventDB.clear();
		startEPG();
	}
	eDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
	Unlock();
}

void eEPGCache::cleanLoop()
{
	singleLock s(cache_lock);
	if ( isRunning || temp.size() )
	{
		CleanTimer.start(5000,true);
		eDebug("[EPGC] schedule cleanloop");
		return;
	}
	if (!eventDB.empty() && !paused )
	{
		eDebug("[EPGC] start cleanloop");
		const eit_event_struct* cur_event;
		int duration;

		time_t now = time(0)+eDVB::getInstance()->time_difference;

		for (eventCache::iterator DBIt = eventDB.begin(); DBIt != eventDB.end(); DBIt++)
		{
			for (timeMap::iterator It = DBIt->second.second.begin(); It != DBIt->second.second.end();)
			{
				if ( DBIt->second.first.size() != DBIt->second.second.size() )
				{
					eFatal("(2)map sizes not equal :( sid %04x tsid %04x onid %04x size %d size2 %d", 
						DBIt->first.sid, DBIt->first.tsid, DBIt->first.onid, 
						DBIt->second.first.size(), DBIt->second.second.size() );
				}
				cur_event = (*It->second).get();

				duration = fromBCD( cur_event->duration_1)*3600 + fromBCD(cur_event->duration_2)*60 + fromBCD(cur_event->duration_3);

				if ( now > (It->first+duration) )  // outdated normal entry (nvod references to)
				{
					// remove entry from eventMap
					eventMap::iterator b(DBIt->second.first.find(It->second->getEventID()));
					if ( b != DBIt->second.first.end() )
					{
//						eDebug("old %d", DBIt->second.first.size() );
						eDebug("[EPGC] delete old event (evmap)");
						// release Heap Memory for this entry   (new ....)
						delete b->second;
						DBIt->second.first.erase(b);
//						eDebug("new %d", DBIt->second.first.size() );
					}
					else
						eFatal("[EPGC] event not found sid %04x tsid %04x onid %04x size %d size2 %d, eventid %d, time %d", 
							DBIt->first.sid, DBIt->first.tsid, DBIt->first.onid, 
							DBIt->second.first.size(), DBIt->second.second.size(),
							It->second->getEventID(), It->second->getStartTime() );

					// remove entry from timeMap
//					eDebug("old %d", DBIt->second.second.size() );
					DBIt->second.second.erase(It);
					eDebug("[EPGC] delete old event (timeMap)");
//					eDebug("new %d", DBIt->second.second.size() );
					It=DBIt->second.second.begin();  // start at begin
					
					// add this (changed) service to temp map...
					if ( temp.find(DBIt->first) == temp.end() )
						temp[DBIt->first]=std::pair<time_t, int>(now, NOWNEXT);

//					eDebug("%d == %d", 
//						DBIt->second.first.size(),
//						DBIt->second.second.size());
					ASSERT(DBIt->second.first.size() == DBIt->second.second.size());
				}
				else  // valid entry found
							// we must not check any other event in this map
							// beginTime is sort key...
					break;
			}
			if ( DBIt->second.second.size() < 2 )  
			// less than two events for this service in cache.. 
			{
				updateMap::iterator u = serviceLastUpdated.find(DBIt->first);
				if ( u != serviceLastUpdated.end() )
				{
					// remove from lastupdated map.. 
					serviceLastUpdated.erase(u);
					// current service?
					if ( DBIt->first == current_service )
					{
//						eDebug("%08x == %08x, %08x == %08x, %08x == %08x",
//							DBIt->first.sid, current_service.sid, 
//							DBIt->first.onid, current_service.onid,
//							DBIt->first.opos, current_service.opos);
					// immediate .. after leave cleanloop 
					// update epgdata for this service
						zapTimer.start(0,true);
					}
				}
			}
		}

#ifdef NVOD
		for (nvodMap::iterator it(NVOD.begin()); it != NVOD.end(); ++it )
		{
			eDebug("check NVOD Service");
			eventCache::iterator evIt(eventDB.find(it->first));
			if ( evIt != eventDB.end() && evIt->second.first.size() )
			{
				for ( eventMap::iterator i(evIt->second.first.begin());
					i != evIt->second.first.end(); )
				{
//					ASSERT(i->second->getStartTime() == 3599);
					int cnt=0;
					for ( std::list<NVODReferenceEntry>::iterator ni(it->second.begin());
						ni != it->second.end(); ++ni )
					{
						eventCache::iterator nie(eventDB.find(uniqueEPGKey(ni->service_id, ni->original_network_id, ni->transport_stream_id) ) );
						if (nie != eventDB.end() && nie->second.first.find( i->first ) != nie->second.first.end() )
						{
							++cnt;
							break;
						}
					}
					if ( !cnt ) // no more referenced
					{
						delete i->second;  // free memory
						evIt->second.first.erase(i);  // remove from eventmap
						i = evIt->second.first.begin();  // begin at first..
					}
					else 
						++i;
				}
			}
		}
#endif

		if (temp.size())
			/*emit*/ EPGUpdated();

		eDebug("[EPGC] stop cleanloop");
		eDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
	}
	CleanTimer.start(CLEAN_INTERVAL,true);
}

eEPGCache::~eEPGCache()
{
	messages.send(Message::quit);
	kill(); // waiting for thread shutdown
	Lock();
	for (eventCache::iterator evIt = eventDB.begin(); evIt != eventDB.end(); evIt++)
		for (eventMap::iterator It = evIt->second.first.begin(); It != evIt->second.first.end(); It++)
			delete It->second;
	Unlock();
}

EITEvent *eEPGCache::lookupEvent(const eServiceReferenceDVB &service, int event_id, bool plain)
{
	singleLock s(cache_lock);
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
	singleLock s(cache_lock);
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
				eventCache::iterator evIt = eventDB.find(uniqueEPGKey( it->service_id, it->original_network_id, it->transport_stream_id ));
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
			firstStart=0;
		}
		Lock();
		temp.clear();
		Unlock();
		eDebug("[EPGC] start caching events");
		firstScheduleEvent.invalidate();
		firstNowNextEvent.invalidate();
		firstScheduleOtherEvent.invalidate();
		scheduleReader.start();
		isRunning |= 1;
		nownextReader.start();
		isRunning |= 2;
		scheduleOtherReader.start();
		isRunning |= 4;
		abortTimer.start(15000,true);  
	// when after 15seks non data is received.. this service have no epg
	}
	else
	{
		eDebug("[EPGC] wait for clock update");
		zapTimer.start(1000, 1); // restart Timer
	}
}

void eEPGCache::abortNonAvail()
{
	if ( !firstNowNextEvent.valid() && (isRunning&2) )
	{
		eDebug("[EPGC] abort non avail nownext reading");
		isRunning &= ~2;
		nownextReader.abort();
	}
	if ( !firstScheduleEvent.valid() && (isRunning&1) )
	{
		eDebug("[EPGC] abort non avail schedule reading");
		isRunning &= ~1;
		scheduleReader.abort();
	}
	if ( !firstScheduleOtherEvent.valid() && (isRunning&4) )
	{
		eDebug("[EPGC] abort non avail schedule_other reading");
		isRunning &= ~4;
		scheduleOtherReader.abort();
	}
	finishEPG();
}

void eEPGCache::enterService(const eServiceReferenceDVB& ref, int err)
{
	if ( ref.path )
		err = 2222;
	else if ( ref.getServiceType() == 7 )
		switch ( eSystemInfo::getInstance()->getHwType() )
		{
			case eSystemInfo::dbox2Nokia:
			case eSystemInfo::dbox2Sagem:
			case eSystemInfo::dbox2Philips:
				err = 1111; //faked
			default:
				break;
		}

	messages.send(Message(Message::enterService, ref, err));
	// -> gotMessage -> changedService
}

void eEPGCache::leaveService(const eServiceReferenceDVB& ref)
{
	messages.send(Message(Message::leaveService, ref));
	// -> gotMessage -> abortEPG
}

void eEPGCache::changedService(const uniqueEPGKey &service, int err)
{
	if ( err == 2222 )
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

	Lock();
	bool empty=eventDB[current_service].first.empty();
	Unlock();

	if (!empty)
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

void eEPGCache::abortEPG()
{
	abortTimer.stop();
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
		if (isRunning & 4)
		{
			isRunning &= ~4;
			scheduleOtherReader.abort();
		}
		eDebug("[EPGC] abort caching events !!");
		Lock();
		temp.clear();
		Unlock();
	}
}

void eEPGCache::gotMessage( const Message &msg )
{
	switch (msg.type)
	{
		case Message::flush:
			flushEPG(msg.service);
			break;
		case Message::enterService:
			changedService(msg.service, msg.err);
			break;
		case Message::leaveService:
			// msg.service is valid.. but not needed
			abortEPG();
			break;
		case Message::pause:
			pauseEPG();
			break;
		case Message::restart:
			restartEPG();
			break;
		case Message::quit:
			quit(0);
			break;
		case Message::timeChanged:
			cleanLoop();
			break;
		default:
			eDebug("unhandled EPGCache Message!!");
			break;
	}
}

void eEPGCache::thread()
{
	nice(4);
	CleanTimer.start(CLEAN_INTERVAL,true);
	exec();
}

eAutoInitP0<eEPGCache> init_eEPGCacheInit(eAutoInitNumbers::dvb+1, "EPG cache");

