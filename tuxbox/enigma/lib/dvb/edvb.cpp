#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef PROFILE
#include <sys/time.h>
#endif
#include "edvb.h"
#include "esection.h"
#include "si.h"
#include "frontend.h"
#include "dvb.h"
#include "decoder.h"
#include <qfile.h>
#include <dbox/info.h>
#include "eavswitch.h"
#include "streamwd.h"

#include "config.h"

eDVB *eDVB::instance;

QString eDVB::getVersion()
{
	return "eDVB core 1.0RC2, compiled " __DATE__;
}

void eDVB::removeDVBBouquets()
{
	for (QListIterator<eBouquet> i(bouquets); i.current(); )
		if (i.current()->bouquet_id>=0)
		{
			qDebug("removing bouquet '%s'", (const char*)i.current()->bouquet_name);
			bouquets.remove(i.current());
		} else
		{
			qDebug("leaving bouquet '%s'", (const char*)i.current()->bouquet_name);
			++i;
		}
}

void eDVB::addDVBBouquet(BAT *bat)
{
	qDebug("wir haben da eine bat, id %x", bat->bouquet_id);
	QString bouquet_name="Weiteres Bouquet";
	for (QListIterator<Descriptor> i(bat->bouquet_descriptors); i.current(); ++i)
	{
		Descriptor *d=i.current();
		if (d->Tag()==DESCR_BOUQUET_NAME)
			bouquet_name=((BouquetNameDescriptor*)d)->name;
	}
	eBouquet *bouquet=createBouquet(bat->bouquet_id, bouquet_name);
	
	for (QListIterator<BATEntry> be(bat->entries); be.current(); ++be)
		for (QListIterator<Descriptor> i(be.current()->transport_descriptors); i.current(); ++i)
			if (i.current()->Tag()==DESCR_SERVICE_LIST)
			{
				ServiceListDescriptor *s=(ServiceListDescriptor*)i.current();
				for (QListIterator<ServiceListDescriptorEntry> a(s->entries); a.current(); ++a)
					bouquet->add(be.current()->transport_stream_id, be.current()->original_network_id, a.current()->service_id);
			}
}

eBouquet *eDVB::getBouquet(int bouquet_id)
{
	for (QListIterator<eBouquet> i(bouquets); i.current(); ++i)
		if (i.current()->bouquet_id==bouquet_id)
			return i.current();
	return 0;
}

static QString beautifyBouquetName(QString bouquet_name)
{
	if (bouquet_name=="ABsat")
		bouquet_name="AB sat";
	if (bouquet_name=="Astra-Net")
		bouquet_name="ASTRA";
/*	if (bouquet_name=="CANALSATELLITE")
		bouquet_name="CANAL SATELLITE"; */
	if (bouquet_name.find("SES")!=-1)
		bouquet_name="SES Multimedia";
	if (bouquet_name=="\x05ZDF.vision")	// ja ich weiss was \x05 bedeutet - SPÄTER
		bouquet_name="ZDF.vision";
	return bouquet_name;
}

eBouquet *eDVB::getBouquet(QString bouquet_name)
{
	for (QListIterator<eBouquet> i(bouquets); i.current(); ++i)
		if (!qstricmp(i.current()->bouquet_name, bouquet_name))
			return i.current();
	return 0;
}

eBouquet *eDVB::createBouquet(int bouquet_id, QString bouquet_name)
{
	eBouquet *n=getBouquet(bouquet_id);
	if (!n)
		bouquets.append(n=new eBouquet(bouquet_id, bouquet_name));
	return n;
}

eBouquet *eDVB::createBouquet(QString bouquet_name)
{
	eBouquet *n=getBouquet(bouquet_name);
	if (!n)
	{
		int bouquet_id=getUnusedBouquetID(0);
		bouquets.append(n=new eBouquet(bouquet_id, bouquet_name));
	}
	return n;
}

int eDVB::getUnusedBouquetID(int range)
{
	if (range)
		range=-1;
	else
		range=1;
	for (int bouquet_id=0; ; bouquet_id+=range)
		if (!getBouquet(bouquet_id))
			return bouquet_id;
}

void eDVB::revalidateBouquets()
{
	qDebug("revalidating bouquets");
	if (transponderlist)
		for (QListIterator<eBouquet> i(bouquets); i.current(); ++i)
			for (QListIterator<eServiceReference> service(i.current()->list); service.current(); ++service)
				service.current()->service=transponderlist->searchService(service.current()->original_network_id, service.current()->service_id);
	emit bouquetListChanged();
	qDebug("ok");
}

