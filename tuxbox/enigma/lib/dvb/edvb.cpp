#include <lib/dvb/edvb.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dbox/info.h>
#include <tuxbox/tuxbox.h>
#include <algorithm>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifdef PROFILE
	#include <sys/time.h>
#endif

#include <lib/driver/eavswitch.h>
#include <lib/driver/streamwd.h>
#include <lib/driver/rfmod.h>
#include <lib/dvb/esection.h>
#include <lib/dvb/si.h>
#include <lib/dvb/frontend.h>
#include <lib/dvb/dvb.h>
#include <lib/dvb/decoder.h>
#include <lib/dvb/record.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/system/econfig.h>
#include <lib/dvb/dvbci.h>

#include <lib/dvb/dvbservice.h>
#include <lib/dvb/dvbscan.h>

eDVBController::~eDVBController()
{
}

eDVB *eDVB::instance;

eString eDVB::getVersion()
{
	return "eDVB lib 1.0, compiled " __DATE__;
}

void eDVB::event(const eDVBEvent &event)
{
	eventOccured(event);
	if (controller)
		controller->handleEvent(event);
}

void eDVB::tunedIn(eTransponder *trans, int err)
{
	event(eDVBEvent(eDVBEvent::eventTunedIn, err, trans));
}

eDVB::eDVB()
	: state(eDVBState::stateIdle), parentEIT(0)
{
	settings=0;
	time_difference=0;

		// singleton
	if (instance)
		eFatal("eDVB already initialized!");
	instance=this;

	DVBCI=new eDVBCI();
	DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::start));

	DVBCI2=new eDVBCI();
	DVBCI2->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::start));

		// initialize frontend (koennte auch nochmal raus)
	eString frontend=getInfo("fe");
	int fe;
	if (!frontend.length())
	{
		eDebug("WARNING: couldn't determine frontend-type, assuming satellite...");
		fe=eFrontend::feSatellite;
	} else
	{
		switch(atoi(frontend.c_str()))
		{
		case DBOX_FE_CABLE:
			fe=eFrontend::feCable;
			break;
		case DBOX_FE_SAT:
			fe=eFrontend::feSatellite;
			break;
		default:
			eDebug("COOL: dvb-t is out. less cool: eDVB doesn't support it yet...");
			fe=eFrontend::feCable;
			break;
		}
	}
	if (eFrontend::open(fe)<0)
		eFatal("couldn't open frontend");

	settings = new eDVBSettings(*this);
		// tuned-in handling
	CONNECT(eFrontend::getInstance()->tunedIn, eDVB::tunedIn);

		// decoder init .... already done!
//	Decoder::Initialize();
	
		// set to idle state
	controller=0;
	setState(eDVBState(eDVBState::stateIdle));
	
		// intialize to service mode
	setMode(controllerService);

		// init AV switch
	switch (tuxbox_get_model())
	{
	case TUXBOX_MODEL_DBOX2:
		switch (tuxbox_get_vendor())
		{
		case TUXBOX_VENDOR_NOKIA:
			new eAVSwitchNokia;
			break;
		case TUXBOX_VENDOR_PHILIPS:
			new eAVSwitchPhilips;
			break;
		case TUXBOX_VENDOR_SAGEM:
			new eAVSwitchSagem;
			break;
		default:
			new eAVSwitchNokia;
			break;
		}
	break;
	case TUXBOX_MODEL_DREAMBOX_DM5600:
		new eRFmod;
		eRFmod::getInstance()->init();
	case TUXBOX_MODEL_DREAMBOX_DM7000:
		new eAVSwitchNokia;
		break;
	default:
		new eAVSwitchNokia;
		break;
	}

	// init stream watchdog
	eStreamWatchdog::getInstance()->reloadSettings();

//	tMHWEIT=0;

		// init dvb recorder
	recorder=0;

		
	eDebug("eDVB::eDVB done.");
}

eDVB::~eDVB()
{
	delete settings; 
	recEnd();

	eAVSwitch::getInstance()->setActive(0);
	delete eAVSwitch::getInstance();
	
	if(eRFmod::getInstance())
		delete eRFmod::getInstance();
		
	Decoder::Close();

	eFrontend::close();
	instance=0;
}

void eDVB::recMessage(int msg)
{
	switch (msg)
	{
	case eDVBRecorder::recWriteError:
		event(eDVBEvent(eDVBEvent::eventRecordWriteError));
		break;
	}
}

