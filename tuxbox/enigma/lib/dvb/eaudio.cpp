#include <lib/dvb/eaudio.h>

#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/system/init.h>
#include <lib/system/econfig.h>

eAudio *eAudio::instance;

eAudio::eAudio()
{
	instance=this;

	reloadSettings();
}

eAudio::~eAudio()
{
	instance=0;
}

eAudio *eAudio::getInstance()
{
	return instance;
}

void eAudio::setAC3default(int a)
{
	eDVBServiceController *sapi=eDVB::getInstance()->getServiceAPI();

	ac3default=a;

	if (sapi)
		sapi->scanPMT();

}

void eAudio::saveSettings()
{
	eConfig::getInstance()->setKey("/elitedvb/audio/ac3default", ac3default);
}

void eAudio::reloadSettings()
{
	if (eConfig::getInstance()->getKey("/elitedvb/audio/ac3default", ac3default))
		ac3default=0;
}

eAutoInitP0<eAudio> init_eAudio(1, "EAUDIO");