int eDVB::checkCA(QList<CA> &list, const QList<Descriptor> &descriptors)
{
	int found=0;
	for (QListIterator<Descriptor> i(descriptors); i.current(); ++i)
	{
		if (i.current()->Tag()==9)	// CADescriptor
		{
			found++;
			CADescriptor *ca=(CADescriptor*)i.current();
			Decoder::addCADescriptor((__u8*)(ca->data));
			int avail=0;
			for (QListIterator<int> i(availableCASystems); i.current() && !avail; ++i)
				if (*i.current() == ca->CA_system_ID)
					avail++;
			if (avail)
			{
				for (QListIterator<CA> a(list); a.current(); ++a)
					if (a.current()->casysid==ca->CA_system_ID)
					{
						avail=0;
						break;
					}
				if (avail)
				{
					CA *n=new CA;
					n->ecmpid=ca->CA_PID;
					n->casysid=ca->CA_system_ID;
					n->emmpid=-1;					
					list.append(n);
				}
			}
		}
	}
	return found;
}

void eDVB::scanEvent(int event)
{
	emit eventOccured(event);
	switch (event)
	{
	case eventScanBegin:
		Decoder::Flush();
		if (transponderlist)
			delete transponderlist;
		transponderlist=new eTransponderList();
		if (useBAT)
			removeDVBBouquets();
		emit serviceListChanged();
		if (!initialTransponders)
		{
			qFatal("no initial transponders");
			scanEvent(eventScanCompleted);
			break;
		}
		for (QListIterator<eTransponder> i(*initialTransponders); i.current(); ++i)
			transponderlist->addTransponder(new eTransponder(*i.current()));
		currentONID=-1;
		knownNetworks.clear();
		scanEvent(eventScanNext);
		break;
	case eventScanNext:
	{
		eTransponder *next=transponderlist->getFirstTransponder(eTransponder::stateListed);
		transponder=next;
		
		if (!next)
		{
			scanEvent(eventScanCompleted);
		} else
		{
			if (next->tune())
				scanEvent(eventScanError);
			else
				setState(stateScanTune);
		}
		break;
	}
	case eventScanTuneError:
		scanEvent(eventScanError);
		break;
	case eventScanTuneOK:
		setState(stateScanGetPAT);
		tPAT.get();
		break;
	case eventScanGotPAT:
	{
		if (state!=stateScanGetPAT)
			qFatal("unexpected gotPAT");

		if (!tPAT.ready())
			qFatal("tmb suckt -> no pat");
		PAT *pat=tPAT.getCurrent();
		int nitpid;
		PATEntry *pe=pat->searchService(0);
		if (!pe)
		{
			qDebug("no NIT-PMTentry, assuming 0x10");
			nitpid=0x10;
		}	else
			nitpid=pe->program_map_PID;
		pat->unlock();
		scanOK=0;
		tNIT.start(new NIT(nitpid));
		if (scanflags&SCAN_ONIT)
			tONIT.start(new NIT(nitpid, NIT::typeOther));
		else
			scanOK|=8;
		tSDT.start(new SDT());
		if (useBAT)
			tBAT.start(new BAT());
		else
			scanOK|=4;
		setState(stateScanWait);
		break;
	}
	case eventScanGotSDT:
	{
		SDT *sdt=tSDT.ready()?tSDT.getCurrent():0;
		if (sdt)
		{
			transponder->transport_stream_id=sdt->transport_stream_id;
			transponder->original_network_id=sdt->original_network_id;
			
			currentONID=sdt->original_network_id;
			int known=0;
			int *i;
			for (i=knownNetworks.first(); i; i=knownNetworks.next())
				if (*i == sdt->original_network_id)
					known=1;
			
			if (known)
			{
				if (scanflags&SCAN_SKIP)
				{
					tNIT.abort();
					tONIT.abort();
				}
			}
			transponderlist->handleSDT(sdt);
			sdt->unlock();
		}
		scanOK|=1;
		qDebug("scanOK %d", scanOK);
		if (scanOK==15)
			scanEvent(eventScanComplete);
		break;
	}
	case eventScanGotNIT:
	case eventScanGotONIT:
	{
		NIT *nit=(event==eventScanGotNIT)?(tNIT.ready()?tNIT.getCurrent():0):(tONIT.ready()?tONIT.getCurrent():0);
		if (nit)
		{
			if (event==eventScanGotNIT)
				if (currentONID!=-1)
					knownNetworks.append(new int(currentONID));
			for (QListIterator<NITEntry> i(nit->entries); i.current(); ++i)
			{
				NITEntry *entry=i.current();
				eTransponder *transponder=transponderlist->create(entry->transport_stream_id, entry->original_network_id);
				for (QListIterator<Descriptor> d(entry->transport_descriptor); d.current(); ++d)
				{
					switch (d.current()->Tag())
					{
					case DESCR_SAT_DEL_SYS:
						transponder->setSatellite((SatelliteDeliverySystemDescriptor*)d.current());
						break;
					case DESCR_CABLE_DEL_SYS:
						transponder->setCable((CableDeliverySystemDescriptor*)d.current());
						break;
					}
				}
			}
			nit->unlock();
		}
		scanOK|=(event==eventScanGotNIT)?2:8;
		qDebug("scanOK %d", scanOK);
		if (scanOK==15)
			scanEvent(eventScanComplete);
		break;
	}
	case eventScanGotBAT:
	{
		BAT *bat=tBAT.ready()?tBAT.getCurrent():0;
		if (bat)
		{
			addDVBBouquet(bat);
			bat->unlock();
		}
		scanOK|=4;
		qDebug("scanOK %d", scanOK);
		if (scanOK==15)
			scanEvent(eventScanComplete);
		break;
	}
	case eventScanError:
		qDebug("with error");
	case eventScanComplete:
		qDebug("completed");
		if (transponder)
		{
			if (event==eventScanError)
				transponder->state=eTransponder::stateError;
			else
				transponder->state=eTransponder::stateOK;
		}
		scanEvent(eventScanNext);
		break;
	case eventScanCompleted:
		qDebug("scan has finally completed.");
		saveServices();
		sortInChannels();
		emit serviceListChanged();
		setState(stateIdle);
		break;
	}
}

