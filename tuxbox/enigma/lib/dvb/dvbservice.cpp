#include <lib/dvb/dvbservice.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <lib/dvb/si.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/iso639.h>
#ifndef DISABLE_CI
	#include <lib/dvb/dvbci.h>
#endif
#include <lib/dvb/service.h>
#include <lib/dvb/eaudio.h>
#include <lib/system/info.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/record.h>
#include <lib/dvb/subtitling.h>
#include <sstream>
#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#define DEMUX1_DEV "/dev/dvb/card0/demux1"
#else
#include <linux/dvb/dmx.h>
#define DEMUX1_DEV "/dev/dvb/adapter0/demux1"
#endif

std::set<eDVBCaPMTClient*> eDVBCaPMTClientHandler::capmtclients;

pthread_mutex_t eDVBServiceController::availCALock =
	PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP;

PMTEntry *priorityAudio(PMTEntry *audio)
{
	PMTEntry *audio2 = audio;
	audio = 0;

	char *audiochannelspriority = 0;
	eConfig::getInstance()->getKey("/extras/audiochannelspriority", audiochannelspriority);

	eString audiochannel;

	if (audiochannelspriority)
	{
//		printf(">>>%s\n", audiochannelspriority);
		std::stringstream audiochannels;
		eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
		if (sapi)
		{
			std::list<eDVBServiceController::audioStream> &astreams(sapi->audioStreams);
			audiochannels.clear();
			audiochannels.str(eString(audiochannelspriority));
			while (audiochannels && audio == 0)
			{
				audiochannels >> audiochannel;
				for (std::list<eDVBServiceController::audioStream>::iterator it(astreams.begin())
					;it != astreams.end(); ++it)
				{
//					printf("comparing:%s:%s:\n", audiochannel.c_str(), it->text.c_str());
					if (audiochannel == it->text)
					{
						audio = it->pmtentry;
						break;
					}
				}
			}
		}
		free(audiochannelspriority);
	}
	if (audio == 0)
		audio = audio2;
//	printf("returning:%d\n", audio->elementary_PID);
	return audio;
}

eString getISO639Description(char *iso)
{
	for (unsigned int i=0; i<sizeof(iso639)/sizeof(*iso639); ++i)
	{
		if (!strncasecmp(iso639[i].iso639foreign, iso, 3))
			return iso639[i].description1;
		if (!strncasecmp(iso639[i].iso639int, iso, 3))
			return iso639[i].description1;
	}
	return eString()+iso[0]+iso[1]+iso[2];
}

eDVBServiceController::eDVBServiceController(eDVB &dvb)
: eDVBController(dvb)
, updateTDTTimer(eApp)
#ifndef DISABLE_CI
, DVBCI(dvb.DVBCI)
, DVBCI2(dvb.DVBCI2)
#endif
{
	CONNECT(updateTDTTimer.timeout, eDVBServiceController::startTDT);
	CONNECT(dvb.tPAT.tableReady, eDVBServiceController::PATready);
	CONNECT(dvb.tPMT.tableReady, eDVBServiceController::PMTready);
	CONNECT(dvb.tSDT.tableReady, eDVBServiceController::SDTready);
	CONNECT(dvb.tEIT.tableReady, eDVBServiceController::EITready);

	initCAlist();

	transponder=0;
	tdt=0;
	tMHWEIT=0;
	calist.setAutoDelete(true);

	eDVBCaPMTClientHandler::registerCaPMTClient(this);  // static method...
}

eDVBServiceController::~eDVBServiceController()
{
	delete tdt;
	Decoder::Flush();
	eDVBCaPMTClientHandler::unregisterCaPMTClient(this);  // static method...
}

