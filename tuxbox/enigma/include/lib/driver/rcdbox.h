#ifndef __rcdbox_h
#define __rcdbox_h

#include <lib/driver/rc.h>

class eRCDeviceInputDev: public eRCDevice
{
	int last, ccode;
	eTimer timeout, repeattimer;
private:
	void timeOut();
	void repeat();
public:
	void handleCode(int code);
	eRCDeviceInputDev(eRCDriver *driver);
	const char *getDescription() const;

	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

class eRCInputDevDriver: public eRCInputEventDriver
{
public:
	eRCInputDevDriver();
};

#endif