void eDVB::serviceEvent(int event)
{
	emit eventOccured(event);
#ifdef PROFILE
	static timeval last_event;
	
	timeval now;
	gettimeofday(&now, 0);
	
	int diff=(now.tv_sec-last_event.tv_sec)*1000000+(now.tv_usec-last_event.tv_usec);
	last_event=now;
	
	char *what="unknown";

	switch (event)
	{
	case	eventServiceSwitch: what="ServiceSwitch"; break;
	case	eventServiceTuneOK: what="TuneOK"; break;
	case	eventServiceTuneFailed: what="TuneFailed"; break;
	case	eventServiceGotPAT: what="GotPAT"; break;
	case	eventServiceGotPMT: what="GotPMT"; break;
	case	eventServiceNewPIDs: what="NewPIDs"; break;
	case	eventServiceGotSDT: what="GotSDT"; break;
	case	eventServiceSwitched: what="Switched"; break;
	case	eventServiceFailed: what="Failed"; break;

	}
	
	printf("[PROFILE] [%s] +%dus\n", what, diff);
#endif
	switch (event)
	{
	case eventServiceSwitch:
	{
		if (!transponderlist)
		{
			service_state=ENOENT;
			serviceEvent(eventServiceFailed);
			return;
		}
		eTransponder *n=transponderlist->searchTS(original_network_id, transport_stream_id);
		if (!n)
		{
			setState(eventServiceTuneFailed);
			break;
		}
		if (n->state!=eTransponder::stateOK)
		{
			qDebug("couldn't tune");
			service_state=ENOENT;
			serviceEvent(eventServiceFailed);
			return;
		}
		if (n==transponder)
		{
//			setState(stateServiceTune);
			serviceEvent(eventServiceTuneOK);
		} else
		{
			emit leaveTransponder(transponder);
			transponder=n;
			if (n->tune())
				serviceEvent(eventServiceTuneFailed);
			else
				setState(stateServiceTune);
		}
		qDebug("<-- tuned");
		break;
	}
	case eventServiceTuneOK:
		emit enterTransponder(transponder);
		switch (service_type)
		{
		case 1:	// digital television service
		case 2:	// digital radio service
		case 3:	// teletext service
			tEIT.start(new EIT(EIT::typeNowNext, service_id, EIT::tsActual));
		case 5:	// NVOD time shifted service
			setState(stateServiceGetPAT);
			tPAT.get();
			break;
		case 4:	// NVOD reference service
			setState(stateServiceGetSDT);
			tEIT.start(new EIT(EIT::typeNowNext, service_id, EIT::tsActual));
			tSDT.start(new SDT());
			break;
		case 6:	// mosaic service
			setState(stateServiceGetPAT);
			tSDT.start(new SDT());
			tPAT.get();
			break;
		case -1: // data
			setState(stateServiceGetPAT);
			tPAT.get();
			break;
		default:
			service_state=ENOSYS;
			serviceEvent(eventServiceFailed);
			break;
		}
		break;
	case eventServiceTuneFailed:
		qDebug("[TUNE] tune failed");
		service_state=ENOENT;
		serviceEvent(eventServiceFailed);
		break;
	case eventServiceGotPAT:
	{
		qDebug("eventServiceGotPAT");
		PAT *pat=tPAT.getCurrent();
		PATEntry *pe=pat->searchService(service_id);
		if (!pe)
		{
			pmtpid=-1;
		}	else
			pmtpid=pe->program_map_PID;
		pat->unlock();
		if (pmtpid==-1)
		{
			qDebug("[PAT] no pat entry");
			service_state=ENOENT;
			serviceEvent(eventServiceFailed);
			return;
		}
		tPMT.start(new PMT(pmtpid, service_id));
		setState(stateServiceGetPMT);
		break;
	}	
	case eventServiceGotPMT:
		service_state=0;
		scanPMT();
		{
			PMT *pmt=tPMT.ready()?tPMT.getCurrent():0;
			if (pmt)
			{
				emit gotPMT(pmt);
				pmt->unlock();
			}
		}
		qDebug("eventServiceSwitched");
		if (state==stateServiceGetPMT)
			serviceEvent(eventServiceSwitched);
		else
			qDebug("nee, doch nicht");
		break;
	case eventServiceGotSDT:
	{
		qDebug("eventServiceGotSDT");
		SDT *sdt=tSDT.ready()?tSDT.getCurrent():0;
		if (sdt)
		{
			setState(stateIdle);
			emit gotSDT(sdt);
			sdt->unlock();
			if (service_type==4)
				service_state=ENVOD;
			serviceEvent(eventServiceSwitched);
		} else
			serviceEvent(eventServiceFailed);
		break;
	}
	case eventServiceNewPIDs:
		Decoder::Set(useAC3);
		break;
	case eventServiceSwitched:
		emit enterService(service);
	case eventServiceFailed:
		switchedService(service, -service_state);
		setState(stateIdle);
		break;
	}
}