void eDVBServiceController::handleEvent(const eDVBEvent &event)
{
#ifdef PROFILE
	static timeval last_event;

	timeval now;
	gettimeofday(&now, 0);

	int diff=(now.tv_sec-last_event.tv_sec)*1000000+(now.tv_usec-last_event.tv_usec);
	last_event=now;

	char *what="unknown";

	switch (event.type)
	{
	case eDVBServiceEvent::eventTunedIn: what="eventTunedIn"; break;  
	case eDVBServiceEvent::eventServiceSwitch: what="ServiceSwitch"; break;
	case eDVBServiceEvent::eventServiceTuneOK: what="TuneOK"; break;
	case eDVBServiceEvent::eventServiceTuneFailed: what="TuneFailed"; break;
	case eDVBServiceEvent::eventServiceGotPAT: what="GotPAT"; break;
	case eDVBServiceEvent::eventServiceGotPMT: what="GotPMT"; break;
	case eDVBServiceEvent::eventServiceNewPIDs: what="NewPIDs"; break;
	case eDVBServiceEvent::eventServiceGotSDT: what="GotSDT"; break;
	case eDVBServiceEvent::eventServiceSwitched: what="Switched"; break;
	case eDVBServiceEvent::eventServiceFailed: what="Failed"; break;
	default: { static char bug[100]; sprintf(bug, "%d", event.type); what=bug; }
	}

	eDebug("[PROFILE] [%s] +%dus", what, diff);
#endif
	switch (event.type)
	{
	case eDVBEvent::eventTunedIn:
		if (transponder==event.transponder)
			dvb.event(eDVBServiceEvent(event.err?eDVBServiceEvent::eventServiceTuneFailed: eDVBServiceEvent::eventServiceTuneOK, event.err, event.transponder));
		break;
	case eDVBServiceEvent::eventServiceSwitch:
	{
		if (!service.path.size()) // a normal service, not a replay
		{
			if (!dvb.settings->getTransponders())
			{
				eDebug("no tranponderlist");
				service_state=ENOENT;
				dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
				return;
			}
			eTransponder *n=dvb.settings->getTransponders()->searchTS(service.getDVBNamespace(), service.getTransportStreamID(), service.getOriginalNetworkID());
			if (!n)
			{
				eDebug("no transponder %x %x", service.getOriginalNetworkID().get(), service.getTransportStreamID().get());
				dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceTuneFailed));
				break;
			}
			if ( !(n->state&eTransponder::stateOK) )
			{
				eDebug("couldn't tune (state is %x)", n->state);
				service_state=ENOENT;
				dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
				return;
			}

			if (n==transponder)
			{
//				dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceTune));
				dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceTuneOK, 0, n));
			} else
			{
				/*emit*/ dvb.leaveTransponder(transponder);
				transponder=n;
				if (n->tune())
					dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceTuneFailed));
				else
					dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceTune));
			}
			eDebug("<-- tuned");
		} else
		{
			eDebug("won't tune, since its a replay.");
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceTuneOK, 0, 0));
		}
		break;
	}
	case eDVBServiceEvent::eventServiceTuneOK:
	{
//		eDebug("apid = %04x, vpid = %04x, pcrpid = %04x, tpid = %04x", Decoder::current.apid, Decoder::current.vpid, Decoder::current.pcrpid, Decoder::current.tpid );
		/*emit*/ dvb.enterTransponder(event.transponder);
		int nopmt=0;

		int spSID=-1;
		// do we haved fixed or cached PID values?
		eService *sp=eServiceInterface::getInstance()->addRef(service);
		if (sp)
		{
			if (sp->dvb)
			{
// VPID
				Decoder::parms.vpid=sp->dvb->get(eServiceDVB::cVPID);
// AC3PID
				int tmp = sp->dvb->get(eServiceDVB::cAC3PID);
				if ( tmp != -1)
				{
					Decoder::parms.audio_type=DECODE_AUDIO_AC3;
					Decoder::parms.apid=tmp;
				}
/* APID*/ else
				{
					tmp = sp->dvb->get(eServiceDVB::cAPID);
					if ( tmp != -1)
					{
						Decoder::parms.audio_type=DECODE_AUDIO_MPEG;
						Decoder::parms.apid=tmp;
					}
				}
// TPID
				Decoder::parms.tpid=sp->dvb->get(eServiceDVB::cTPID);
// PCRPID ... do not set on recorded streams..
				tmp = sp->dvb->get(eServiceDVB::cPCRPID);
				if ( tmp != -1 && !service.path.length() )
					Decoder::parms.pcrpid=tmp;
// start yet...
				Decoder::Set();

				if (sp->dvb->dxflags & eServiceDVB::dxNoPMT)
					nopmt=1;

				spSID=sp->dvb->service_id.get();
			}
			eServiceInterface::getInstance()->removeRef(service);
		}

		if ( service.path )  // replay ?
		{
			if ( !service.getServiceID().get() && spSID != -1 )
				service.data[1] = spSID;

			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPAT));
			dvb.tPAT.start(new PAT());
			eServiceInterface::getInstance()->removeRef(service);
			break;
		}
		if (!nopmt && service.getServiceID().get() ) // if not a dvb service, don't even try to search a PAT, PMT etc.
		{
// workaround for zap in background before recordings
			if ( Decoder::locked == 2 && !dvb.recorder )
			{
				eDebug("start PAT on demux1");
				dvb.tPAT.start(new PAT(), DEMUX1_DEV);
			}
			else
			{
				eDebug("start PAT on demux0");
				dvb.tPAT.start(new PAT());
			}
		}
		if (nopmt || ( service.path.size() && !service.getServiceID().get() ) )
		{
			dvb.tEIT.start(new EIT(EIT::typeNowNext, service.getServiceID().get(), EIT::tsActual));
			service_state=0;
			/*emit*/ dvb.enterService(service);
			/*emit*/ dvb.switchedService(service, -service_state);
			dvb.setState(eDVBServiceState(eDVBServiceState::stateIdle));
			break;
		}

		if (tMHWEIT)
		{
			delete tMHWEIT;
			tMHWEIT=0;
		}

		startTDT();

		switch (service.getServiceType())
		{
		case 1:	// digital television service
		case 2:	// digital radio service
		case 3:	// teletext service
			dvb.tEIT.start(new EIT(EIT::typeNowNext, service.getServiceID().get(), EIT::tsActual));
		case 5:	// NVOD time shifted service ( faked )
		case 6:	// mosaic service
			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPAT));
			dvb.tPAT.get();
			break;
		case 4:	// NVOD reference service
			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetSDT));
			dvb.tEIT.start(new EIT(EIT::typeNowNext, service.getServiceID().get(), EIT::tsActual));
			break;
		case 7: // linkage ( faked )
			// start parentEIT
			dvb.tEIT.start(new EIT(EIT::typeNowNext, parentservice.getServiceID().get(),
				( (service.getTransportStreamID()==parentservice.getTransportStreamID())
				&&(service.getOriginalNetworkID()==parentservice.getOriginalNetworkID())) ? EIT::tsActual:EIT::tsOther ));
		case -1: // data
			dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPAT));
			dvb.tPAT.get();
			break;
		default:
			service_state=ENOSYS;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
			break;
		}
		break;
	}
	case eDVBServiceEvent::eventServiceTuneFailed:
		eDebug("[TUNE] tune failed");
		service_state=ENOENT;
		dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