eString eDVB::getInfo(const char *info)
{
	FILE *f=fopen("/proc/bus/dbox", "rt");
	if (!f)
		return "";
	eString result;
	while (1)
	{
		char buffer[128];
		if (!fgets(buffer, 128, f))
			break;
		if (strlen(buffer))
			buffer[strlen(buffer)-1]=0;
		if ((!strncmp(buffer, info, strlen(info)) && (buffer[strlen(info)]=='=')))
		{
			int i = strlen(info)+1;
			result = eString(buffer).mid(i, strlen(buffer)-i);
			break;
		}
	}	
	fclose(f);
	return result;
}

		// for external access only
PMT *eDVB::getPMT()
{
	return tPMT.ready()?tPMT.getCurrent():0;
}

EIT *eDVB::getEIT()
{
	if ( tEIT.ready() )
		return tEIT.getCurrent();
	else if ( parentEIT )
	{
		parentEIT->lock();
		return parentEIT;
	}
	else
		return 0;
}

SDT *eDVB::getSDT()
{
	return tSDT.ready()?tSDT.getCurrent():0;
}

static void unpack(__u32 l, int *t)
{
	for (int i=0; i<4; i++)
		*t++=(l>>((3-i)*8))&0xFF;
}

void eDVB::configureNetwork()
{
	struct in_addr sinet_address, snameserver, sinet_gateway;
	int type, sinet_netmask;
	char buf[128];
	int sdosetup=0;

	eConfig::getInstance()->getKey("/elitedvb/network/type", type);
	eConfig::getInstance()->getKey("/elitedvb/network/inet/address", sinet_address.s_addr);
	eConfig::getInstance()->getKey("/elitedvb/network/inet/netmask", sinet_netmask);
	eConfig::getInstance()->getKey("/elitedvb/network/inet/gateway", sinet_gateway.s_addr);
	eConfig::getInstance()->getKey("/elitedvb/network/nameserver", snameserver.s_addr);
	eConfig::getInstance()->getKey("/elitedvb/network/dosetup", sdosetup);

	if (sdosetup)
	{
		FILE *f = fopen("/etc/resolv.conf", "wt");
		if (!f)
			eDebug("couldn't write resolv.conf");
		else
		{
			fputs("# Generated by enigma\n\n", f);
			inet_ntop(AF_INET, &snameserver, buf, 128);
			fprintf(f, "nameserver %s\n", buf);
			fclose(f);
		}
		f = fopen("/etc/network/interfaces", "wt");
		if (!f)
			eDebug("couldn't write /etc/network/interfaces");
		else
		{
			fputs("# Generated by enigma\n\n", f);
			fputs("auto lo\niface lo inet loopback\n", f);
			fputs("auto eth0\n\n", f);
			if (type == 1)
				fputs("iface eth0 inet dhcp\n", f);
			else
			{
				fputs("iface eth0 inet static\n", f);
				inet_ntop(AF_INET, &sinet_address, buf, 128);
				fprintf(f, "address %s\n", buf);
				fprintf(f, "netmask %d\n", sinet_netmask);
				inet_ntop(AF_INET, &sinet_gateway, buf, 128);
				fprintf(f, "gateway %s\n", buf);
			}
			fclose(f);
		}
		system("/sbin/ifdown eth0");
		system("/sbin/ifup eth0");
	}
}