void eDVB::scanPMT()
{
	PMT *pmt=tPMT.ready()?tPMT.getCurrent():0;
	if (!pmt)
	{
		qDebug("scanPMT with no available pmt");
		return;
	}
	Decoder::parms.pmtpid=pmtpid;
	Decoder::parms.pcrpid=pmt->PCR_PID;
	Decoder::parms.ecmpid=Decoder::parms.emmpid=Decoder::parms.casystemid=-1;
	Decoder::parms.vpid=Decoder::parms.apid=-1;
	
	int isca=0;
	
	calist.clear();
	Decoder::parms.descriptor_length=0;

	isca+=checkCA(calist, pmt->program_info);
	
	PMTEntry *audio=0, *video=0, *teletext=0;
	
	for (QListIterator<PMTEntry> i(pmt->streams); i.current(); ++i)
	{
		PMTEntry *pe=i.current();
		switch (pe->stream_type)
		{
		case 1:	// ISO/IEC 11172 Video
		case 2: // ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2 constrained parameter video stream
			isca+=checkCA(calist, pe->ES_info);
			if (!video)
				video=pe;
			break;
		case 3:	// ISO/IEC 11172 Audio
		case 4: // ISO/IEC 13818-3 Audio
			isca+=checkCA(calist, pe->ES_info);
			if (!audio)
				audio=pe;
			break;
		case 6:
		{
			isca+=checkCA(calist, pe->ES_info);
			for (QListIterator<Descriptor> i(pe->ES_info); i.current(); ++i)
			{
				Descriptor *d=i.current();
				if ((d->Tag()==DESCR_AC3) && useAC3)
					audio=pe;
				if (d->Tag()==DESCR_TELETEXT)
					teletext=pe;
			}
			break;
		}
		default:
			qDebug("streamtype %d (Pid: %x) unused", pe->stream_type, pe->elementary_PID);
		}
	}

	setPID(video);
	setPID(audio);
	setPID(teletext);

	if (!isca)
	{
		qDebug("channel is FTA!");
	}

	if (isca && calist.isEmpty())
	{
		qDebug("NO CASYS");
		service_state=ENOCASYS;
	}

	if ((Decoder::parms.vpid==-1) && (Decoder::parms.apid==-1))
		service_state=ENOSTREAM;

	for (QListIterator<CA> i(calist); i.current(); ++i)
	{
		qDebug("CA %04x ECMPID %04x", i.current()->casysid, i.current()->ecmpid);
	}

	pmt->unlock();
	setDecoder();
}

void eDVB::tunedIn(eTransponder *trans, int err)
{
	currentTransponder=trans;
	currentTransponderState=err;
	if (!err)
		emit enterTransponder(trans);
	tPAT.start(new PAT());
	if (tdt)
		delete tdt;
	tdt=new TDT();
	connect(tdt, SIGNAL(tableReady(int)), SLOT(TDTready(int)));
	tdt->start();
	switch (state)
	{
	case stateIdle:
		break;
	case stateScanTune:
		if (transponder==trans)
			scanEvent(err?eventScanTuneError:eventScanTuneOK);
		break;
	case stateServiceTune:
		if (transponder==trans)
			serviceEvent(err?eventServiceTuneFailed:eventServiceTuneOK);
		break;		
	}
}

void eDVB::PATready(int error)
{
	switch (state)
	{
	case stateScanGetPAT:
		scanEvent(error?eventScanError:eventScanGotPAT);
		break;
	case stateServiceGetPAT:
		serviceEvent(error?eventServiceFailed:eventServiceGotPAT);
		break;
	}
}

