#ifndef __rcdbox_h
#define __rcdbox_h

#include "rc.h"

class eRCDeviceDBoxOld: public eRCDevice
{
	Q_OBJECT
	int last, ccode;
	QTimer timeout, repeattimer;
private slots:
	int timeOut();
	int repeat();
public:
	void handleCode(int code);
	eRCDeviceDBoxOld(eRCDriver *driver);
	const char *getDescription() const;
};

class eRCDeviceDBoxNew: public eRCDevice
{
	Q_OBJECT
	int last, ccode;
	QTimer timeout, repeattimer;
private slots:
	int timeOut();
	int repeat();
public:
	void handleCode(int code);
	eRCDeviceDBoxNew(eRCDriver *driver);
	const char *getDescription() const;
	int getCompatibleCode() const;
};

class eRCDeviceDBoxButton: public eRCDevice
{
	Q_OBJECT
	int last;
	QTimer repeattimer;
private slots:
	int repeat();
public:
	void handleCode(int code);
	eRCDeviceDBoxButton(eRCDriver *driver);
	const char *getDescription() const;
};

class eRCDBoxDriver: public eRCShortDriver
{
public:
	eRCDBoxDriver();
};

class eRCKeyDBoxOld: public eRCKey
{
public:
	const char *getDescription() const;
	int getCompatibleCode() const;
	eRCKeyDBoxOld(eRCDevice *producer, int code, int flags)
			: eRCKey(producer, code, flags)
	{
	}
};

class eRCKeyDBoxNew: public eRCKey
{
public:
	const char *getDescription() const;
	int getCompatibleCode() const;
	eRCKeyDBoxNew(eRCDevice *producer, int code, int flags)
			: eRCKey(producer, code, flags)
	{
	}
};

class eRCKeyDBoxButton: public eRCKey
{
public:
	const char *getDescription() const;
	eRCKeyDBoxButton(eRCDevice *producer, int code, int flags)
			: eRCKey(producer, code, flags)
	{
	}
};

#endif
