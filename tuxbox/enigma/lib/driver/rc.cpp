#include <asm/types.h>
#include <qdatetime.h>
#include "rc.h"
#include <stdio.h>

int eRCKey::getCompatibleCode() const
{
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
	{
		qDebug("driver open success");
		eRCInput::getInstance()->setHandle(rc.handle());
	}
	sn=new QSocketNotifier(rc.handle(), QSocketNotifier::Read, this);
	connect(sn, SIGNAL(activated(int)), SLOT(keyPressed(int)));
}

eRCInput *eRCInput::instance;

eRCInput::eRCInput()
{
	instance=this;
	locked = 0;
}

eRCInput::~eRCInput()
{
}

int eRCInput::lock()
{
	if (locked || !handle)	
		return -1;

	locked=1;

	return handle;
}

void eRCInput::unlock()
{
	if (locked)
		locked=0;
}

void eRCInput::setHandle(int hRC)
{
	handle=hRC;
}