void eDVB::SDTready(int error)
{
	qDebug("SDTready %s", strerror(-error));
	switch (state)
	{
	case stateScanWait:
		scanEvent(eventScanGotSDT);
		break;
	case stateServiceGetSDT:
		serviceEvent(eventServiceGotSDT);
		break;
	}
}

void eDVB::PMTready(int error)
{
	qDebug("PMTready %s", strerror(-error));
	switch (state)
	{
	case stateIdle:
	case stateServiceGetPMT:
		serviceEvent(eventServiceGotPMT);
		break;
	}
}

void eDVB::NITready(int error)
{
	qDebug("NITready %s", strerror(-error));
	switch (state)
	{
	case stateScanWait:
		scanEvent(eventScanGotNIT);
		break;
	}
}

void eDVB::ONITready(int error)
{
	qDebug("ONITready %s", strerror(-error));
	switch (state)
	{
	case stateScanWait:
		scanEvent(eventScanGotONIT);
		break;
	}
}

void eDVB::EITready(int error)
{
	qDebug("EITready %s", strerror(-error));
	if (!error)
	{
		EIT *eit=tEIT.getCurrent();
		emit gotEIT(eit, 0);
		eit->unlock();
	} else
		emit gotEIT(0, error);
}

void eDVB::TDTready(int error)
{
	qDebug("TDTready %d", error);
	if (!error)
	{
		qDebug("[TIME] time update to %s", ctime(&tdt->UTC_time));
		time_difference=tdt->UTC_time-time(0);
	}
}

void eDVB::BATready(int error)
{
	qDebug("BATready %s", strerror(-error));
	switch (state)
	{
	case stateScanWait:
		scanEvent(eventScanGotBAT);
		break;	
	}
}

int eDVB::startScan(const QList<eTransponder> &initial, int flags)
{
	if (state!=stateIdle)
	{
		qFatal("BUSY");
		return -EBUSY;
	}
	scanflags=flags;
	initialTransponders=&initial;
	scanEvent(eventScanBegin);
	initialTransponders=0;
	return 0;
}

int eDVB::switchService(eService *newservice)
{
	if (newservice==service)
		return 0;
	emit leaveService(service);
	service=newservice;
	return switchService(service->service_id, service->original_network_id, service->transport_stream_id, service->service_type);
}

int eDVB::switchService(int nservice_id, int noriginal_network_id, int ntransport_stream_id, int nservice_type)
{
	original_network_id=noriginal_network_id;
	transport_stream_id=ntransport_stream_id;
	service_id=nservice_id;
	service_type=nservice_type;
	
	serviceEvent(eventServiceSwitch);
	qDebug("<--- switch service event");
	return 1;
}

eDVB::eDVB()
{
	config.setName(CONFIGDIR "/elitedvb/registry");
	if (config.open())
	{
		if (config.createNew())
		{
			mkdir(CONFIGDIR "/elitedvb", 0777);
			if (config.createNew())
				qFatal("error while opening/creating registry - create " CONFIGDIR "/elitedvb");
		}
		if (config.open())
			qFatal("still can't open configfile");
	}

	time_difference=0;
	if (config.getKey("/elitedvb/audio/useAC3", useAC3))
		useAC3=0;
	if (config.getKey("/elitedvb/DVB/useBAT", useBAT))
		useBAT=0;
	if (instance)
		qFatal("eDVB already initialized!");
	instance=this;
	QString frontend=getInfo("fe");
	int fe;
	if (!frontend)
	{
		qWarning("WARNING: couldn't determine frontend-type, assuming cable...\n");
		fe=eFrontend::feCable;
	} else
	{
		switch(atoi(frontend))
		{
		case DBOX_FE_CABLE:
			fe=eFrontend::feCable;
			break;
		case DBOX_FE_SAT:
			fe=eFrontend::feSatellite;
			break;
		default:
			qWarning("COOL: dvb-t is out. less cool: eDVB doesn't support it yet...\n");
			fe=eFrontend::feCable;
			break;
		}
	}
	if (eFrontend::open(fe)<0)
		qFatal("couldn't open frontend");
	connect(eFrontend::fe(), SIGNAL(tunedIn(eTransponder*,int)), SLOT(tunedIn(eTransponder*,int)));

	transponderlist=0;
	currentTransponder=0;
	currentTransponderState=-1;
	loadServices();
	loadBouquets();
	Decoder::Initialize();
	
	connect(&tPAT, SIGNAL(tableReady(int)), SLOT(PATready(int)));
	connect(&tPMT, SIGNAL(tableReady(int)), SLOT(PMTready(int)));
	connect(&tSDT, SIGNAL(tableReady(int)), SLOT(SDTready(int)));
	connect(&tNIT, SIGNAL(tableReady(int)), SLOT(NITready(int)));
	connect(&tONIT, SIGNAL(tableReady(int)), SLOT(ONITready(int)));
	connect(&tEIT, SIGNAL(tableReady(int)), SLOT(EITready(int)));
	connect(&tBAT, SIGNAL(tableReady(int)), SLOT(BATready(int)));
	
	setState(stateIdle);
	knownNetworks.setAutoDelete(true);
	availableCASystems.setAutoDelete(true);

	availableCASystems.append(new int(0x1702));	// BetaCrypt C (sat)
	availableCASystems.append(new int(0x1722));	// BetaCrypt D (cable)
	availableCASystems.append(new int(0x1762));	// BetaCrypt F (ORF)
	
	service=0;
	tdt=0;
	calist.setAutoDelete(true);
	bouquets.setAutoDelete(true);

	int type=0;
	QString mid=getInfo("mID");
	if (mid)
		type=atoi(mid);
	
	switch (type)
	{
	case 1:
		new eAVSwitchNokia;
		break;
/*	case 2:
		new eAVSwitchPhilips;
		break;
	case 3:
		new eAVSwitchSagem;
		break;	*/
	default:
		new eAVSwitchNokia;
		break;
	}
	
	qDebug("instance: %p",  eAVSwitch::getInstance());
	eAVSwitch::getInstance()->setInput(0);
	eAVSwitch::getInstance()->setActive(1);

	int vol, m;
	if (config.getKey("/elitedvb/audio/volume", vol))
		vol=10;
	if (config.getKey("/elitedvb/audio/mute", m))
		m=0;
	changeVolume(1, vol);
	changeVolume(3, m);
	
	streamwd=new eStreamWatchdog();
	
	qDebug("eDVB::eDVB done.");
}