//		transponder=0;
		break;
	case eDVBServiceEvent::eventServiceGotPAT:
	{
		int pmtpid=-1;
		if (dvb.getState() != eDVBServiceState::stateServiceGetPAT)
			break;

		if ( !service.path )
			dvb.tSDT.start(new SDT());

		PAT *pat=dvb.tPAT.getCurrent();
		PATEntry *pe=pat->searchService(service.getServiceID().get());
		if (!pe)
		{
#ifndef DISABLE_FILE
			if ( service.path ) // recorded ts
			{
				// we try to find manual the correct sid
				int fd = open( service.path.c_str(), O_RDONLY );
				if ( fd < 0 )
					eDebug("open %s failed");
				else
				{
					eDebug("parse ts file for find the correct pmtpid");
					unsigned char *buf = new unsigned char[256*1024]; // 256 kbyte
					int rd=0;
					while ( pmtpid == -1 && (rd < 1024*1024*5) )
					{
						std::set<int> pids;
						int r = ::read( fd, buf, 256*1024 );
						if ( r <= 0 )
							break;
						rd+=r;
						int cnt=0;
						while(cnt < r)
						{
							while ( (buf[cnt] != 0x47) && ((cnt+188) < r) && (buf[cnt+188] != 0x47) )
							{
//								eDebug("search sync byte %02x %02x, %d %d", buf[cnt], buf[cnt+188], cnt+188, r);
								cnt++;
							}
							if ( buf[cnt] == 0x47 )
							{
								int pid = ((buf[cnt+1]&0x3F) << 8) | buf[cnt+2];
//								eDebug("addpid %d", pid);
								pids.insert(pid);
								cnt+=188;
							}
							else
								break;
						}
						for( std::set<int>::iterator it(pids.begin()); pmtpid==-1 && it != pids.end(); ++it )
						{
							for ( ePtrList<PATEntry>::iterator i(pat->entries); i != pat->entries.end(); ++i)
								if ( i->program_map_PID == *it )
								{
									pmtpid = *it;
									service.data[1] = i->program_number;
									break;
								}
						}
					}
					delete [] buf;
					close(fd);
				}
			}
#endif
		}
		else
			pmtpid=pe->program_map_PID;
		pat->unlock();
		if (pmtpid==-1)
		{
			eDebug("[PAT] no pat entry");
			service_state=ENOENT;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
			return;
		}
		dvb.setState(eDVBServiceState(eDVBServiceState::stateServiceGetPMT));

// workaround for zap in background before recordings
		if ( Decoder::locked == 2 && !service.path && !dvb.recorder )
		{
			eDebug("start PMT on demux1");
			dvb.tPMT.start(new PMT(pmtpid, service.getServiceID().get()), DEMUX1_DEV );
		}
		else
		{
			eDebug("start PMT on demux0");
			dvb.tPMT.start(new PMT(pmtpid, service.getServiceID().get()));
		}

		break;
	}	
	case eDVBServiceEvent::eventServiceGotPMT:
	{
		service_state=0;
		PMT *pmt=dvb.tPMT.ready()?dvb.tPMT.getCurrent():0;
		if (pmt)
		{
			scanPMT(pmt);

			if ( !service.path )
				eDVBCaPMTClientHandler::distribute_gotPMT(service, pmt);

			/*emit*/ dvb.gotPMT(pmt);
			pmt->unlock();
		}
		if (dvb.getState()==eDVBServiceState::stateServiceGetPMT)
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceSwitched));
		else
			eDebug("nee, doch nicht (state ist %d)", (int)dvb.getState());
		break;
	}
	case eDVBServiceEvent::eventServiceGotSDT:
	{
		if (dvb.getState() != eDVBServiceState::stateServiceGetSDT)
			break;

		SDT *sdt=dvb.tSDT.ready()?dvb.tSDT.getCurrent():0;
		if (sdt)
		{
			dvb.setState(eDVBServiceState(eDVBServiceState::stateIdle));
			/*emit*/ dvb.gotSDT(sdt);
			sdt->unlock();
			if (service.getServiceType()==4)
				service_state=ENVOD;
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceSwitched));
		} else
			dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceFailed));
		break;
	}
	case eDVBServiceEvent::eventServiceNewPIDs:
		Decoder::Set();
		break;
	case eDVBServiceEvent::eventServiceSwitched:
