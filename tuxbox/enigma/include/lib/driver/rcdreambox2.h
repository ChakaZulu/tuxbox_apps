#ifndef __rcdreambox2_h
#define __rcdreambox2_h

#include <core/driver/rc.h>

class eRCDeviceDreambox2: public eRCDevice
{
	int last, ccode;
	eTimer timeout, repeattimer;
private:
	void timeOut();
	void repeat();
public:
	void handleCode(int code);
	eRCDeviceDreambox2(eRCDriver *driver);
	const char *getDescription() const;
	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

class eRCDreamboxDriver2: public eRCShortDriver
{
public:
	eRCDreamboxDriver2();
};

#endif
