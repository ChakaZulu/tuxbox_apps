#include <asm/types.h>
#include "rc.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "init.h"
#include <core/system/econfig.h>

int eRCDevice::getKeyCompatibleCode(const eRCKey &) const
{
	return -1;
}

eRCDevice::eRCDevice(const char *id, eRCDriver *driver): id(id), driver(driver)
{
	input=driver->getInput();
	driver->addCodeListener(this);
	eRCInput::getInstance()->addDevice(id, this);
}

eRCDevice::~eRCDevice()
{
	driver->removeCodeListener(this);
	eRCInput::getInstance()->removeDevice(id);
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
		printf("failed to open %s\n", filename);
		sn=0;
	} else
	{
		sn=new eSocketNotifier(eApp, handle, eSocketNotifier::Read);
//		connect(sn, SIGNAL(activated(int)), SLOT(keyPressed(int)));
		CONNECT(sn->activated, eRCShortDriver::keyPressed);
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

eRCConfig::eRCConfig()
{
	reload();
}

void eRCConfig::reload()
{
	rdelay=500;
	rrate=100;
	eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rrate);
}

void eRCConfig::save()
{
	eConfig::getInstance()->setKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->setKey("/ezap/rc/repeatDelay", rrate);
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

void eRCInput::addDevice(const char *id, eRCDevice *dev)
{
	devices.insert(std::pair<const char*,eRCDevice*>(id, dev));
}

void eRCInput::removeDevice(const char *id)
{
	devices.erase(id);
}

eRCDevice *eRCInput::getDevice(const char *id)
{
	std::map<const char*,eRCDevice*>::iterator i=devices.find(id);
	if (i == devices.end())
		return 0;
	return i->second;
}

eAutoInitP0<eRCInput> init_rcinput(1, "RC Input layer");