#ifndef DISABLE_FILE
		if ( service.path )
		{
			eString filename = service.path;
			filename.erase(filename.length()-2, 2);
			filename+="eit";
			int fd = ::open( filename.c_str(), O_RDONLY );
			if ( fd > -1 )
			{
				__u8 buf[4096];
				int rd = ::read(fd, buf, 4096);
				::close(fd);
				if ( rd > 12 /*EIT_LOOP_SIZE*/ )
				{
					EITEvent *evt = new EITEvent( (eit_event_struct*)buf );
					EIT *e=new EIT();
					evt->free_CA_mode=0;
					e->ts=EIT::tsFaked;
					e->type=EIT::typeNowNext;
					e->version_number=0;
					e->current_next_indicator=0;
					e->transport_stream_id=service.getTransportStreamID().get();
					e->original_network_id=service.getOriginalNetworkID().get();
					e->events.push_back(evt);
					e->ready=1;
					dvb.tEIT.inject(e);
				}
			}
		}
#endif
		/*emit*/ dvb.enterService(service);
	case eDVBServiceEvent::eventServiceFailed:
		/*emit*/ dvb.switchedService(service, -service_state);
		dvb.setState(eDVBServiceState(eDVBServiceState::stateIdle));
		break;
	}
}

void eDVBServiceController::PATready(int error)
{
	eDebug("PATready (%d)",error);
	dvb.event(eDVBServiceEvent(error?eDVBServiceEvent::eventServiceFailed:eDVBServiceEvent::eventServiceGotPAT));
}

void eDVBServiceController::SDTready(int error)
{
	eDebug("SDTready (%d)", error);
	dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceGotSDT));
	if (dvb.settings->getTransponders())
	{
		SDT *sdt=dvb.tSDT.ready()?dvb.tSDT.getCurrent():0;
		if (sdt)
		{
			if ( transponder->state & eTransponder::stateOnlyFree )
				dvb.settings->getTransponders()->handleSDT(sdt, service.getDVBNamespace(), -1, -1, &freeCheckFinishedCallback );
			else
				dvb.settings->getTransponders()->handleSDT(sdt, service.getDVBNamespace());
			sdt->unlock();
		}
	}
}

void eDVBServiceController::freeCheckFinished()
{
	eDebug("freeCheckFinished");
}

void eDVBServiceController::PMTready(int error)
{
	eDebug("PMTready (%d)", error);
	dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceGotPMT));
}

void eDVBServiceController::EITready(int error)
{
	eDebug("EITready (%d)", error);
	if (!error)
	{
		EIT *eit=dvb.getEIT();

		for (ePtrList<EITEvent>::iterator e(eit->events); e != eit->events.end(); ++e)
		{
//		eDebug("running_status = %d", e->running_status );
			if ((e->running_status>=2) || (!e->running_status) ) // currently running service
			{
				for (ePtrList<Descriptor>::iterator d(e->descriptor); d != e->descriptor.end(); ++d)
				{
					if (d->Tag()==DESCR_COMPONENT)
					{
						for (std::list<audioStream>::iterator it(audioStreams.begin())
							;it != audioStreams.end(); ++it )
						{
							if (((ComponentDescriptor*)*d)->component_tag == it->component_tag )
							{
								eString tmp = ((ComponentDescriptor*)*d)->text;
								if (tmp)
								{
									it->text=tmp;
									if ( it->isAC3 )
										it->text+=" (AC3)";
									else if ( it->isDTS )
										it->text+=" (DTS)";
								}
							}
						}
					}
				}
				break;
			}
		}

		if ( service.getServiceType() == 4 ) // NVOD Service
		{
			delete dvb.parentEIT;
			dvb.parentEIT = new EIT( eit );
			dvb.parentEIT->events.setAutoDelete(true);
			eit->events.setAutoDelete(false);
		}
		/*emit*/ dvb.gotEIT(eit, 0);
		eit->unlock();
	}
	else
		/*emit*/ dvb.gotEIT(0, error);
}