void eDVB::recBegin(const char *filename, eServiceReferenceDVB service)
{
	if (recorder)
		recEnd();
	recorder=new eDVBRecorder();
	recorder->open(filename);
	CONNECT(recorder->recMessage, eDVB::recMessage);
	
	PMT *pmt=getPMT();
	if (!pmt)
	{
		if (Decoder::parms.apid != -1)
			recorder->addPID(Decoder::parms.apid);
		if (Decoder::parms.vpid != -1)
			recorder->addPID(Decoder::parms.vpid);
		if (Decoder::parms.tpid != -1)
			recorder->addPID(Decoder::parms.tpid);
		if (Decoder::parms.pcrpid != -1)
			recorder->addPID(Decoder::parms.pcrpid);
	} else
	{
		recorder->addPID(pmt->PCR_PID);
		for (ePtrList<PMTEntry>::iterator i(pmt->streams); i != pmt->streams.end(); ++i)
			recorder->addPID(i->elementary_PID);
		pmt->unlock();
	}
	
	recorder->addPID(0);	// PAT
	if (Decoder::parms.pmtpid != -1)
		recorder->addPID(Decoder::parms.pmtpid);
		
	// build SMI table.
	
	// build SIT:
	__u8 sit[4096];
	int pos=0;
	sit[pos++]=0x7F;              // TID
	sit[pos++]=0x80;              // section_syntax_indicator, length
	sit[pos++]=0;                 // length
	sit[pos++]=0; sit[pos++]=0;   // reserved
	sit[pos++]=1;                 // reserved, version number, current/next indicator
	sit[pos++]=0;                 // section number
	sit[pos++]=0;                 // last section number
	sit[pos++]=0;                 // loop length
	sit[pos++]=0;                 // "    "
	int loop_pos=pos;
		// TODO: build Partitial Transport Stream descriptor containing valid values about rate
	sit[loop_pos-2]|=(pos-loop_pos)>>8;
	sit[loop_pos-1]|=(pos-loop_pos)&0xFF;
	
	// create one single entry: our service...
	sit[pos++]=service.getServiceID().get()>>8;
	sit[pos++]=service.getServiceID().get()&0xFF;
	sit[pos++]=3<<4;              // running state
	sit[pos++]=0;
	loop_pos=pos;

	// create our special descriptor:
	sit[pos++]=0x80;              // private	
	sit[pos++]=0;
	int descr_pos=pos;
	memcpy(sit+pos, "ENIGMA", 6);
	pos+=6;

	if (Decoder::parms.vpid != -1)
	{
		sit[pos++]=eServiceDVB::cVPID;
		sit[pos++]=2;
		sit[pos++]=Decoder::parms.vpid>>8;
		sit[pos++]=Decoder::parms.vpid&0xFF;
	}

	if (Decoder::parms.apid != -1)
	{
		sit[pos++]=eServiceDVB::cAPID;
		sit[pos++]=3;
		sit[pos++]=Decoder::parms.apid>>8;
		sit[pos++]=Decoder::parms.apid&0xFF;
		sit[pos++]=Decoder::parms.audio_type;
	}
	
	if (Decoder::parms.tpid != -1)
	{
		sit[pos++]=eServiceDVB::cTPID;
		sit[pos++]=2;
		sit[pos++]=Decoder::parms.tpid>>8;
		sit[pos++]=Decoder::parms.tpid&0xFF;
	}

	if (Decoder::parms.pcrpid != -1)
	{
		sit[pos++]=eServiceDVB::cPCRPID;
		sit[pos++]=2;
		sit[pos++]=Decoder::parms.pcrpid>>8;
		sit[pos++]=Decoder::parms.pcrpid&0xFF;
	}

	sit[descr_pos-2]|=(pos-descr_pos)>>8;
	sit[descr_pos-1]|=(pos-descr_pos)&0xFF;

		// fix lengths
	sit[loop_pos-2]|=(pos-loop_pos)>>8;
	sit[loop_pos-1]|=(pos-loop_pos)&0xFF;
	
		// add CRC (calculate later)
	sit[pos++]=0;
	sit[pos++]=0;
	sit[pos++]=0;
	sit[pos++]=0;

		// fix length
	sit[1]|=(pos-3)>>8;
	sit[2]|=(pos-3)&0xFF;

	// generate CRC :)
	
	// write section.
	recorder->writeSection(sit, 0x1f);
}

void eDVB::recPause()
{
	recorder->stop();
}

void eDVB::recResume()
{
	recorder->start();
}

void eDVB::recEnd()
{
	if (!recorder)
		return;
	recorder->close();
	delete recorder;
	recorder=0;
}

void eDVB::setMode(int mode)
{
	if (controllertype == mode)
		return;
	controllertype = mode;
	if (controller)
		delete controller;
	switch (mode)
	{
	case controllerScan:
		controller = new eDVBScanController(*this);
		break;
	case controllerService:
		controller = new eDVBServiceController(*this);
		DVBCI->messages.send(eDVBCI::eDVBCIMessage(eDVBCI::eDVBCIMessage::getcaids));
		break;
	}
}

eDVBServiceController *eDVB::getServiceAPI()
{
	if (controllertype != controllerService)
		return 0;
	return (eDVBServiceController*)controller;
}

eDVBScanController *eDVB::getScanAPI()
{
	if (controllertype != controllerScan)
		return 0;
	return (eDVBScanController*)controller;
}

eAutoInitP0<eDVB> init_dvb(eAutoInitNumbers::dvb, "eDVB lib");
