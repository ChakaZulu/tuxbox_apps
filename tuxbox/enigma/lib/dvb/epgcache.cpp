#include <lib/dvb/epgcache.h>

#undef NVOD
#undef EPG_DEBUG  

#include <time.h>
#include <unistd.h>  // for usleep
#include <sys/vfs.h> // for statfs
#include <libmd5sum.h>
#include <lib/system/info.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/dvb/lowlevel/dvb.h>
#include <lib/dvb/si.h>
#ifdef ENABLE_MHW_EPG
#include <lib/dvb/lowlevel/mhw.h>
#endif
#include <lib/dvb/service.h>
#include <lib/dvb/dvbservice.h>

int eventData::CacheSize=0;

eEPGCache *eEPGCache::instance;
pthread_mutex_t eEPGCache::cache_lock=
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

descriptorMap eventData::descriptors;
__u8 eventData::data[4108];

extern unsigned int crc32_table[256];

eventData::eventData(const eit_event_struct* e, int size, int type)
	:ByteSize(size&0xFF), type(type&0xFF)
{
	if (!e)
		return;

	__u32 descr[65];
	__u32 *pdescr=descr;

	__u8 *data = (__u8*)e;
	int ptr=12;
	size -= 12;

	while(size > 1)
	{
		__u8 *descr = data+ptr;
		int descr_len = descr[1];
		descr_len += 2;
		if (size >= descr_len)
		{
			switch (descr[0])
			{
				case DESCR_EXTENDED_EVENT:
				case DESCR_SHORT_EVENT:
				case DESCR_LINKAGE:
				case DESCR_COMPONENT:
				case DESCR_CONTENT:
				case DESCR_TIME_SHIFTED_EVENT:
				{
					__u32 crc = 0;
					int cnt=0;
					while(cnt++ < descr_len)
						crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ data[ptr++]) & 0xFF];

					descriptorMap::iterator it =
						descriptors.find(crc);
					if ( it == descriptors.end() )
					{
						CacheSize+=descr_len;
						__u8 *d = new __u8[descr_len];
						memcpy(d, descr, descr_len);
						descriptors[crc] = descriptorPair(1, d);
					}
					else
						++it->second.first;
					*pdescr++=crc;
					break;
				}
				default: // do not cache all other descriptors
					ptr += descr_len;
					break;
			}
			size -= descr_len;
		}
		else
			break;
	}
	ASSERT(pdescr <= &descr[65]);
	ByteSize = 10+((pdescr-descr)*4);
	EITdata = new __u8[ByteSize];
	CacheSize+=ByteSize;
	memcpy(EITdata, (__u8*) e, 10);
	memcpy(EITdata+10, descr, ByteSize-10);
}

const eit_event_struct* eventData::get() const
{
	int pos = 12;
	int tmp = ByteSize-10;
	memcpy(data, EITdata, 10);
	int descriptors_length=0;
	__u32 *p = (__u32*)(EITdata+10);
	while(tmp>3)
	{
		descriptorMap::iterator it =
			descriptors.find(*p++);
		if ( it != descriptors.end() )
		{
			int b = it->second.second[1]+2;
			memcpy(data+pos, it->second.second, b );
			pos += b;
			descriptors_length += b;
		}
		else
			eFatal("LINE %d descriptor not found in descriptor cache %08x!!!!!!", __LINE__, *(p-1));
		tmp-=4;
	}
	ASSERT(pos <= 4108);
	data[10] = (descriptors_length >> 8) & 0x0F;
	data[11] = descriptors_length & 0xFF;
	return (eit_event_struct*)data;
}

eventData::~eventData()
{
	if ( ByteSize )
	{
		CacheSize -= ByteSize;
		__u32 *d = (__u32*)(EITdata+10);
		ByteSize -= 10;
		while(ByteSize>3)
		{
			descriptorMap::iterator it =
				descriptors.find(*d++);
			if ( it != descriptors.end() )
			{
				descriptorPair &p = it->second;
				if (!--p.first) // no more used descriptor
				{
					CacheSize -= it->second.second[1];
					delete [] it->second.second;  	// free descriptor memory
					descriptors.erase(it);	// remove entry from descriptor map
				}
			}
			else
				eFatal("LINE %d descriptor not found in descriptor cache %08x!!!!!!", __LINE__, *(d-1));
			ByteSize -= 4;
		}
		delete [] EITdata;
	}
}

void eventData::load(FILE *f)
{
	int size=0;
	int id=0;
	__u8 header[2];
	descriptorPair p;
	fread(&size, sizeof(int), 1, f);
	while(size)
	{
		fread(&id, sizeof(__u32), 1, f);
		fread(&p.first, sizeof(int), 1, f);
		fread(header, 2, 1, f);
		int bytes = header[1]+2;
		p.second = new __u8[bytes];
		p.second[0] = header[0];
		p.second[1] = header[1];
		fread(p.second+2, bytes-2, 1, f);
		descriptors[id]=p;
		--size;
		CacheSize+=bytes;
	}
}

void eventData::save(FILE *f)
{
	int size=descriptors.size();
	descriptorMap::iterator it(descriptors.begin());
	fwrite(&size, sizeof(int), 1, f);
	while(size)
	{
		fwrite(&it->first, sizeof(__u32), 1, f);
		fwrite(&it->second.first, sizeof(int), 1, f);
		fwrite(it->second.second, it->second.second[1]+2, 1, f);
		++it;
		--size;
	}
}

eEPGCache::eEPGCache()
	:messages(this,1), paused(0), firstStart(1)
	,CleanTimer(this), zapTimer(this), abortTimer(this)
{
	eDebug("[EPGC] Initialized EPGCache");
	isRunning=0;
	scheduleReader.setContext(this);
	scheduleOtherReader.setContext(this);
#ifdef ENABLE_MHW_EPG
	scheduleMhwReader.setContext(this);
#endif
	nownextReader.setContext(this);
#ifdef ENABLE_PRIVATE_EPG
	contentReader.setContext(this);
	CONNECT(eDVB::getInstance()->gotContentPid, eEPGCache::setContentPid);
#endif
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
		if ( cached_service )
			enterService(cached_service, cached_err);
	}
	else
		messages.send(Message(Message::timeChanged));
}

#ifdef ENABLE_PRIVATE_EPG

struct date_time
{
	__u8 data[5];
	time_t tm;
	date_time( const date_time &a )
	{
		memcpy(data, a.data, 5);
		tm = a.tm;
	}
	date_time( const __u8 data[5])
	{
		memcpy(this->data, data, 5);
		tm = parseDVBtime(data[0], data[1], data[2], data[3], data[4]);
	}
	date_time()
	{
	}
	const __u8& operator[](int pos) const
	{
		return data[pos];
	}
};

struct less_datetime
{
	bool operator()( const date_time &a, const date_time &b ) const
	{
		return abs(a.tm-b.tm) < 360 ? false : a.tm < b.tm;
	}
};