void eDVBServiceController::TDTready(int error)
{
	eDebug("TDTready %d", error);
	// receive new TDT every 90 minutes
	updateTDTTimer.start(90*60*1000,true);
	if (!error && transponder)
	{
		std::map<tsref,int> &tOffsMap = eTransponderList::getInstance()->TimeOffsetMap;
		std::map< tsref, int >::iterator it( tOffsMap.find( *transponder ) );

		time_t nowTime=time(0)+dvb.time_difference;
		int enigma_diff = tdt->UTC_time-nowTime;

		if ( dvb.time_difference )  // ref time ready?
		{
			// curTime is our current reference time....
			time_t TPTime = nowTime+enigma_diff;

			// difference between reference time (current enigma time) 
			// and the transponder time
			int diff = nowTime-TPTime;
			eDebug("[TIME] diff is %d", diff);

			if ( abs(diff) < 120 )
			{
				eDebug("[TIME] diff < 120 .. use Transponder Time");
				tOffsMap[*transponder] = 0;
				dvb.time_difference = enigma_diff;
			}
			else if ( it != tOffsMap.end() ) // correction saved?
			{
				eDebug("[TIME] we have correction %d", it->second);
				time_t CorrectedTpTime = TPTime+it->second;
				int ddiff = nowTime-CorrectedTpTime;
				eDebug("[TIME] diff after add correction is %d", ddiff);
				if ( abs(ddiff) < 120 )
				{
					eDebug("[TIME] use stored correction");
					dvb.time_difference = enigma_diff + it->second;
				}
				else  // big change in calced correction..
				{
					eDebug("[TIME] update stored correction to %d", diff);
					tOffsMap[*transponder] = diff;
				}
			}
			else
			{
				eDebug("[TIME] no correction found... store calced correction(%d)",diff);
				tOffsMap[*transponder] = diff;
			}
		}
		else  // no time setted yet
		{
			if ( it != tOffsMap.end() )
			{
				enigma_diff += it->second;
				eDebug("[TIME] we have correction (%d)... use", it->second );
			}
			else
				eDebug("[TIME] dont have correction.. set Transponder Diff");
			dvb.time_difference=enigma_diff;
		}

		eDebug("[TIME] time_difference is %d", dvb.time_difference );
		time_t t = nowTime+dvb.time_difference;
		tm now = *localtime(&t);
		eDebug("[TIME] time update to %02d:%02d:%02d",
			now.tm_hour,
			now.tm_min,
			now.tm_sec);

		if ( abs(dvb.time_difference) > 60 )
		{
			eDebug("[TIME] set Linux Time");
			timeval tnow;
			gettimeofday(&tnow,0);
			tnow.tv_sec=t;
			settimeofday(&tnow,0);
			for (ePtrList<eMainloop>::iterator it(eMainloop::existing_loops)
				;it != eMainloop::existing_loops.end(); ++it)
				it->setTimerOffset(t-nowTime);
			dvb.time_difference=1;
		}
		else if ( !dvb.time_difference )
			dvb.time_difference=1;

		lastTpTimeDifference=tdt->UTC_time-t;

		/*emit*/ dvb.timeUpdated();
	}
}

void eDVBServiceController::scanPMT( PMT *pmt )
{
	Decoder::parms.pmtpid=pmt->pid;

	usedCASystems.clear();

	PMTEntry *audio=0, *ac3_audio=0, *video=0, *teletext=0;

	int audiopid=-1, videopid=-1, ac3pid=-1, tpid=-1;

	int sac3default=eAudio::getInstance()->getAC3default();

	if ( Decoder::parms.pcrpid != pmt->PCR_PID && !service.path.size() )
		Decoder::parms.pcrpid = pmt->PCR_PID;

	// get last selected audio / video pid from pid cache
	eService *sp=eServiceInterface::getInstance()->addRef(service);
	if (sp)
	{
		if (sp->dvb)
		{
			videopid=sp->dvb->get(eServiceDVB::cVPID);
			audiopid=sp->dvb->get(eServiceDVB::cAPID);
			ac3pid=sp->dvb->get(eServiceDVB::cAC3PID);
			sp->dvb->set(eServiceDVB::cPCRPID, Decoder::parms.pcrpid);
		}
		eServiceInterface::getInstance()->removeRef(service);
	}

	int isca=checkCA(calist, pmt->program_info, pmt->program_number);

	audioStreams.clear();
	videoStreams.clear();
	subtitleStreams.clear();

	ePtrList<PMTEntry>::iterator TTXIt=pmt->streams.end();
	for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
	{
		PMTEntry *pe=*i;
		bool isAudio=false;

		switch (pe->stream_type)
		{
			case 1:	// ISO/IEC 11172 Video
			case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
				if ( (!video) || (pe->elementary_PID == videopid) )
					video=pe;
				isca+=checkCA(calist, pe->ES_info, pmt->program_number);
				videoStreams.push_back(pe);
				break;
			case 3: // ISO/IEC 11172 Audio
			case 4: // ISO/IEC 13818-3 Audio
				isAudio=true;
			case 6:
			{
				isca+=checkCA(calist, pe->ES_info, pmt->program_number);
				audioStream stream(pe);
				for (ePtrList<Descriptor>::iterator ii(pe->ES_info); ii != pe->ES_info.end(); ++ii)
				{
					switch( ii->Tag() )
					{
						case DESCR_AC3:
							stream.isAC3=1;
							break;
						case DESCR_REGISTRATION:
							if (!memcmp(((RegistrationDescriptor*)*ii)->format_identifier, "DTS", 3))
								stream.isDTS=1;
							break;
						case DESCR_TELETEXT:
							if ( (!teletext) || (pe->elementary_PID == tpid) )
							{
								TTXIt = i;
								teletext=pe;
							}
							break;
						case DESCR_SUBTITLING:
							subtitleStreams.push_back(pe);
							break;
						case DESCR_ISO639_LANGUAGE:
							stream.text=getISO639Description(((ISO639LanguageDescriptor*)*ii)->language_code);
							break;
						case DESCR_STREAM_ID:
							stream.component_tag=((StreamIdentifierDescriptor*)*ii)->component_tag;
							break;
						case DESCR_LESRADIOS:
						{
							LesRadiosDescriptor *d = (LesRadiosDescriptor*)*ii;
							if ( d->id && d->name )
								stream.text.sprintf("%d.) %s", d->id, d->name.c_str());
							else
								isAudio=false;
							break;
						}
						default:
							break;
					}
				}

				if (stream.isAC3)
				{
					stream.text+=" (AC3)";
					isAudio=true;
					if ( (!ac3_audio) || (pe->elementary_PID == ac3pid) )
						ac3_audio=pe;
				}
				else if (stream.isDTS)
				{
					stream.text+=" (DTS)";
					isAudio=true;
					if ( (!ac3_audio) || (pe->elementary_PID == ac3pid) )
						ac3_audio=pe;
				}
				else if (isAudio && ( (!audio) || (pe->elementary_PID == audiopid) ) )
					audio=pe;

				if (!stream.text)
					stream.text.sprintf("PID %04x", pe->elementary_PID);

				if (isAudio)
					audioStreams.push_back(stream);
			}
			default:
				break;
		}
	}

	audio = priorityAudio(audio);

	ePtrList<PMTEntry>::iterator tmp = pmt->streams.end();
	if (TTXIt != tmp)
	{
		// move teletext to the end of the list
		--tmp;
		if ( tmp != TTXIt )
			std::iter_swap(tmp, TTXIt);
	}

	int needAC3Workaround=0;
	switch (eSystemInfo::getInstance()->getHwType())
	{
		case eSystemInfo::dbox2Nokia:
		case eSystemInfo::dbox2Philips:
		case eSystemInfo::dbox2Sagem:
			needAC3Workaround=1;
		default:
			break;
	}

	if ( !needAC3Workaround && ac3_audio && ( sac3default || (ac3pid != -1) || (!audio) ) )
	{
		audiopid = ac3pid;
		audio = ac3_audio;
	}

	if ( video && video->elementary_PID != videopid )
		setPID(video);

	if ( audio && audiopid != audio->elementary_PID )
		setPID(audio);

	if ( teletext && tpid != teletext->elementary_PID )
		setPID(teletext);

	/*emit*/ dvb.scrambled(isca);

	int hideerror=0;
	eConfig::getInstance()->getKey("/elitedvb/extra/hideerror", hideerror);
	if ( hideerror || (dvb.recorder && service.path) )
		;
	else if (isca && !service.path && !calist )
	{
		eDebug("NO CASYS");
		service_state=ENOCASYS;
	}

	if ((Decoder::parms.vpid==-1) && (Decoder::parms.apid==-1))
		service_state=ENOSTREAM;

//	for (ePtrList<CA>::iterator i(calist); i != calist.end(); ++i)
//		eDebug("CA %04x ECMPID %04x", i->casysid, i->ecmpid);

	setDecoder();

	// AC3 DBOX2 Workaround... buggy drivers...
	if ( needAC3Workaround && ac3_audio && ( sac3default || (ac3pid != -1) || (!audio) ) )
	{
		setPID(ac3_audio);
		setDecoder();
	}
}

