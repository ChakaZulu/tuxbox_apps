#include <config.h>
#if HAVE_DVB_API_VERSION == 3

#ifndef __rcdbox_h
#define __rcdbox_h

#include <linux/input.h>
#include <lib/driver/rc.h>

class eRCDeviceInputDev: public eRCDevice
{
	struct input_event cur;
	eTimer repeattimer;
public:
	void repeat();
	void handleCode(int code);
	eRCDeviceInputDev(eRCInputEventDriver *driver);
	const char *getDescription() const;

	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

#endif // __rcdbox_h

#endif // API_V3
