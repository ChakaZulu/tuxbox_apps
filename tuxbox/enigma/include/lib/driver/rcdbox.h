#ifndef __rcdbox_h
#define __rcdbox_h

#include "rc.h"

class eRCDeviceDBoxOld: public eRCDevice
{
//	Q_OBJECT
	int last, ccode;
	eTimer timeout, repeattimer;
private:// slots:
	void timeOut();
	void repeat();
public:
	void handleCode(int code);
	eRCDeviceDBoxOld(eRCDriver *driver);
	const char *getDescription() const;

	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

class eRCDeviceDBoxNew: public eRCDevice
{
//	Q_OBJECT
	int last, ccode;
	eTimer timeout, repeattimer;
private:// slots:
	void timeOut();
	void repeat();
public:
	void handleCode(int code);
	eRCDeviceDBoxNew(eRCDriver *driver);
	const char *getDescription() const;

	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

class eRCDeviceDBoxButton: public eRCDevice
{
//	Q_OBJECT
	int last;
	eTimer repeattimer;
private:// slots:
	void repeat();
public:
	void handleCode(int code);
	eRCDeviceDBoxButton(eRCDriver *driver);
	const char *getDescription() const;

	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

class eRCDBoxDriver: public eRCShortDriver
{
public:
	eRCDBoxDriver();
};

#endif