eDVB::~eDVB()
{
	delete streamwd;
	delete eAVSwitch::getInstance();
	config.setKey("/elitedvb/audio/volume", volume);
	config.setKey("/elitedvb/audio/mute", mute);
	Decoder::Close();
	if (transponderlist)
		delete transponderlist;
	eFrontend::close();
	instance=0;
	config.close();
}

eTransponderList *eDVB::getTransponders()
{
	return transponderlist;
}

QList<eService> *eDVB::getServices()
{
	return transponderlist?transponderlist->getServices():0;
}

QList<eBouquet> *eDVB::getBouquets()
{
	return &bouquets;
}

void eDVB::setTransponders(eTransponderList *tlist)
{
	if (transponderlist)
		delete transponderlist;
	transponderlist=tlist;
	emit serviceListChanged();
}

QString eDVB::getInfo(const char *info)
{
	FILE *f=fopen("/proc/bus/dbox", "rt");
	if (!f)
		return 0;
	QString result(0);
	while (1)
	{
		char buffer[128];
		if (!fgets(buffer, 128, f))
			break;
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;
		if ((!strncmp(buffer, info, strlen(info)) && (buffer[strlen(info)]=='=')))
		{
			result=QString(buffer).mid(strlen(info)+1);
			break;
		}
	}	
	fclose(f);
	return result;
}

void eDVB::setPID(PMTEntry *entry)
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
			for (QListIterator<Descriptor> i(entry->ES_info); i.current(); ++i)
			{
				Descriptor *d=i.current();
				if (d->Tag()==DESCR_AC3)
				{
					isaudio=1;
					isAC3=1;
				}
				if (d->Tag()==DESCR_TELETEXT)
					isteletext=1;
			}
		}
		}
		if (isaudio)
		{
			Decoder::parms.audio_type=isAC3?DECODE_AUDIO_AC3:DECODE_AUDIO_MPEG;
			Decoder::parms.apid=entry->elementary_PID;
		}
		if (isvideo)
			Decoder::parms.vpid=entry->elementary_PID;
		if (isteletext)
			Decoder::parms.tpid=entry->elementary_PID;
	}
}

void eDVB::setDecoder()
{
	serviceEvent(eventServiceNewPIDs);
}

PMT *eDVB::getPMT() 
{
	return tPMT.ready()?tPMT.getCurrent():0;
}

EIT *eDVB::getEIT()
{
	return tEIT.ready()?tEIT.getCurrent():0;
}

void eDVB::sortInChannels()
{
	qDebug("sorting in channels");
	removeDVBBouquets();
	for (QListIterator<eService> i(*getServices()); i.current(); ++i)
	{
		eService *service=i.current();
		QString add;
		switch (service->service_type)
		{
		case 1:
		case 4:
		case 5:
			add=" [TV]";
			break;
		case 2:
			add=" [Radio]";
			break;
		default:
			add=" [Data]";
		}
		eBouquet *b=createBouquet(beautifyBouquetName(service->service_provider)+add);
		b->add(service->transport_stream_id, service->original_network_id, service->service_id);
	}
	revalidateBouquets();
	saveBouquets();
}

