#ifndef __lib_system_info_h
#define __lib_system_info_h

#include <set>

class eString;

class eSystemInfo
{
	static eSystemInfo *instance;
public:
	static eSystemInfo *getInstance() { return instance; }
	eSystemInfo();

	int hasHDD();
	int hasCI();
	int hasRFMod();
	int hasLCD();
	int hasNetwork();
	int canMeasureLNBCurrent();
	
	std::set<int> getCAIDs();
	
	eString getVendorString();
	eString getMachineString();
	eString getProcessorString();
	
	int isRelease();
	enum { avsNokia, avsSagem, avsPhilips, avsDM7000, avsDM5600 };
	int getAVS();
};

#endif