int ePrivateContent::sectionRead(__u8 *data)
{
	eEPGCache &instance = *eEPGCache::getInstance();
	uniqueEPGKey &current_service = instance.current_service;
	contentMap &content_time_table = instance.content_time_tables[current_service];
	singleLock s(instance.cache_lock);
	if ( instance.paused )
		return 0;
	if ( seenSections.find( data[6] ) == seenSections.end() )
	{
		std::map< date_time, std::list<uniqueEPGKey>, less_datetime > start_times;
		eventCache &eventDB = instance.eventDB;
		eventMap &evMap = eventDB[current_service].first;
		timeMap &tmMap = eventDB[current_service].second;
		int ptr=8;
		int content_id = data[ptr++] << 24;
		content_id |= data[ptr++] << 16;
		content_id |= data[ptr++] << 8;
		content_id |= data[ptr++];

		contentTimeMap &time_event_map =
			content_time_table[content_id];
		for ( contentTimeMap::iterator it( time_event_map.begin() );
			it != time_event_map.end(); ++it )
		{
			eventMap::iterator evIt( evMap.find(it->second.second) );
			if ( evIt != evMap.end() )
			{
				delete evIt->second;
				evMap.erase(evIt);
			}
			tmMap.erase(it->second.first);
		}
		time_event_map.clear();

		__u8 duration[3];
		memcpy(duration, data+ptr, 3);
		ptr+=3;
		int duration_sec =
			fromBCD(duration[0])*3600+fromBCD(duration[1])*60+fromBCD(duration[2]);

		__u8 *descriptors[65];
		__u8 **pdescr = descriptors;

		int descriptors_length = (data[ptr++]&0x0F) << 8;
		descriptors_length |= data[ptr++];
		while ( descriptors_length > 1 )
		{
			int descr_type = data[ptr];
			int descr_len = data[ptr+1];
			descriptors_length -= 2;
			if (descriptors_length >= descr_len)
			{
				descriptors_length -= descr_len;
				if ( descr_type == 0xf2 && descr_len > 5)
				{
					ptr+=2;
					int tsid = data[ptr++] << 8;
					tsid |= data[ptr++];
					int onid = data[ptr++] << 8;
					onid |= data[ptr++];
					int sid = data[ptr++] << 8;
					sid |= data[ptr++];

// WORKAROUND for wrong transmitted epg data (01.08.2006)
					if ( onid == 0x85 )
					{
						switch( (tsid << 16) | sid )
						{
							case 0x01030b: sid = 0x1b; tsid = 4; break;  // Premiere Win
							case 0x0300f0: sid = 0xe0; tsid = 2; break;
							case 0x0300f1: sid = 0xe1; tsid = 2; break;
							case 0x0300f5: sid = 0xdc; break;
							case 0x0400d2: sid = 0xe2; tsid = 0x11; break;
							case 0x1100d3: sid = 0xe3; break;
						}
					}
////////////////////////////////////////////

					uniqueEPGKey service( sid, onid, tsid );
					descr_len -= 6;
					while( descr_len > 2 )
					{
						__u8 datetime[5];
						datetime[0] = data[ptr++];
						datetime[1] = data[ptr++];
						int tmp_len = data[ptr++];
						descr_len -= 3;
						if (descr_len >= tmp_len)
						{
							descr_len -= tmp_len;
							while( tmp_len > 2 )
							{
								memcpy(datetime+2, data+ptr, 3);
								ptr += 3;
								tmp_len -= 3;
								start_times[datetime].push_back(service);
							}
						}
					}
				}
				else
				{
					*pdescr++=data+ptr;
					ptr += 2;
					ptr += descr_len;
				}
			}
		}
		ASSERT(pdescr <= &descriptors[65]);
		__u8 event[4098];
		eit_event_struct *ev_struct = (eit_event_struct*) event;
		ev_struct->running_status = 0;
		ev_struct->free_CA_mode = 1;
		memcpy(event+7, duration, 3);
		ptr = 12;
		__u8 **d=descriptors;
		while ( d < pdescr )
		{
			memcpy(event+ptr, *d, ((*d)[1])+2);
			ptr+=(*d++)[1];
			ptr+=2;
		}
		ASSERT(ptr <= 4098);
		for ( std::map< date_time, std::list<uniqueEPGKey> >::iterator it(start_times.begin()); it != start_times.end(); ++it )
		{
			time_t now = time(0)+eDVB::getInstance()->time_difference;
			if ( (it->first.tm + duration_sec) < now )
				continue;
			memcpy(event+2, it->first.data, 5);
			int bptr = ptr;
			int cnt=0;
			for (std::list<uniqueEPGKey>::iterator i(it->second.begin()); i != it->second.end(); ++i)
			{
				event[bptr++] = 0x4A;
				__u8 *len = event+(bptr++);
				event[bptr++] = (i->tsid & 0xFF00) >> 8;
				event[bptr++] = (i->tsid & 0xFF);
				event[bptr++] = (i->onid & 0xFF00) >> 8;
				event[bptr++] = (i->onid & 0xFF);
				event[bptr++] = (i->sid & 0xFF00) >> 8;
				event[bptr++] = (i->sid & 0xFF);
				event[bptr++] = 0xB0;
				bptr += sprintf((char*)(event+bptr), "Option %d", ++cnt);
				*len = ((event+bptr) - len)-1;
			}
			int llen = bptr - 12;
			ev_struct->descriptors_loop_length_hi = (llen & 0xF00) >> 8;
			ev_struct->descriptors_loop_length_lo = (llen & 0xFF);

			time_t stime = it->first.tm;
			while( tmMap.find(stime) != tmMap.end() )
				++stime;
			event[6] += (stime - it->first.tm);
			__u16 event_id = 0;
			while( evMap.find(event_id) != evMap.end() )
				++event_id;
			event[0] = (event_id & 0xFF00) >> 8;
			event[1] = (event_id & 0xFF);
			time_event_map[it->first.tm]=std::pair<time_t, __u16>(stime, event_id);
			eventData *d = new eventData( ev_struct, bptr, eEPGCache::PRIVATE );
			evMap[event_id] = d;
			tmMap[stime] = d;
			ASSERT(bptr <= 4098);
		}
		seenSections.insert(data[6]);
	}
	if ( seenSections.size() == (unsigned int)(data[7] + 1) )
	{
		if (!instance.eventDB[current_service].first.empty())
		{
			/*emit*/ instance.EPGAvail(1);
			instance.temp[current_service] =
				std::pair<time_t, int>(time(0)+eDVB::getInstance()->time_difference, eEPGCache::NOWNEXT);
			/*emit*/ instance.EPGUpdated();
		}
		int version = data[5];
		if ( eSystemInfo::getInstance()->hasNegFilter() )
			version = ((version & 0x3E) >> 1);
		else
			version = (version&0xC1)|((version+2)&0x3E);
		start_filter(pid,version);
	}
	return 0;
}
#endif // ENABLE_PRIVATE_EPG

