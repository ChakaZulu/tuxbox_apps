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
	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

class eRCDreamboxDriver: public eRCShortDriver
{
public:
	eRCDreamboxDriver();
};

#endif
