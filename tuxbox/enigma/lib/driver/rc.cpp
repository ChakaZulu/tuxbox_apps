#include <asm/types.h>
#include "rc.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
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
	while (1)
	{
		if (read(handle, &rccode, 2)!=2)
			break;
		for (std::list<eRCDevice*>::iterator i(listeners.begin()); i!=listeners.end(); ++i)
			(*i)->handleCode(rccode);
	}
}

eRCShortDriver::eRCShortDriver(const char *filename): eRCDriver(eRCInput::getInstance())
{
	handle=open(filename, O_RDONLY|O_NONBLOCK);
	if (handle<0)
	{
		qDebug("failed to open %s", filename);
		sn=0;
	} else
	{
		sn=new QSocketNotifier(handle, QSocketNotifier::Read, this);
		connect(sn, SIGNAL(activated(int)), SLOT(keyPressed(int)));
		eRCInput::getInstance()->setFile(handle);
	}
}

eRCShortDriver::~eRCShortDriver()
{
	if (handle>=0)
		close(handle);
	if (sn)
		delete sn;
}

eRCInput *eRCInput::instance;

eRCInput::eRCInput()
{
	instance=this;
	handle = -1;
	locked = 0;
}

eRCInput::~eRCInput()
{
}

void eRCInput::close()
{
}

bool eRCInput::open()
{
}

int eRCInput::lock()
{
	return handle;
}

void eRCInput::unlock()
{
	if (locked)
		locked=0;
}

void eRCInput::setFile(int newh)
{
	handle=newh;
}

eAutoInitP0<eRCInput> init_rcinput(1, "RC Input layer");
