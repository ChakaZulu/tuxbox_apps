#include <lib/system/info.h>

#include <lib/base/estring.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <lib/dvb/frontend.h>
#if HAVE_DVB_API_VERSION == 3
#include <tuxbox.h>
#endif

eSystemInfo *eSystemInfo::instance;

eSystemInfo::eSystemInfo()
	:hashdd(0), hasci(0), hasrfmod(0), haslcd(0), hasnetwork(1)
	,canmeasurelnbcurrent(0), hasnegfilter(0)
{
	instance=this;
#if HAVE_DVB_API_VERSION == 3
	int fd=::open(DEMOD_DEV, O_RDWR);
	if (fd>=0)
	{
		dvb_frontend_info info;
		if ( ::ioctl(fd, FE_GET_INFO, &info) >= 0 )
		{
			switch (info.type)
			{
				case FE_QPSK:
					fetype = feSatellite;
					break;
				case FE_QAM:
					fetype = feCable;
					break;
				default:
				case FE_OFDM:
					eDebug("COOL: dvb-t is out. less cool: enigma doesn't support it yet...");
					fetype = feCable;
					break;
			}
			::close (fd);
		}
		else
			eDebug("FE_GET_INFO failed (%m)");
	}
	else
		eDebug("open demod failed (%m)");

#else
	if ( eSystemInfo::getInstance()->getHwType() < eSystemInfo::DM7000 )
	{
		switch (atoi(getInfo("fe").c_str()))
		{
			case 0: // DBOX_FE_CABLE
				fetype=feCable;
				break;
			case 1: // DBOX_FE_SATELLITE
				fetype=feSatellite;
				break;
			default:
				fetype=feSatellite;
		}
	}
	else
	{
		int fd=::open(DEMOD_DEV, O_RDWR);
		if (fd>=0)
		{
			FrontendInfo info;
			if ( ::ioctl(fd, FE_GET_INFO, &info) >= 0 )
			{
				switch (info.type)
				{
					case FE_QPSK:
						fetype = feSatellite;
						break;
					case FE_QAM:
						fetype = feCable;
						break;
					default:
					case FE_OFDM:
						eDebug("COOL: dvb-t is out. less cool: enigma doesn't support it yet...");
						fetype = feCable;
						break;
				}
			}
			else
				eDebug("FE_GET_INFO failed (%m)");
			::close (fd);
		}
	}
#endif
#if HAVE_DVB_API_VERSION==3
	hasnegfilter=1;
	switch (tuxbox_get_submodel())
	{
		case TUXBOX_SUBMODEL_DREAMBOX_DM7000:
			hashdd = haslcd = hasnetwork = canmeasurelnbcurrent = 1;
			hasci = 1;
			hwtype = DM7000;
			caids.insert(0x4a70);
			break;
		case TUXBOX_SUBMODEL_DREAMBOX_DM5600:
			hwtype = DM5600;
			caids.insert(0x4a70);
			hasnetwork=0;
			break;
		case TUXBOX_SUBMODEL_DBOX2:
			caids.insert(0x1702);
			caids.insert(0x1722);
			caids.insert(0x1762);
			haslcd=1;
			switch ( tuxbox_get_vendor() )
			{
				case TUXBOX_VENDOR_NOKIA:
					hwtype = dbox2Nokia;
					break;
				case TUXBOX_VENDOR_SAGEM:
					hwtype = dbox2Sagem;
					break;
				case TUXBOX_VENDOR_PHILIPS:
					hwtype = dbox2Philips;
					break;
				default:
					hwtype = Unknown;
			}
			break;
		default:
			hwtype = Unknown;
			break;
	}
#else
	int mid = atoi(getInfo("mID").c_str());
	switch (mid)
	{
		case 5:
			hashdd = haslcd = hasnetwork = canmeasurelnbcurrent = hasci = 1;
			hwtype = DM7000;
			caids.insert(0x4a70);
			break;
		case 6:
			hasci = 2;
			hwtype = getInfo("type", true) == "DM5600" ? DM5600 : DM5620;
			if ( hwtype == DM5600 )
				hasnetwork=0;
			caids.insert(0x4a70);
			break;
		case 7:
			hasnetwork = 1;
			hwtype = DM500;
			caids.insert(0x4a70);
			break;
		case 1 ... 3:
			caids.insert(0x1702);
			caids.insert(0x1722);
			caids.insert(0x1762);
			haslcd=1;
			switch ( mid )
			{
				case 1:
					hwtype = dbox2Nokia;
					break;
				case 2:
					hwtype = dbox2Philips;
					break;
				case 3:
					hwtype = dbox2Sagem;
					break;
				default:
					break;
			}
		default:
			break;
	}
	int fd = open ("/dev/rfmod0", O_RDWR);
	if ( fd >= 0 )
	{
		hasrfmod=1;
		close(fd);
	}
	fd = open("/dev/dvb/card0/demux0", O_RDWR);
	if ( fd >=0 )
	{
		if ( ::ioctl( fd, DMX_SET_NEGFILTER_MASK, 0 ) >= 0 )
			hasnegfilter=1;
		close(fd);
  }
#endif
}

#if HAVE_DVB_API_VERSION < 3
eString eSystemInfo::getInfo(const char *info, bool dreambox)
{
	FILE *f=0;
	if ( dreambox )
		f=fopen("/proc/bus/dreambox", "rt");
	else
		f=fopen("/proc/bus/dbox", "rt");
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
#endif

eAutoInitP0<eSystemInfo> init_info(eAutoInitNumbers::sysinfo, "SystemInfo");