void eEPGCache::FixOverlapping(std::pair<eventMap,timeMap> &servicemap, time_t TM, int duration, const timeMap::iterator &tm_it, const uniqueEPGKey &service)
{
	timeMap::iterator tmp = tm_it;
	while ((tmp->first+tmp->second->getDuration()-300) > TM)
	{
		if(tmp->first != TM
#ifdef ENABLE_PRIVATE_EPG
			&& tmp->second->type != PRIVATE
#endif
#ifdef ENABLE_MHW_EPG
			&& tmp->second->type != SCHEDULE_MHW
#endif
			)
		{
			__u16 event_id = tmp->second->getEventID();
			servicemap.first.erase(event_id);
			delete tmp->second;
			if (tmp == servicemap.second.begin())
			{
				servicemap.second.erase(tmp);
				break;
			}
			else
				servicemap.second.erase(tmp--);
		}
		else
		{
			if (tmp == servicemap.second.begin())
				break;
			--tmp;
		}
	}

	tmp = tm_it;
	while(tmp->first < (TM+duration-300))
	{
		if(tmp->first != TM
#ifdef ENABLE_PRIVATE_EPG
			&& tmp->second->type != PRIVATE
#endif
#ifdef ENABLE_MHW_EPG
			&& tmp->second->type != SCHEDULE_MHW
#endif
			)
		{
			__u16 event_id = tmp->second->getEventID();
			servicemap.first.erase(event_id);
			delete tmp->second;
			servicemap.second.erase(tmp++);
		}
		else
			++tmp;
		if (tmp == servicemap.second.end())
			break;
	}
}

int eEPGCache::sectionRead(__u8 *data, int source)
{
	if ( !data || state > 1 )
		return -ECANCELED;

	eit_t *eit = (eit_t*) data;
	bool seen=false;
	tidMap &seenSections = this->seenSections[source/2]; // source is 1,2,4 .. so index is 0,1,2
	tidMap &calcedSections = this->calcedSections[source/2];

#ifdef ENABLE_MHW_EPG
	if ( source != SCHEDULE_MHW )
	// In case of SCHEDULE_MHW the procedure that is feeding the data will send each section once.
	{
#endif
		__u32 sectionNo = data[0] << 24;
		sectionNo |= data[3] << 16;
		sectionNo |= data[4] << 8;
		sectionNo |= eit->section_number;

		tidMap::iterator it =
			seenSections.find(sectionNo);

		if ( it != seenSections.end() )
			seen=true;
		else
		{
			seenSections.insert(sectionNo);
			calcedSections.insert(sectionNo);
			__u32 tmpval = sectionNo & 0xFFFFFF00;
			__u8 incr = source == NOWNEXT ? 1 : 8;
			for ( int i = 0; i <= eit->last_section_number; i+=incr )
			{
				if ( i == eit->section_number )
				{
					for (int x=i; x <= eit->segment_last_section_number; ++x)
						calcedSections.insert(tmpval|(x&0xFF));
				}
				else
					calcedSections.insert(tmpval|(i&0xFF));
			}
		}
#ifdef ENABLE_MHW_EPG
	}
#endif

	int len=HILO(eit->section_length)-1;//+3-4;
	int ptr=EIT_SIZE;
	if ( ptr < len && !seen )
	{
		// This fixed the EPG on the Multichoice irdeto systems
		// the EIT packet is non-compliant.. their EIT packet stinks
		if ( data[ptr-1] < 0x40 )
			--ptr;

		uniqueEPGKey service( HILO(eit->service_id), HILO(eit->original_network_id), HILO(eit->transport_stream_id) );

		eit_event_struct* eit_event = (eit_event_struct*) (data+ptr);
		int eit_event_size;
		int duration;

		time_t TM = parseDVBtime( eit_event->start_time_1, eit_event->start_time_2,	eit_event->start_time_3, eit_event->start_time_4, eit_event->start_time_5);
		time_t now = time(0)+eDVB::getInstance()->time_difference;

		if ( TM != 3599 && TM > -1 && source < 8)
			haveData |= source;

#ifdef ENABLE_MHW_EPG
		if ( haveData & (SCHEDULE|SCHEDULE_OTHER) && isRunning & SCHEDULE_MHW )
		{
			eDebug("[EPGC] si schedule data avail.. abort mhw reader");
			isRunning &= ~SCHEDULE_MHW;
			scheduleMhwReader.abort();
		}
#endif

		Lock();
		// hier wird immer eine eventMap zurück gegeben.. entweder eine vorhandene..
		// oder eine durch [] erzeugte
		std::pair<eventMap,timeMap> &servicemap = eventDB[service];
		eventMap::iterator prevEventIt = servicemap.first.end();
		timeMap::iterator prevTimeIt = servicemap.second.end();
	
		while (ptr<len)
		{
			eit_event_size = HILO(eit_event->descriptors_loop_length)+EIT_LOOP_SIZE;

			if ( eit_event_size < EIT_LOOP_SIZE+	2 ) // skip events without descriptors
				goto next;
	
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
//				eDebug("event_id is %d sid is %04x", event_id, service.sid);

				eventData *evt = 0;
				int ev_erase_count = 0;
				int tm_erase_count = 0;

// search in eventmap
				eventMap::iterator ev_it =
					servicemap.first.find(event_id);

				// entry with this event_id is already exist ?
				if ( ev_it != servicemap.first.end() )
				{
					if ( source > ev_it->second->type )  // update needed ?
						goto next; // when not.. the skip this entry
// search this event in timemap
					timeMap::iterator tm_it_tmp =
						servicemap.second.find(ev_it->second->getStartTime());

					if ( tm_it_tmp != servicemap.second.end() )
					{
						if ( tm_it_tmp->first == TM ) // correct eventData
						{
							// exempt memory
							eventData *tmp = ev_it->second;
							ev_it->second = tm_it_tmp->second =
								new eventData(eit_event, eit_event_size, source);
							FixOverlapping(servicemap, TM, duration, tm_it_tmp, service);
							delete tmp;
							goto next;
						}
						else
						{
							tm_erase_count++;
							// delete the found record from timemap
							servicemap.second.erase(tm_it_tmp);
							prevTimeIt=servicemap.second.end();
						}
					}
				}

// search in timemap, for check of a case if new time has coincided with time of other event
// or event was is not found in eventmap
				timeMap::iterator tm_it =
					servicemap.second.find(TM);

				if ( tm_it != servicemap.second.end() )
				{
					if ( source > tm_it->second->type && tm_erase_count == 0 ) // update needed ?
						goto next; // when not.. the skip this entry

// search this time in eventmap
					eventMap::iterator ev_it_tmp =
						servicemap.first.find(tm_it->second->getEventID());
	
					if ( ev_it_tmp != servicemap.first.end() )
					{
						ev_erase_count++;
						// delete the found record from eventmap
						servicemap.first.erase(ev_it_tmp);
						prevEventIt=servicemap.first.end();
					}
				}

				evt = new eventData(eit_event, eit_event_size, source);
#if EPG_DEBUG
				bool consistencyCheck=true;
#endif
				if (ev_erase_count > 0 && tm_erase_count > 0) // 2 different pairs have been removed
				{
					// exempt memory
					delete ev_it->second;
					delete tm_it->second;
					ev_it->second=evt;
					tm_it->second=evt;
				}
				else if (ev_erase_count == 0 && tm_erase_count > 0)
				{
					// exempt memory
					delete ev_it->second;
					tm_it=prevTimeIt=servicemap.second.insert( prevTimeIt, std::pair<const time_t, eventData*>( TM, evt ) );
					ev_it->second=evt;
				}
				else if (ev_erase_count > 0 && tm_erase_count == 0)
				{
					// exempt memory
					delete tm_it->second;
					ev_it=prevEventIt=servicemap.first.insert( prevEventIt, std::pair<const __u16, eventData*>( event_id, evt) );
					tm_it->second=evt;
				}
				else // added new eventData
				{
#if EPG_DEBUG
					consistencyCheck=false;
#endif
					ev_it=prevEventIt=servicemap.first.insert( prevEventIt, std::pair<const __u16, eventData*>( event_id, evt) );
					tm_it=prevTimeIt=servicemap.second.insert( prevTimeIt, std::pair<const time_t, eventData*>( TM, evt ) );
				}

				FixOverlapping(servicemap, TM, duration, tm_it, service);

#if EPG_DEBUG
				if ( consistencyCheck )
				{
					if ( tm_it->second != evt || ev_it->second != evt )
						eFatal("tm_it->second != ev_it->second");
					else if ( tm_it->second->getStartTime() != tm_it->first )
						eFatal("event start_time(%d) non equal timemap key(%d)",
							tm_it->second->getStartTime(), tm_it->first );
					else if ( tm_it->first != TM )
						eFatal("timemap key(%d) non equal TM(%d)",
							tm_it->first, TM);
					else if ( ev_it->second->getEventID() != ev_it->first )
						eFatal("event_id (%d) non equal event_map key(%d)",
							ev_it->second->getEventID(), ev_it->first);
					else if ( ev_it->first != event_id )
						eFatal("eventmap key(%d) non equal event_id(%d)",
							ev_it->first, event_id );
				}
#endif
			}
next:
#if EPG_DEBUG
			if ( servicemap.first.size() != servicemap.second.size() )
			{
				FILE *f = fopen("/hdd/event_map.txt", "w+");
				int i=0;
				for (eventMap::iterator it(servicemap.first.begin())
					; it != servicemap.first.end(); ++it )
					fprintf(f, "%d(key %d) -> time %d, event_id %d, data %p\n",
						i++, (int)it->first, (int)it->second->getStartTime(), (int)it->second->getEventID(), it->second );
				fclose(f);
				f = fopen("/hdd/time_map.txt", "w+");
				i=0;
				for (timeMap::iterator it(servicemap.second.begin())
					; it != servicemap.second.end(); ++it )
				fprintf(f, "%d(key %d) -> time %d, event_id %d, data %p\n",
					i++, (int)it->first, (int)it->second->getStartTime(), (int)it->second->getEventID(), it->second );
				fclose(f);
	
				eFatal("(1)map sizes not equal :( sid %04x tsid %04x onid %04x size %d size2 %d",
					service.sid, service.tsid, service.onid,
					servicemap.first.size(), servicemap.second.size() );
			}
#endif
			ptr += eit_event_size;
			eit_event=(eit_event_struct*)(((__u8*)eit_event)+eit_event_size);
		}

		tmpMap::iterator it = temp.find( service );
		if ( it != temp.end() )
		{
			if ( source > it->second.second )
			{
				it->second.first=now;
				it->second.second=source;
			}
		}
		else
			temp[service] = std::pair< time_t, int> (now, source);

		Unlock();
	}

