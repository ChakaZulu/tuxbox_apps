#include <asm/types.h>
#include <qdatetime.h>
#include "rc.h"
#include <stdio.h>

int eRCKey::getCompatibleCode() const
{
	switch (code&0xFF)
	{
		case 0: return eRCInput::RC_0;
		case 1: return eRCInput::RC_1;
    case 2: return eRCInput::RC_2;
		case 3: return eRCInput::RC_3;
		case 4: return eRCInput::RC_4;
		case 5: return eRCInput::RC_5;
		case 6: return eRCInput::RC_6;
		case 7: return eRCInput::RC_7;
		case 8: return eRCInput::RC_8;
		case 9: return eRCInput::RC_9;
		case 10: return eRCInput::RC_RIGHT;
		case 11: return eRCInput::RC_LEFT;
		case 12: return eRCInput::RC_UP;
		case 13: return eRCInput::RC_DOWN;
		case 14: return eRCInput::RC_OK;
		case 15: return eRCInput::RC_MUTE;
		case 16: return eRCInput::RC_STANDBY;
		case 17: return eRCInput::RC_GREEN;
		case 18: return eRCInput::RC_YELLOW;
		case 19: return eRCInput::RC_RED;
		case 20: return eRCInput::RC_BLUE;
		case 22: return eRCInput::RC_MINUS;
		case 21: return eRCInput::RC_PLUS;
		case 23: return eRCInput::RC_HELP;
		case 24: return eRCInput::RC_DBOX;
		case 31: return eRCInput::RC_HOME;
	}
	return -1;
}

eRCDevice::eRCDevice(eRCDriver *driver): driver(driver)
{
	input=driver->getInput();
	driver->addCodeListener(this);
}

eRCDevice::~eRCDevice()
{
	driver->removeCodeListener(this);
}

eRCDriver::eRCDriver(eRCInput *input): input(input)
{
	qDebug("creating eRCDriver");
}

eRCDriver::~eRCDriver()
{
	for (std::list<eRCDevice*>::iterator i=listeners.begin(); i!=listeners.end(); ++i)
		delete *i;
}

void eRCShortDriver::keyPressed(int)
{
	__u16 rccode;
	rc.readBlock((char*)&rccode, 2);
	for (std::list<eRCDevice*>::iterator i(listeners.begin()); i!=listeners.end(); ++i)
		(*i)->handleCode(rccode);
}

eRCShortDriver::eRCShortDriver(const char *filename): eRCDriver(eRCInput::getInstance())
{
	rc.setName(filename);
	if (!rc.open(IO_ReadOnly))
		qDebug("failed to open %s", filename);
	else
		qDebug("driver open success");
	sn=new QSocketNotifier(rc.handle(), QSocketNotifier::Read, this);
	connect(sn, SIGNAL(activated(int)), SLOT(keyPressed(int)));
}

#if 0
eRCDreamboxDriver::eRCDreamboxDriver(): eRCShortDriver("/dev/rawir")
{
}
#endif

eRCInput *eRCInput::instance;

eRCInput::eRCInput()
{
	instance=this;
}

eRCInput::~eRCInput()
{
}

int eRCInput::lock()
{
	return -1;
}

void eRCInput::unlock()
{
}

