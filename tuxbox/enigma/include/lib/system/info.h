#ifndef __lib_system_info_h
#define __lib_system_info_h

#include <set>

class eString;

class eSystemInfo
{
	static eSystemInfo *instance;
	int hashdd, hasci, hasrfmod, haslcd, hasnetwork,
	canmeasurelnbcurrent, hwtype, fetype;
	std::set<int> caids;
	eString getInfo(const char *info);
public:
	static eSystemInfo *getInstance() { return instance; }
	eSystemInfo();
	enum { dbox2Nokia, dbox2Sagem, dbox2Philips, DM7000, DM5600, Unknown };
	enum { feSatellite, feCable, feTerrestrial };

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