void eDVB::saveServices()
{
	FILE *f=fopen(CONFIGDIR "/elitedvb/services", "wt");
	if (!f)
		qFatal("couldn't open servicefile - create " CONFIGDIR "/elitedvb!");
	fprintf(f, "eDVB services - modify as long as you pay for the damage!\n");
	fprintf(f, "transponders\n");
	for (QListIterator<eTransponder> i(*transponderlist->getTransponders()); i.current(); ++i)
	{
		eTransponder *t=i.current();
		if (t->state!=eTransponder::stateOK)
			continue;
		fprintf(f, "%04x:%04x %d\n", t->transport_stream_id, t->original_network_id, t->state);
		if (t->cable.valid)
			fprintf(f, "\tc %d:%d\n", t->cable.frequency, t->cable.symbol_rate);
		if (t->satellite.valid)
			fprintf(f, "\ts %d:%d:%d:%d:%d\n", t->satellite.frequency, t->satellite.symbol_rate, t->satellite.polarisation, t->satellite.fec, t->satellite.sat);
		fprintf(f, "/\n");
	}
	fprintf(f, "end\n");
	fprintf(f, "services\n");
	for (QListIterator<eService> i(*transponderlist->getServices()); i.current(); ++i)
	{
		eService *s=i.current();
		fprintf(f, "%04x:%04x:%04x:%d:%d\n", s->service_id, s->transport_stream_id,
				s->original_network_id, s->service_type, s->service_number);
		fprintf(f, "%s\n", (const char*)s->service_name);	// ich kille denjenigen, der ein \n in seinem servicenamen hat 
		fprintf(f, "%s\n", (const char*)s->service_provider);
	}
	fprintf(f, "end\n");
	fprintf(f, "Have a lot of fun!\n");
	fclose(f);
}

void eDVB::loadServices()
{
	FILE *f=fopen(CONFIGDIR "/elitedvb/services", "rt");
	if (!f)
		return;
	char line[256];
	if ((!fgets(line, 256, f)) || strncmp(line, "eDVB services", 13))
	{
		qDebug("not a servicefile");
		return;
	}
	qDebug("reading services");
	if ((!fgets(line, 256, f)) || strcmp(line, "transponders\n"))
	{
		qDebug("services invalid, no transponders");
		return;
	}
	if (transponderlist)
		delete transponderlist;
	transponderlist=new eTransponderList;

	while (!feof(f))
	{
		if (!fgets(line, 256, f))
			break;
		if (!strcmp(line, "end\n"))
			break;
		int transport_stream_id=-1, original_network_id=-1, state=-1;
		sscanf(line, "%04x:%04x %d", &transport_stream_id, &original_network_id, &state);
		eTransponder *t=transponderlist->create(transport_stream_id, original_network_id);
		t->state=state;
		while (!feof(f))
		{
			fgets(line, 256, f);
			if (!strcmp(line, "/\n"))
				break;
			if (line[1]=='s')
			{
				int frequency, symbol_rate, polarisation, fec, sat;
				sscanf(line+2, "%d:%d:%d:%d:%d", &frequency, &symbol_rate, &polarisation, &fec, &sat);
				t->setSatellite(frequency, symbol_rate, polarisation, fec, sat);
			}
			if (line[1]=='c')
			{
				int frequency, symbol_rate;
				sscanf(line+2, "%d:%d", &frequency, &symbol_rate);
				t->setCable(frequency, symbol_rate);
			}
		}
	}

	if ((!fgets(line, 256, f)) || strcmp(line, "services\n"))
	{
		qDebug("services invalid, no services");
		return;
	}

	while (!feof(f))
	{
		if (!fgets(line, 256, f))
			break;
		if (!strcmp(line, "end\n"))
			break;

		int service_id=-1, transport_stream_id=-1, original_network_id=-1, service_type=-1, service_number=-1;
		sscanf(line, "%04x:%04x:%04x:%d:%d", &service_id, &transport_stream_id, &original_network_id, &service_type, &service_number);
		eService *s=transponderlist->createService(transport_stream_id, original_network_id, service_id, service_number);
		s->service_type=service_type;
		fgets(line, 256, f);
		if (strlen(line))
			line[strlen(line)-1]=0;
		s->service_name=line;
		fgets(line, 256, f);
		if (strlen(line))
			line[strlen(line)-1]=0;
		s->service_provider=line;
	}
	
	qDebug("loaded %d services", transponderlist->getServices()->count());
	
	fclose(f);
	qDebug("ok");
}

