#ifndef __lib_driver_rcconsole_h
#define __lib_driver_rcconsole_h

#include <lib/driver/rc.h>

class eRCConsoleDriver: public eRCDriver
{
	struct termios ot;
protected:
	int handle;
	eSocketNotifier *sn;
	void keyPressed(int);
public:
	eRCConsoleDriver(const char *filename);
	~eRCConsoleDriver();
};

class eRCConsole: public eRCDevice
{
public:
	void handleCode(int code);
	eRCConsole(eRCDriver *driver);
	const char *getDescription() const;
	const char *getKeyDescription(const eRCKey &key) const;
	int getKeyCompatibleCode(const eRCKey &key) const;
};

#endif