int eDVBServiceController::switchService(const eServiceReferenceDVB &newservice)
{
	if (newservice == service)
	{
		eDebug("is same service..");
		return 0;
	}

	Decoder::Flush();

#ifndef DISABLE_FILE
	eServiceReferenceDVB recRef =
		dvb.recorder && dvb.recorder->recRef ?
			dvb.recorder->recRef : eServiceReferenceDVB();
	recRef.data[0] = service.getServiceType();
#endif

	if ( service
#ifndef DISABLE_FILE
		&& !service.path
		&& Decoder::locked != 2 // leave service for (timer) zap in Background
		&& service != recRef
#endif
		)
	{
		// must replace faked service types.. for capmt handlers
		eServiceReferenceDVB ref=service;
		switch(ref.getServiceType())
		{
			case 4:
			case 7:
				ref.data[0]=1; // TV
				break;
		}
		eDVBCaPMTClientHandler::distribute_leaveService(ref); // capmt handler call..
	}

	/*emit*/ dvb.leaveService(service);

// Linkage service handling..
	if ( newservice.getServiceType()==7 && prevservice )
	{
		parentservice = prevservice;
		prevservice = eServiceReferenceDVB();
	}

	if ( !newservice && service.getServiceType() != 7 )
		prevservice=service;  // save currentservice
/////////////////////////////////

	service=newservice;

	dvb.tEIT.start(0);  // clear eit
	dvb.tPAT.start(0);  // clear tables.
	dvb.tPMT.start(0);
	dvb.tSDT.start(0);

	if (service)
	{
		if ( service && !service.path )
			eDVBCaPMTClientHandler::distribute_enterService(service); // capmt handler call..

		dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceSwitch));
	}

	switch(newservice.getServiceType())
	{
		case 1:  // tv service
		case 2:  // radio service
		case 4:  // nvod parent service
		case 7:  // linkage service
			delete dvb.parentEIT;
			dvb.parentEIT = 0;
		break;
		case 5:  // nvod ref service
			// send Parent EIT .. for osd text..
			dvb.gotEIT(0,0);
		break;
	}
	return 1;
}

