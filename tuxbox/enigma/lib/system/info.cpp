#include <lib/system/info.h>
#include <lib/base/estring.h>
#include <tuxbox.h>

eSystemInfo *eSystemInfo::instance;

eSystemInfo::eSystemInfo()
{
	instance=this;
}

int eSystemInfo::hasHDD()
{
	return 1;
}

int eSystemInfo::hasCI()
{
	return 1;
}

int eSystemInfo::hasRFMod()
{
	return 1;
}

int eSystemInfo::hasLCD()
{
	return 1;
}

int eSystemInfo::hasNetwork()
{
	return 1;
}

int eSystemInfo::canMeasureLNBCurrent()
{
	return 1;
}

std::set<int> eSystemInfo::getCAIDs()
{
	std::set<int> i;
	i.insert(0x1702);
	i.insert(0x1722);
	i.insert(0x1762);
	return i;
}

eString eSystemInfo::getVendorString()
{
	return eString("TMB");
}

eString eSystemInfo::getMachineString()
{
	return eString("killerBox");
}

eString eSystemInfo::getProcessorString()
{
	return eString("6502");
}
      
int eSystemInfo::isRelease()
{
	return 0;
}

int eSystemInfo::getAVS()
{

	switch (tuxbox_get_vendor())
	{
		case TUXBOX_VENDOR_NOKIA:
			return avsNokia;
		case TUXBOX_VENDOR_SAGEM:
			return avsSagem;
		case TUXBOX_VENDOR_PHILIPS:
			return avsPhilips;
		default:
			return avsDM7000;
	}
}

