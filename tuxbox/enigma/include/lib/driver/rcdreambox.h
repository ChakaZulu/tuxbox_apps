#ifndef __rcdreambox_h
#define __rcdreambox_h

#include "rc.h"

class eRCDeviceDreambox: public eRCDevice
{
//	Q_OBJECT
	int last, ccode;
	QTimer timeout, repeattimer;
private:// slots:
	void timeOut();
	void repeat();
public:
	void handleCode(int code);
	eRCDeviceDreambox(eRCDriver *driver);
	const char *getDescription() const;
};

class eRCDreamboxDriver: public eRCShortDriver
{
public:
	eRCDreamboxDriver();
};

class eRCKeyDreambox: public eRCKey
{
public:
	const char *getDescription() const;
	eRCKeyDreambox(eRCDevice *producer, int code, int flags)
			: eRCKey(producer, code, flags)
	{
	}
 int getCompatibleCode() const;
};

#endif