#ifndef DISABLE_CI
void eDVBServiceController::handlePMT(const eServiceReferenceDVB &ref, PMT *pmt)
{
	if ( CIServices.find(ref) != CIServices.end() )
	{
		int prevPMTVersion = CIServices[ref];
		if ( prevPMTVersion == pmt->version )
		{
			eDebug("[eDVBCIHandler] dont send pmt with self pmt version");
			return;
		}
	}
	else
	{
		eDebug("[eDVBCIHandler] get PMT for unknown service");
		return;
	}

	int sid= pmt->program_number;

	if ( eSystemInfo::getInstance()->hasCI() )
		calist.clear();

	if ( DVBCI )
		DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTsetVersion, pmt->program_number, pmt->version, -1 ));
	if ( DVBCI2 )
		DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTsetVersion, pmt->program_number, pmt->version, -1 ));

	for (ePtrList<Descriptor>::const_iterator i(pmt->program_info);
		i != pmt->program_info.end(); ++i)
	{
		if (i->Tag()==9)	// CADescriptor
		{
			CADescriptor *ca=(CADescriptor*)*i;
			if ( DVBCI )
			{
				unsigned char *buf=new unsigned char[ca->data[1]+2];
				memcpy(buf, ca->data, ca->data[1]+2);
				DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTaddDescriptor, sid, buf));
			}
			if ( DVBCI2 )
			{
				unsigned  char *buf2=new unsigned char[ca->data[1]+2];
				memcpy(buf2, ca->data, ca->data[1]+2);
				DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTaddDescriptor, sid, buf2));
			}
		}
	}

	for (ePtrList<PMTEntry>::iterator i(pmt->streams);
		i != pmt->streams.end(); ++i)
	{
		PMTEntry *pe=*i;

		if ( DVBCI )
			DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTaddPID, pmt->program_number, pe->elementary_PID, pe->stream_type));
		if ( DVBCI2 )
			DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTaddPID, pmt->program_number, pe->elementary_PID, pe->stream_type));

		switch (pe->stream_type)
		{
			case 1:	// ISO/IEC 11172 Video
			case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
			case 3:	// ISO/IEC 11172 Audio
			case 4: // ISO/IEC 13818-3 Audio
			case 6:
			{
				for (ePtrList<Descriptor>::const_iterator i(pe->ES_info);
					i != pe->ES_info.end(); ++i)
				{
					if (i->Tag()==9)	// CADescriptor
					{
						CADescriptor *ca=(CADescriptor*)*i;
						if ( DVBCI )
						{
							unsigned char *buf=new unsigned char[ca->data[1]+2];
							memcpy(buf, ca->data, ca->data[1]+2);
							DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTaddDescriptor, sid, buf));
						}
						if ( DVBCI2 )
						{
							unsigned  char *buf2=new unsigned char[ca->data[1]+2];
							memcpy(buf2, ca->data, ca->data[1]+2);
							DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTaddDescriptor, sid, buf2));
						}
					}
				}
				break;
			}
		}
	}
	if ( DVBCI )
		DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::go));
	if ( DVBCI2 )
		DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::go));
	CIServices[ref]=pmt->version;
}

void eDVBServiceController::enterService( const eServiceReferenceDVB &service)
{
	if (!service)
		return;
	if ( CIServices.find(service) == CIServices.end() )
	{
//		eDebug("[eDVBCIHandler] new service %s", service.toString().c_str() );
		CIServices[service]=-1;
	}
}

void eDVBServiceController::leaveService( const eServiceReferenceDVB &service)
{
	if (!service)
		return;
	std::map<eServiceReferenceDVB,int>::iterator it = CIServices.find(service);
	if ( it != CIServices.end() )
	{
//		eDebug("[eDVBCIHandler] leave service %s", service.toString().c_str() );
		CIServices.erase(it);
		if ( DVBCI )
			DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTflush, service.getServiceID().get() ));
		if ( DVBCI2 )
			DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::PMTflush, service.getServiceID().get() ));
	}
}
#endif

void eDVBServiceController::setPID(const PMTEntry *entry)
{
	if (entry)
	{
		int isvideo=0, isaudio=0, isteletext=0, isAC3=0;
		switch (entry->stream_type)
		{
			case 1:	// ISO/IEC 11172 Video
			case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
				isvideo=1;
			break;
			case 3:	// ISO/IEC 11172 Audio
			case 4: // ISO/IEC 13818-3 Audio
				isaudio=1;
			break;
			case 6:
			{
				for (ePtrList<Descriptor>::const_iterator i(entry->ES_info);
					i != entry->ES_info.end(); ++i)
				{
					if (i->Tag()==DESCR_AC3)
					{
						isaudio=1;
						isAC3=1;
					}
					else if (i->Tag() == DESCR_REGISTRATION)
					{
						RegistrationDescriptor *reg=(RegistrationDescriptor*)*i;
						if (!memcmp(reg->format_identifier, "DTS", 3))
						{
							isaudio=1;
							isAC3=1;
						}
					} else if (i->Tag()==DESCR_TELETEXT)
						isteletext=1;
				}
			}
		}

		eService *sp=eServiceInterface::getInstance()->addRef(service);
		if (isaudio)
		{
			if (isAC3)
			{
				Decoder::parms.audio_type=DECODE_AUDIO_AC3;
				Decoder::parms.apid=entry->elementary_PID;
				if (sp && sp->dvb)
				{
					sp->dvb->set(eServiceDVB::cAC3PID, entry->elementary_PID);
					sp->dvb->set(eServiceDVB::cAPID, -1);
				}
			} else
			{
				Decoder::parms.audio_type=DECODE_AUDIO_MPEG;
				Decoder::parms.apid=entry->elementary_PID;
				if (sp && sp->dvb)
				{
					sp->dvb->set(eServiceDVB::cAC3PID, -1);
					sp->dvb->set(eServiceDVB::cAPID, entry->elementary_PID);
				}
			}
		}
		else if (isvideo)
		{
			Decoder::parms.vpid=entry->elementary_PID;
			if (sp && sp->dvb)
				sp->dvb->set(eServiceDVB::cVPID, entry->elementary_PID);
		}
		else if (isteletext)
		{
			Decoder::parms.tpid=entry->elementary_PID;
			if (sp && sp->dvb)
				sp->dvb->set(eServiceDVB::cTPID, entry->elementary_PID);
		}

		if (sp)
			eServiceInterface::getInstance()->removeRef(service);
	}
}