#ifdef ENABLE_MHW_EPG
	if ( source != SCHEDULE_MHW && (state == 1 && seenSections == calcedSections) )
#else
	if ( state == 1 && seenSections == calcedSections )
#endif
		return -ECANCELED;

	return 0;
}

bool eEPGCache::finishEPG()
{
	if (!isRunning)  // epg ready
	{
		eDebug("[EPGC] stop caching events(%d)", time(0)+eDVB::getInstance()->time_difference );
		for (int i=0; i < 3; ++i)
		{
			seenSections[i].clear();
			calcedSections[i].clear();
		}
		zapTimer.start(UPDATE_INTERVAL, 1);
		eDebug("[EPGC] next update in %i min", UPDATE_INTERVAL / 60000);

		singleLock l(cache_lock);
		tmpMap::iterator It = temp.begin();
		abortTimer.stop();

		while (It != temp.end())
		{
//			eDebug("sid = %02x, onid = %02x, type %d", It->first.sid, It->first.onid, It->second.second );
#ifdef ENABLE_MHW_EPG
			// Only add services to LastUpdated if they are part of the current stream.
			if ( current_service.tsid == It->first.tsid || It->second.second == SCHEDULE || It->second.second == SCHEDULE_MHW )
#else
			if ( It->first.tsid == current_service.tsid || It->second.second == SCHEDULE )
#endif
			{
//				eDebug("ADD to last updated Map");
				serviceLastUpdated[It->first]=It->second.first;
			}
			if ( eventDB.find( It->first ) == eventDB.end() )
			{
//				eDebug("REMOVE from update Map");
				temp.erase(It++);
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
	singleLock l(cache_lock);
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
#ifdef ENABLE_PRIVATE_EPG
			contentMaps::iterator it =
				content_time_tables.find(s);
			if ( it != content_time_tables.end() )
			{
				it->second.clear();
				content_time_tables.erase(it);
			}
#endif
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
#ifdef ENABLE_PRIVATE_EPG
		content_time_tables.clear();
#endif
	}
	eDebug("[EPGC] %i bytes for cache used", eventData::CacheSize);
}

void eEPGCache::cleanLoop()
{
	singleLock s(cache_lock);
	if ( isRunning || (temp.size() && haveData) )
	{
		CleanTimer.start(5000,true);
		eDebug("[EPGC] schedule cleanloop");
		return;
	}
	if (!eventDB.empty() && !paused )
	{
		eDebug("[EPGC] start cleanloop");

		time_t now = time(0)+eDVB::getInstance()->time_difference;

		for (eventCache::iterator DBIt = eventDB.begin(); DBIt != eventDB.end(); DBIt++)
		{
			bool updated=false;
			for (timeMap::iterator It = DBIt->second.second.begin(); It != DBIt->second.second.end() && It->first < now;)
			{
				if ( now > (It->first+It->second->getDuration()) )  // outdated normal entry (nvod references to)
				{
					// remove entry from eventMap
					eventMap::iterator b(DBIt->second.first.find(It->second->getEventID()));
					if ( b != DBIt->second.first.end() )
					{
						// release Heap Memory for this entry   (new ....)
//						eDebug("[EPGC] delete old event (evmap)");
						DBIt->second.first.erase(b);
					}

					// remove entry from timeMap
//					eDebug("[EPGC] release heap mem");
					delete It->second;
					DBIt->second.second.erase(It++);
//					eDebug("[EPGC] delete old event (timeMap)");

					// add this (changed) service to temp map...
					if ( temp.find(DBIt->first) == temp.end() )
					{
						temp[DBIt->first]=std::pair<time_t, int>(now, NOWNEXT);
						updated=true;
					}
				}
				else
					++It;
			}

#ifdef ENABLE_PRIVATE_EPG
			if ( updated )
			{
				contentMaps::iterator x =
					content_time_tables.find( DBIt->first );
				if ( x != content_time_tables.end() )
				{
					timeMap &tmMap = eventDB[DBIt->first].second;
					for ( contentMap::iterator i = x->second.begin(); i != x->second.end(); )
					{
						for ( contentTimeMap::iterator it(i->second.begin());
							it != i->second.end(); )
						{
							if ( tmMap.find(it->second.first) == tmMap.end() )
								i->second.erase(it++);
							else
								++it;
						}
						if ( i->second.size() )
							++i;
						else
							x->second.erase(i++);
					}
				}
			}
#endif

			if ( DBIt->second.second.size() == 1 )
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
//			eDebug("check NVOD Service");
			eventCache::iterator evIt(eventDB.find(it->first));
			if ( evIt != eventDB.end() && evIt->second.first.size() )
			{
				for ( eventMap::iterator i(evIt->second.first.begin());
					i != evIt->second.first.end(); )
				{
#if EPG_DEBUG
					ASSERT(i->second->getStartTime() == 3599);
#endif
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
						evIt->second.first.erase(i++);  // remove from eventmap
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
		// get plain data... the returned pointer is in eventData format !!
		// to get a eit_event_struct .. use ->get() on the eventData*
		// but take care the eit_event_struct is only valid until the next ->get() call of
		// any eventData* .. when you will use the event_data for a longer time then make
		// a copy of it.. with memcpy or by another way or use EITEvents (not plain)
				return (EITEvent*) i->second;
			else
				return new EITEvent( *i->second, (It->first.tsid<<16)|It->first.onid );
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

#ifdef NVOD
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
						EITEvent refEvt(*emIt->second, (evIt->first.tsid<<16)|evIt->first.onid);
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
										if ( i->second->getEventID() == event_id )
										{
											if ( plain )
												// get plain data... not in EITEvent Format !!!
												// before use .. cast it to eit_event_struct*
												return (EITEvent*) i->second;
											EITEvent *evt = new EITEvent( *i->second, (evIt->first.tsid<<16)|evIt->first.onid);
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
#endif

		timeMap::iterator i = It->second.second.lower_bound(t);
		if ( i != It->second.second.end() )
		{
			i--;
			if ( i != It->second.second.end() )
			{
				if ( t <= i->first+i->second->getDuration() )
				{
					if ( plain )
		// get plain data... the returned pointer is in eventData format !!
		// to get a eit_event_struct .. use ->get() on the eventData*
		// but take care the eit_event_struct is only valid until the next ->get() call of
		// any eventData* .. when you will use the event_data for a longer time then make
		// a copy of it.. with memcpy or by another way or use EITEvents (not plain)
						return (EITEvent*) i->second;
					return new EITEvent( *i->second, (It->first.tsid<<16)|It->first.onid  );
				}
			}
		}

		for ( eventMap::iterator i( It->second.first.begin() ); i != It->second.first.end(); i++)
		{
			time_t begTime = i->second->getStartTime();
			if ( t >= begTime && t <= begTime+i->second->getDuration()) // then we have found
			{
				if ( plain )
					// get plain data... not in EITEvent Format !!!
					// before use .. cast it to eit_event_struct*
					return (EITEvent*) i->second;
				return new EITEvent( *i->second, (It->first.tsid<<16)|It->first.onid );
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
		for (int i=0; i < 3; ++i)
		{
			seenSections[i].clear();
			calcedSections[i].clear();
		}
		Unlock();
		eDebug("[EPGC] start caching events(%d)", time(0)+eDVB::getInstance()->time_difference);
		state=0;
		haveData=0;
		scheduleReader.start();
		isRunning |= SCHEDULE;
		nownextReader.start();
		isRunning |= NOWNEXT;
		scheduleOtherReader.start();
		isRunning |= SCHEDULE_OTHER;
#ifdef ENABLE_MHW_EPG
		int mhwepg = 1;
		eConfig::getInstance()->getKey("/extras/mhwepg", mhwepg);
		if (mhwepg)
		{
			scheduleMhwReader.start();
			isRunning |= SCHEDULE_MHW;
		}
#endif
		abortTimer.start(7000,true);
	}
	else
	{
		eDebug("[EPGC] wait for clock update");
		zapTimer.start(1000, 1); // restart Timer
	}
}

void eEPGCache::abortNonAvail()
{
	if (!state)
	{
		if ( !(haveData&NOWNEXT) && (isRunning&NOWNEXT) )
		{
			eDebug("[EPGC] abort non avail nownext reading");
			isRunning &= ~NOWNEXT;
			nownextReader.abort();
		}
		if ( !(haveData&SCHEDULE) && (isRunning&SCHEDULE) )
		{
			eDebug("[EPGC] abort non avail schedule reading");
			isRunning &= ~SCHEDULE;
			scheduleReader.abort();
		}
		if ( !(haveData&SCHEDULE_OTHER) && (isRunning&SCHEDULE_OTHER) )
		{
			eDebug("[EPGC] abort non avail schedule_other reading");
			isRunning &= ~SCHEDULE_OTHER;
			scheduleOtherReader.abort();
		}
#ifdef ENABLE_MHW_EPG
		if ( !(haveData&SCHEDULE_MHW) && (isRunning&SCHEDULE_MHW) )
		{
			eDebug("[EPGC] abort non avail mhw schedule reading");
			isRunning &= ~SCHEDULE_MHW;
			scheduleMhwReader.abort();
		}
#endif
		if ( isRunning )
			abortTimer.start(90000, true);
		else
		{
			eDebug("[EPGC] no data .. abort");
			++state;
		}
	}
	else
		eDebug("[EPGC] timeout");
	++state;
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

	if ( thread_running() )
	// -> gotMessage -> changedService
		messages.send(Message(Message::enterService, ref, err));
	else
	{
		cached_service = ref;
		cached_err = err;
	}
}

void eEPGCache::leaveService(const eServiceReferenceDVB& ref)
{
	if ( thread_running() )
		messages.send(Message(Message::leaveService, ref));
	else
		cached_service=eServiceReferenceDVB();
	// -> gotMessage -> abortEPG
}

#ifdef ENABLE_PRIVATE_EPG
void eEPGCache::setContentPid( int pid )
{
	messages.send(Message(Message::content_pid, pid));
	// -> gotMessage -> content_pid
}
#endif

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
		if (isRunning & SCHEDULE)
		{
			isRunning &= ~SCHEDULE;
			scheduleReader.abort();
		}
		if (isRunning & NOWNEXT)
		{
			isRunning &= ~NOWNEXT;
			nownextReader.abort();
		}
		if (isRunning & SCHEDULE_OTHER)
		{
			isRunning &= ~SCHEDULE_OTHER;
			scheduleOtherReader.abort();
		}
#ifdef ENABLE_MHW_EPG
		if (isRunning & SCHEDULE_MHW)
		{
			isRunning &= ~SCHEDULE_MHW;
			scheduleMhwReader.abort();
		}
#endif
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
#ifdef ENABLE_PRIVATE_EPG
			contentReader.stop();
#endif
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
#ifdef ENABLE_PRIVATE_EPG
		case Message::content_pid:
			contentReader.start(msg.pid);
			break;
#endif
		case Message::save:
			save();
			break;
		case Message::load:
			flushEPG();
			load();
			break;
		default:
			eDebug("unhandled EPGCache Message!!");
			break;
	}
}

void eEPGCache::thread()
{
	nice(4);
	load();
	cleanLoop();
	exec();
	save();
}

void eEPGCache::load()
{
	FILE *f = fopen("/hdd/epg.dat", "r");
	if (f)
	{
		unsigned char md5_saved[16];
		unsigned char md5[16];
		int size=0;
		int cnt=0;
		bool md5ok=false;
		if (!md5_file("/hdd/epg.dat", 1, md5))
		{
			FILE *f = fopen("/hdd/epg.dat.md5", "r");
			if (f)
			{
				fread( md5_saved, 16, 1, f);
				fclose(f);
				if ( !memcmp(md5_saved, md5, 16) )
					md5ok=true;
			}
		}
		if ( md5ok )
		{
			unsigned int magic=0;
			fread( &magic, sizeof(int), 1, f);
			if (magic != 0x98765432)
			{
				eDebug("epg file has incorrect byte order.. dont read it");
				fclose(f);
				return;
			}
			char text1[13];
			fread( text1, 13, 1, f);
			if ( !strncmp( text1, "ENIGMA_EPG_V7", 13) )
			{
				singleLock l(cache_lock);
				fread( &size, sizeof(int), 1, f);
				while(size--)
				{
					uniqueEPGKey key;
					eventMap evMap;
					timeMap tmMap;
					int size=0;
					fread( &key, sizeof(uniqueEPGKey), 1, f);
					fread( &size, sizeof(int), 1, f);
					while(size--)
					{
						__u8 len=0;
						__u8 type=0;
						eventData *event=0;
						fread( &type, sizeof(__u8), 1, f);
						fread( &len, sizeof(__u8), 1, f);
						event = new eventData(0, len, type);
						event->EITdata = new __u8[len];
						eventData::CacheSize+=len;
						fread( event->EITdata, len, 1, f);
						evMap[ event->getEventID() ]=event;
						tmMap[ event->getStartTime() ]=event;
						++cnt;
					}
					eventDB[key]=std::pair<eventMap,timeMap>(evMap,tmMap);
				}
				eventData::load(f);
				eDebug("%d events read from /hdd/epg.dat", cnt);
#ifdef ENABLE_PRIVATE_EPG
				char text2[11];
				fread( text2, 11, 1, f);
				if ( !strncmp( text2, "PRIVATE_EPG", 11) )
				{
					size=0;
					fread( &size, sizeof(int), 1, f);
					while(size--)
					{
						int size=0;
						uniqueEPGKey key;
						fread( &key, sizeof(uniqueEPGKey), 1, f);
						eventMap &evMap=eventDB[key].first;
						fread( &size, sizeof(int), 1, f);
						while(size--)
						{
							int size;
							int content_id;
							fread( &content_id, sizeof(int), 1, f);
							fread( &size, sizeof(int), 1, f);
							while(size--)
							{
								time_t time1, time2;
								__u16 event_id;
								fread( &time1, sizeof(time_t), 1, f);
								fread( &time2, sizeof(time_t), 1, f);
								fread( &event_id, sizeof(__u16), 1, f);
								content_time_tables[key][content_id][time1]=std::pair<time_t, __u16>(time2, event_id);
								eventMap::iterator it =
									evMap.find(event_id);
								if (it != evMap.end())
									it->second->type = PRIVATE;
							}
						}
					}
				}
#endif // ENABLE_PRIVATE_EPG
			}
			else
				eDebug("[EPGC] don't read old epg database");
			fclose(f);
		}
	}
}

void eEPGCache::save()
{
	struct statfs s;
	off64_t tmp;
	if (statfs("/hdd", &s)<0)
		tmp=0;
	else
	{
		tmp=s.f_blocks;
		tmp*=s.f_bsize;
	}

	// prevent writes to builtin flash
	if ( tmp < 1024*1024*50 ) // storage size < 50MB
		return;

	// check for enough free space on storage
	tmp=s.f_bfree;
	tmp*=s.f_bsize;
	if ( tmp < (eventData::CacheSize*12)/10 ) // 20% overhead
		return;

	FILE *f = fopen("/hdd/epg.dat", "w");
	int cnt=0;
	if ( f )
	{
		unsigned int magic = 0x98765432;
		fwrite( &magic, sizeof(int), 1, f);
		const char *text = "ENIGMA_EPG_V7";
		fwrite( text, 13, 1, f );
		int size = eventDB.size();
		fwrite( &size, sizeof(int), 1, f );
		for (eventCache::iterator service_it(eventDB.begin()); service_it != eventDB.end(); ++service_it)
		{
			timeMap &timemap = service_it->second.second;
			fwrite( &service_it->first, sizeof(uniqueEPGKey), 1, f);
			size = timemap.size();
			fwrite( &size, sizeof(int), 1, f);
			for (timeMap::iterator time_it(timemap.begin()); time_it != timemap.end(); ++time_it)
			{
				__u8 len = time_it->second->ByteSize;
				fwrite( &time_it->second->type, sizeof(__u8), 1, f );
				fwrite( &len, sizeof(__u8), 1, f);
				fwrite( time_it->second->EITdata, len, 1, f);
				++cnt;
			}
		}
		eDebug("%d events written to /hdd/epg.dat", cnt);
		eventData::save(f);
#ifdef ENABLE_PRIVATE_EPG
		const char* text3 = "PRIVATE_EPG";
		fwrite( text3, 11, 1, f );
		size = content_time_tables.size();
		fwrite( &size, sizeof(int), 1, f);
		for (contentMaps::iterator a = content_time_tables.begin(); a != content_time_tables.end(); ++a)
		{
			contentMap &content_time_table = a->second;
			fwrite( &a->first, sizeof(uniqueEPGKey), 1, f);
			int size = content_time_table.size();
			fwrite( &size, sizeof(int), 1, f);
			for (contentMap::iterator i = content_time_table.begin(); i != content_time_table.end(); ++i )
			{
				int size = i->second.size();
				fwrite( &i->first, sizeof(int), 1, f);
				fwrite( &size, sizeof(int), 1, f);
				for ( contentTimeMap::iterator it(i->second.begin());
					it != i->second.end(); ++it )
				{
					fwrite( &it->first, sizeof(time_t), 1, f);
					fwrite( &it->second.first, sizeof(time_t), 1, f);
					fwrite( &it->second.second, sizeof(__u16), 1, f);
				}
			}
		}
#endif
		fclose(f);
		unsigned char md5[16];
		if (!md5_file("/hdd/epg.dat", 1, md5))
		{
			FILE *f = fopen("/hdd/epg.dat.md5", "w");
			if (f)
			{
				fwrite( md5, 16, 1, f);
				fclose(f);
			}
		}
	}
}

eAutoInitP0<eEPGCache> init_eEPGCacheInit(eAutoInitNumbers::service+3, "EPG cache");

#ifdef ENABLE_MHW_EPG
void eScheduleMhw::cleanup()
{
	channels.clear();
	themes.clear();
	titles.clear();
	program_ids.clear();
}

__u8 *eScheduleMhw::delimitName( __u8 *in, __u8 *out, int len_in )
{
	// Names in mhw structs are not strings as they are not '\0' terminated.
	// This function converts the mhw name into a string.
	// Constraint: "length of out" = "length of in" + 1.
	int i;
	for ( i=0; i < len_in; i++ )
		out[i] = in[i];
	
	i = len_in - 1;
	while ( ( i >=0 ) && ( out[i] == 0x20 ) )
		i--;
	
	out[i+1] = 0;
	return out;
}

void eScheduleMhw::timeMHW2DVB( u_char hours, u_char minutes, u_char *return_time)
// For time of day
{
	return_time[0] = toBCD( hours );
	return_time[1] = toBCD( minutes );
	return_time[2] = 0;
}

void eScheduleMhw::timeMHW2DVB( int minutes, u_char *return_time)
{
	timeMHW2DVB( int(minutes/60), minutes%60, return_time );
}

void eScheduleMhw::timeMHW2DVB( u_char day, u_char hours, u_char minutes, u_char *return_time)
// For date plus time of day
{
	// Remove offset in mhw time.
	__u8 local_hours = hours;
	if ( hours >= 16 )
		local_hours -= 4;
	else if ( hours >= 8 )
		local_hours -= 2;
	
	// As far as we know all mhw time data is sent in central Europe time zone.
	// So, temporarily set timezone to western europe 
	char *old_tz = getenv( "TZ" );
	putenv("TZ=CET-1CEST,M3.5.0/2,M10.5.0/3");
	tzset();
	
	time_t dt = time(0)+eDVB::getInstance()->time_difference;
	tm localnow;
	localtime_r( &dt, &localnow );
	if (day == 7)
		day = 0;
	if ( day + 1 < localnow.tm_wday )		// day + 1 to prevent old events to show for next week.
		day += 7;
	if (local_hours <= 5)
		day++;
	
	dt += 3600*24*(day - localnow.tm_wday);	// Shift dt to the recording date (local time zone).
	dt += 3600*(local_hours - localnow.tm_hour);  // Shift dt to the recording hour.

	tm recdate;
	gmtime_r( &dt, &recdate );   // This will also take care of DST.
	
	// Calculate MJD according to annex in ETSI EN 300 468
	int l=0;
	if ( recdate.tm_mon <= 1 )	// Jan or Feb
		l=1;
	int mjd = 14956 + recdate.tm_mday + int( (recdate.tm_year - l) * 365.25) + 
		int( (recdate.tm_mon + 2 + l * 12) * 30.6001);
	
	return_time[0] = (mjd & 0xFF00)>>8;
	return_time[1] = mjd & 0xFF;

	if ( old_tz == NULL )
		unsetenv( "TZ" );
	else
		putenv( old_tz );
		
	tzset();

	timeMHW2DVB( recdate.tm_hour, minutes, return_time+2 );
}

void eScheduleMhw::storeTitle(std::map<__u32, mhw_title_t>::iterator itTitle, eString sumText, __u8 *data)
// data is borrowed from calling proc to save memory space.
{
	// For each title a separate EIT packet will be sent to eEPGCache::sectionRead()
	__u8 name[24];
					
	eit_t *packet = (eit_t *) data;
	packet->table_id = 0x50;
	packet->section_syntax_indicator = 1;
	packet->service_id_hi = channels[ itTitle->second.channel_id - 1 ].channel_id_hi;
	packet->service_id_lo = channels[ itTitle->second.channel_id - 1 ].channel_id_lo;
	packet->version_number = 0;	// eEPGCache::sectionRead() will dig this for the moment
	packet->current_next_indicator = 0;
	packet->section_number = 0;	// eEPGCache::sectionRead() will dig this for the moment
	packet->last_section_number = 0;	// eEPGCache::sectionRead() will dig this for the moment
	packet->transport_stream_id_hi = channels[ itTitle->second.channel_id - 1 ].transport_stream_id_hi;
	packet->transport_stream_id_lo = channels[ itTitle->second.channel_id - 1 ].transport_stream_id_lo;
	packet->original_network_id_hi = channels[ itTitle->second.channel_id - 1 ].network_id_hi;
	packet->original_network_id_lo = channels[ itTitle->second.channel_id - 1 ].network_id_lo;
	packet->segment_last_section_number = 0; // eEPGCache::sectionRead() will dig this for the moment
	packet->segment_last_table_id = 0x50;
	
	eString prog_title = (char *) delimitName( itTitle->second.title, name, 23 );
	int prog_title_length = prog_title.length();

	int packet_length = EIT_SIZE + EIT_LOOP_SIZE + EIT_SHORT_EVENT_DESCRIPTOR_SIZE +
		prog_title_length + 1;

	eit_event_t *event_data = (eit_event_t *) (data + EIT_SIZE);
	event_data->event_id_hi = (( itTitle->first ) >> 8 ) & 0xFF;
	event_data->event_id_lo = ( itTitle->first ) & 0xFF;

	timeMHW2DVB( itTitle->second.day, itTitle->second.hours, itTitle->second.minutes, 
		(u_char *) event_data + 2 );
	timeMHW2DVB( HILO(itTitle->second.duration), (u_char *) event_data+7 );
			
	event_data->running_status = 0;
	event_data->free_CA_mode = 0;
	int descr_ll = EIT_SHORT_EVENT_DESCRIPTOR_SIZE + 1 + prog_title_length;

	eit_short_event_descriptor_struct *short_event_descriptor =
		(eit_short_event_descriptor_struct *) ( (u_char *) event_data + EIT_LOOP_SIZE);
	short_event_descriptor->descriptor_tag = EIT_SHORT_EVENT_DESCRIPTOR;
	short_event_descriptor->descriptor_length = EIT_SHORT_EVENT_DESCRIPTOR_SIZE + 
		prog_title_length - 1;
	short_event_descriptor->language_code_1 = 'e';
	short_event_descriptor->language_code_2 = 'n';
	short_event_descriptor->language_code_3 = 'g';
	short_event_descriptor->event_name_length = prog_title_length;
	delimitName( itTitle->second.title, name, 23 );
	u_char *event_name = (u_char *) short_event_descriptor + EIT_SHORT_EVENT_DESCRIPTOR_SIZE;
	for ( int i = 0; i < prog_title_length; i++ )
		event_name[i] = name[i];

	// Set text length
	event_name[prog_title_length] = 0;

	if ( sumText.length() > 0 )
	// There is summary info
	{
		unsigned int sum_length = sumText.length();
		if ( sum_length + short_event_descriptor->descriptor_length <= 0xff )
		// Store summary in short event descriptor
		{
			// Increase all relevant lengths
			event_name[prog_title_length] = sum_length;
			short_event_descriptor->descriptor_length += sum_length;
			packet_length += sum_length;
			descr_ll += sum_length;
			sumText.copy( (char *) event_name+prog_title_length+1, sum_length );
		}
		else
		// Store summary in extended event descriptors
		{
			int remaining_sum_length = sumText.length();
			int nbr_descr = int(remaining_sum_length/247) + 1;
			for ( int i=0; i < nbr_descr; i++)
			// Loop once per extended event descriptor
			{
				eit_extended_descriptor_struct *ext_event_descriptor = (eit_extended_descriptor_struct *) (data + packet_length);
				sum_length = remaining_sum_length > 247 ? 247 : remaining_sum_length;
				remaining_sum_length -= sum_length;
				packet_length += 8 + sum_length;
				descr_ll += 8 + sum_length;
					
				ext_event_descriptor->descriptor_tag = EIT_EXTENDED_EVENT_DESCRIPOR;
				ext_event_descriptor->descriptor_length = sum_length + 6;
				ext_event_descriptor->descriptor_number = i;
				ext_event_descriptor->last_descriptor_number = nbr_descr - 1;
				ext_event_descriptor->iso_639_2_language_code_1 = 'e';
				ext_event_descriptor->iso_639_2_language_code_2 = 'n';
				ext_event_descriptor->iso_639_2_language_code_3 = 'g';
				u_char *the_text = (u_char *) ext_event_descriptor + 8;
				the_text[-2] = 0;
				the_text[-1] = sum_length;
				sumText.copy( (char *) the_text, sum_length, sumText.length() - sum_length - remaining_sum_length );
			}
		}
	}
	// Add content descriptor
	u_char *descriptor = (u_char *) data + packet_length;
	packet_length += 4;
	descr_ll += 4;
		
	int content_id = 0;
	eString content_descr = (char *) delimitName( themes[itTitle->second.theme_id].name, name, 15 );
	if ( content_descr.find( "FILM" ) != eString::npos )
		content_id = 0x10;
	else if ( content_descr.find( "SPORT" ) != eString::npos )
		content_id = 0x40;

	descriptor[0] = 0x54;
	descriptor[1] = 2;
	descriptor[2] = content_id;
	descriptor[3] = 0;
		
	event_data->descriptors_loop_length_hi = (descr_ll & 0xf00)>>8;
	event_data->descriptors_loop_length_lo = (descr_ll & 0xff);

	packet->section_length_hi =  ((packet_length - 3)&0xf00)>>8;
	packet->section_length_lo =  (packet_length - 3)&0xff;

	eEPGCache *e = eEPGCache::getInstance();
	e->sectionRead( data, eEPGCache::SCHEDULE_MHW );  // Feed the data to eEPGCache::sectionRead()
}

int eScheduleMhw::sectionRead(__u8 *data)
{
	eEPGCache *cache = eEPGCache::getInstance();

	if ( cache->state > 1 )
		return -ECANCELED;
	
	if ( ( pid == 0xD3 ) && ( tableid == 0x91 ) )
	// Channels table
	{
		int len = ((data[1]&0xf)<<8) + data[2] - 1;
		int record_size = sizeof( mhw_channel_name_t );
		int nbr_records = int (len/record_size);
		
		for ( int i = 0; i < nbr_records; i++ )
		{
			mhw_channel_name_t *channel = (mhw_channel_name_t*) &data[4 + i*record_size];
			channels.push_back( *channel );
		}

		cache->haveData |= eEPGCache::SCHEDULE_MHW;
	}
	else if ( ( pid == 0xD3 ) && ( tableid == 0x92 ) )
	// Themes table
	{
		int len = ((data[1]&0xf)<<8) + data[2] - 16;
		int record_size = sizeof( mhw_theme_name_t );
		int nbr_records = int (len/record_size);
		int idx_ptr = 0;
		__u8 next_idx = (__u8) *(data + 3 + idx_ptr);
		__u8 idx = 0;
		__u8 sub_idx = 0;
		for ( int i = 0; i < nbr_records; i++ )
		{
			mhw_theme_name_t *theme = (mhw_theme_name_t*) &data[19 + i*record_size];
			if ( i >= next_idx )
			{
				idx = (idx_ptr<<4);
				idx_ptr++;
				next_idx = (__u8) *(data + 3 + idx_ptr);
				sub_idx = 0;
			}
			else
				sub_idx++;
			
			themes[idx+sub_idx] = *theme;
		}
	}
	else if ( ( pid == 0xD2 ) && ( tableid == 0x90 ) )
	// Titles table
	{
		mhw_title_t *title = (mhw_title_t*) data;
		
		if ( title->channel_id == 0xFF )	// Separator
			return 0;	// Continue reading of the current table.
		else
		{
			// Create unique key per title
			__u32 title_id = ((title->channel_id)<<16)|((title->day)<<13)|((title->hours)<<8)|
				(title->minutes);
			__u32 program_id = ((title->program_id_hi)<<24)|((title->program_id_mh)<<16)|
				((title->program_id_ml)<<8)|(title->program_id_lo);
			
			if ( titles.find( title_id ) == titles.end() )
			{
				titles[ title_id ] = *title;
				if ( (title->summary_available) && (program_ids.find(program_id) == program_ids.end()) )
					// program_ids will be used to gather summaries.
					program_ids[ program_id ] = title_id;
				return 0;	// Continue reading of the current table.
			}
		}
	}
	else if (( pid == 0xD3 ) && ( tableid == 0x90 ))
	// Summaries table
	{
		mhw_summary_t *summary = (mhw_summary_t*) data;
		
		// Create unique key per record
		__u32 program_id = ((summary->program_id_hi)<<24)|((summary->program_id_mh)<<16)|
			((summary->program_id_ml)<<8)|(summary->program_id_lo);
		int len = ((data[1]&0xf)<<8) + data[2];
		data[len+3] = 0;	// Terminate as a string.
		std::map<__u32, __u32>::iterator itProgid( program_ids.find( program_id ) );
		if ( itProgid == program_ids.end() )
		{ /*	This part is to prevent to looping forever if some summaries are not received yet.
			There is a timeout of 4 sec. after the last successfully read summary. */
			
			if ( !program_ids.empty() && 
				time(0)+eDVB::getInstance()->time_difference - tnew_summary_read <= 4 )
				return 0;	// Continue reading of the current table.
		}
		else
		{
			tnew_summary_read = time(0)+eDVB::getInstance()->time_difference;
			eString the_text = (char *) (data + 11 + summary->nb_replays * 7);
			the_text.strReplace( "\r\n", " " );
			
			// Find corresponding title, store title and summary in epgcache.
			std::map<__u32, mhw_title_t>::iterator itTitle( titles.find( itProgid->second ) );
			if ( itTitle != titles.end() )
			{
				storeTitle( itTitle, the_text, data );
				titles.erase( itTitle );
			}
			
			program_ids.erase( itProgid );
			if ( !program_ids.empty() )
				return 0;	// Continue reading of the current table.
		}
	}
	return -EAGAIN;
}

void eScheduleMhw::sectionFinish(int err)
{
	eEPGCache *e = eEPGCache::getInstance();
	if (e->isRunning & eEPGCache::SCHEDULE_MHW)
	{
		if ( ( pid == 0xD3 ) && ( tableid == 0x91 ) && ( err == -EAGAIN ) )
		{
			// Channels table has been read, start reading the themes table.
			setFilter( 0xD3, 0x92, -1, -1, SECREAD_NOTIMEOUT, 0xFF );
			return;
		}
		if ( ( pid == 0xD3 ) && ( tableid == 0x92 ) && ( err == -EAGAIN ) )
		{
			// Themes table has been read, start reading the titles table.
			setFilter( 0xD2, 0x90, -1, -1, SECREAD_NOTIMEOUT, 0xFF );
			return;
		}
		if ( ( pid == 0xD2 ) && ( tableid == 0x90 ) && ( err == -EAGAIN ) && ( !program_ids.empty() ) )
		{
			// Titles table has been read, there are summaries to read.
			// Start reading summaries, store corresponding titles on the fly.
			tnew_summary_read = time(0)+eDVB::getInstance()->time_difference;
			setFilter( 0xD3, 0x90, -1, -1, SECREAD_NOTIMEOUT, 0xFF );
			return;
		}
		if ( err == -EAGAIN )
		{
			// Summaries have been read, titles that have summaries have been stored.
			// Now store titles that do not have summaries.
			__u8 data[65];
			for (std::map<__u32, mhw_title_t>::iterator itTitle(titles.begin()); itTitle != titles.end(); itTitle++)
			{
				storeTitle( itTitle, "", data );
			}
		}
		eDebug("[EPGC] stop schedule mhw");
		e->isRunning &= ~eEPGCache::SCHEDULE_MHW;
		if (e->haveData)
			e->finishEPG();
	}
	cleanup();
}
#endif
