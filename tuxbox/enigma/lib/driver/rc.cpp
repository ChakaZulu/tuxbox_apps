#include <asm/types.h>
#include <qdatetime.h>
#include "rc.h"
#include <stdio.h>
#include "init.h"

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
	{
		qDebug("failed to open %s", filename);
		sn=0;
	} else
	{
		sn=new QSocketNotifier(rc.handle(), QSocketNotifier::Read, this);
		connect(sn, SIGNAL(activated(int)), SLOT(keyPressed(int)));
		eRCInput::getInstance()->setFile(&rc);
	}
}

eRCShortDriver::~eRCShortDriver()
{
	rc.close();
	if (sn)
		delete sn;
}

eRCInput *eRCInput::instance;

eRCInput::eRCInput()
{
	instance=this;
	rc = 0;
	locked = 0;
}

eRCInput::~eRCInput()
{
}

void eRCInput::close()
{
	if (rc)
  	rc->close();
}

bool eRCInput::open()
{
	if (rc)
  	return rc->open(IO_ReadOnly);
	else
		return false;
}

int eRCInput::lock()
{
	if (locked || !rc || !rc->handle())	
		return -1;

	locked=1;

	return rc->handle();
}

void eRCInput::unlock()
{
	if (locked)
		locked=0;
}

void eRCInput::setFile(QFile* file)
{
	rc = file;
}

eAutoInitP0<eRCInput> init_rcinput(1, "RC Input layer");