void eDVBServiceController::setDecoder()
{
	dvb.event(eDVBServiceEvent(eDVBServiceEvent::eventServiceNewPIDs));
}

void eDVBServiceController::MHWEITready(int error)
{
	if (!error)
	{
		EIT *e=new EIT();
		e->ts=EIT::tsFaked;
		e->type=EIT::typeNowNext;
		e->version_number=0;
		e->current_next_indicator=0;
		e->transport_stream_id=service.getTransportStreamID().get();
		e->original_network_id=service.getOriginalNetworkID().get();
		
		for (int i=0; i<2; i++)
		{
			MHWEITEvent *me=&tMHWEIT->events[i];
			EITEvent *ev=new EITEvent;
			int thisday=dvb.time_difference+time(0);
			thisday-=thisday%(60*60*24);
			if (thisday < (dvb.time_difference+time(0)))
				thisday+=60*60*24;
			e->service_id=me->service_id;
			ev->event_id=0xFFFF;
			ev->start_time=thisday+(me->starttime>>8)*60*60+(me->starttime&0xFF)*60;
			ev->duration=(me->duration>>8)*60*60+(me->duration&0xFF)*60;
			ev->running_status=1;
			ev->free_CA_mode=0;
			ShortEventDescriptor *se=new ShortEventDescriptor();
			se->language_code[0]='?';
			se->language_code[1]='?';
			se->language_code[2]='?';
			se->event_name=me->event_name;
			se->text=me->short_description;
			ev->descriptor.push_back(se);
			e->events.push_back(ev);
		}
		e->ready=1;
		// check if no normal eit exist...
//		if (!dvb.tEIT.ready())
//			dvb.tEIT.inject(e);
	}
	else
	{
		delete tMHWEIT;
		tMHWEIT=0;
	}
}

int eDVBServiceController::checkCA(ePtrList<CA> &list, const ePtrList<Descriptor> &descriptors, int sid)
{
	int found=0;
	for (ePtrList<Descriptor>::const_iterator i(descriptors);
		i != descriptors.end(); ++i)
	{
		if (i->Tag()==9)	// CADescriptor
		{
			found++;
			CADescriptor *ca=(CADescriptor*)*i;

#if 0
// this is old unneeded code for camd call..
// now we do this in eDVBCAHandler..
			Decoder::addCADescriptor((__u8*)(ca->data));
#endif 

			int avail=0;
			{
				singleLock s(availCALock);
				if (availableCASystems.find(ca->CA_system_ID) != availableCASystems.end())
					avail++;
			}

			usedCASystems.insert(ca->CA_system_ID);

			if (avail)
			{
				for (ePtrList<CA>::iterator a = list.begin();
					a != list.end(); a++)
				{
					if (a->casysid==ca->CA_system_ID)
					{
						avail=0;
						break;
					}
				}
				if (avail)
				{
					CA *n=new CA;
					n->ecmpid=ca->CA_PID;
					n->casysid=ca->CA_system_ID;
					n->emmpid=-1;
					list.push_back(n);
				}
			}
		}
	}
	return found;
}

void eDVBServiceController::initCAlist()
{
	singleLock s(availCALock);
	availableCASystems=eSystemInfo::getInstance()->getCAIDs();
}

void eDVBServiceController::clearCAlist()
{
	{
		singleLock s(availCALock);
		availableCASystems.clear();
	}
	initCAlist();
#ifndef DISABLE_CI
	if (DVBCI)
		DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::getcaids));
	if (DVBCI2)
		DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::getcaids));
#endif
}

void eDVBServiceController::startTDT()
{
	delete tdt;
	tdt=0;
	if ( !service.path.size() )
	{
		tdt=new TDT();
		CONNECT(tdt->tableReady, eDVBServiceController::TDTready);
		tdt->start();
	}
}

/*
					case 0xC1:
					{
						for (ePtrList<Descriptor>::iterator i(pe->ES_info); i != pe->ES_info.end(); ++i)
						if (i->Tag()==DESCR_MHW_DATA)
						{
							MHWDataDescriptor *mhwd=(MHWDataDescriptor*)*i;
							if (!strncmp(mhwd->type, "PILOTE", 6))
							{
								if (tMHWEIT)	// nur eine zur zeit
								{
									delete tMHWEIT;
									tMHWEIT=0;
								}
								eDebug("starting MHWEIT on pid %x, sid %x", pe->elementary_PID, service.getServiceID().get());
								tMHWEIT=new MHWEIT(pe->elementary_PID, service.getServiceID().get());
								CONNECT(tMHWEIT->ready, eDVBServiceController::MHWEITready);
								tMHWEIT->start();
								break;
							}
						}
*/
