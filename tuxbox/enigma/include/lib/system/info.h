#ifndef __lib_system_info_h
#define __lib_system_info_h

#include <set>
#include <config.h>

#if HAVE_DVB_API_VERSION < 3
#include <ost/dmx.h>
#ifndef DMX_SET_NEGFILTER_MASK
	#define DMX_SET_NEGFILTER_MASK   _IOW('o',48,uint8_t *)
#endif
#endif

class eString;

class eSystemInfo
{
	static eSystemInfo *instance;
	int hashdd, hasci, hasrfmod, haslcd, hasnetwork,
	canmeasurelnbcurrent, hwtype, fetype, hasnegfilter;
	std::set<int> caids;
	eString getInfo(const char *info, bool dreambox=false);
public:
	static eSystemInfo *getInstance() { return instance; }
	eSystemInfo();
	enum { dbox2Nokia, dbox2Sagem, dbox2Philips, DM7000, DM5600, DM5620, DM500, Unknown };
	enum { feSatellite, feCable, feTerrestrial };

	int hasNegFilter() { return hasnegfilter; }
	int hasHDD() { return hashdd; }
	int hasCI() { return hasci; }
	int hasRFMod() { return hasrfmod; }
	int hasLCD() { return haslcd; }
	int hasNetwork() { return hasnetwork; }
	int canMeasureLNBCurrent() { return canmeasurelnbcurrent; }
	int getHwType() { return hwtype; }
	int getFEType() { return fetype; }
	const std::set<int> &getCAIDs() { return caids; }
};

#endif