void eDVB::saveBouquets()
{
	qDebug("saving bouquets...");
	
	FILE *f=fopen(CONFIGDIR "/elitedvb/bouquets", "wt");
	if (!f)
		qFatal("couldn't open bouquetfile - create " CONFIGDIR "/elitedvb!");
	fprintf(f, "eDVB bouquets - modify as long as you don't blame me!\n");
	fprintf(f, "bouquets\n");
	for (QListIterator<eBouquet> i(*getBouquets()); i.current(); ++i)
	{
		eBouquet *b=i.current();
		fprintf(f, "%0d\n", b->bouquet_id);
		fprintf(f, "%s\n", (const char*)b->bouquet_name);
		for (QListIterator<eServiceReference> s(b->list); s.current(); ++s)
			fprintf(f, "%04x:%04x:%04x\n", s.current()->service_id, s.current()->transport_stream_id, s.current()->original_network_id);
		fprintf(f, "/\n");
	}
	fprintf(f, "end\n");
	fprintf(f, "Have a lot of fun!\n");
	fclose(f);
	qDebug("done");
}

void eDVB::loadBouquets()
{
	FILE *f=fopen(CONFIGDIR "/elitedvb/bouquets", "rt");
	if (!f)
		return;
	char line[256];
	if ((!fgets(line, 256, f)) || strncmp(line, "eDVB bouquets", 13))
	{
		qDebug("not a bouquetfile");
		return;
	}
	qDebug("reading bouquets");
	if ((!fgets(line, 256, f)) || strcmp(line, "bouquets\n"))
	{
		qDebug("settings invalid, no transponders");
		return;
	}

	bouquets.clear();

	while (!feof(f))
	{
		if (!fgets(line, 256, f))
			break;
		if (!strcmp(line, "end\n"))
			break;
		int bouquet_id=-1;
		sscanf(line, "%d", &bouquet_id);
		if (!fgets(line, 256, f))
			break;
		line[strlen(line)-1]=0;
		eBouquet *bouquet=createBouquet(bouquet_id, line);
		while (!feof(f))
		{
			fgets(line, 256, f);
			if (!strcmp(line, "/\n"))
				break;
			int service_id=-1, transport_stream_id=-1, original_network_id=-1;
			sscanf(line, "%04x:%04x:%04x", &service_id, &transport_stream_id, &original_network_id);
			bouquet->add(transport_stream_id, original_network_id, service_id);
		}
	}

	qDebug("loaded %d bouquets", getBouquets()->count());
	
	fclose(f);
	
	revalidateBouquets();
	qDebug("ok");
}

void eDVB::changeVolume(int abs, int vol)
{
	switch (abs)
	{
	case 0:
		volume+=vol;
		mute=0;
		break;
	case 1:
		volume=vol;
		mute=0;
		break;
	case 2:
		if (vol)
			mute=!mute;
		break;
	case 3:
		mute=vol;
		break;
	}
	
	if (volume<0)
		volume=0;
	if (volume>63)
		volume=63;

	eAVSwitch::getInstance()->setVolume(mute?0:((63-volume)*65536/64));

	emit volumeChanged(mute?63:volume);
}

static void unpack(__u32 l, int *t)
{
	for (int i=0; i<4; i++)
		*t++=(l>>((3-i)*8))&0xFF;
}

void eDVB::configureNetwork()
{
	__u32 sip=0, snetmask=0, sdns=0, sgateway=0;
	int ip[4], netmask[4], dns[4], gateway[4];
	int sdosetup=0;

	eDVB::getInstance()->config.getKey("/elitedvb/network/ip", sip);
	eDVB::getInstance()->config.getKey("/elitedvb/network/netmask", snetmask);
	eDVB::getInstance()->config.getKey("/elitedvb/network/dns", sdns);
	eDVB::getInstance()->config.getKey("/elitedvb/network/gateway", sgateway);
	eDVB::getInstance()->config.getKey("/elitedvb/network/dosetup", sdosetup);

	unpack(sip, ip);
	unpack(snetmask, netmask);
	unpack(sdns, dns);
	unpack(sgateway, gateway);
	
	if (sdosetup)
	{
		FILE *f=fopen("/etc/resolv.conf", "wt");
		if (!f)
			qDebug("couldn't write resolv.conf");
		else
		{
			fprintf(f, "# Generated by EliteDVB\nnameserver %d.%d.%d.%d\n", dns[0], dns[1], dns[2], dns[3]);
			fclose(f);
		}
		QString buffer;
		buffer.sprintf("/sbin/ifconfig eth0 %d.%d.%d.%d up netmask %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3], netmask[0], netmask[1], netmask[2], netmask[3]);
		if (system(buffer)>>8)
			qDebug("'%s' failed.", (const char*)buffer);
		else
		{
			system("/sbin/route del default 2> /dev/null");
			buffer.sprintf("/sbin/route add default gw %d.%d.%d.%d", gateway[0], gateway[1], gateway[2], gateway[3]);
			if (system(buffer)>>8)
				qDebug("'%s' failed", (const char*)buffer);
		}
	}
}
