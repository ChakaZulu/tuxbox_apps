#include <lib/driver/streamwd.h>
#include <lib/driver/eavswitch.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "ost/video.h"
#include <dbox/event.h>

#include <lib/base/eerror.h>
#include <lib/dvb/edvb.h>
#include <lib/system/init.h>
#include <lib/system/econfig.h>

#define EVENT_DEVICE "/dev/dbox/event0"
#define WDE_VIDEOMODE 		(uint)1
#define WDE_VCRONOFF 			(uint)2

eStreamWatchdog *eStreamWatchdog::instance;

eStreamWatchdog::eStreamWatchdog()
{
	sn=0;
	handle=open( EVENT_DEVICE, O_RDONLY | O_NONBLOCK );

	if (handle<0)
	{
		eDebug("failed to open %s", EVENT_DEVICE);
		sn=0;
	}
	else
	{
		if ( ioctl(handle, EVENT_SET_FILTER, EVENT_ARATIO_CHANGE) < 0 )
		{
			perror("ioctl");
			close(handle);
		}
		else
		{
			sn=new eSocketNotifier(eApp, handle, eSocketNotifier::Read);
			CONNECT(sn->activated, eStreamWatchdog::check);
		}
	}

	if (!instance)
		instance=this;
	isanamorph=0;
}

eStreamWatchdog *eStreamWatchdog::getInstance()
{
	return instance;
}

void eStreamWatchdog::check(int)
{
	struct event_t event;
	int eventSize = sizeof (event);
	int status;
	while ((status = read(handle, &event, eventSize)) == eventSize)
		if (event.event == EVENT_ARATIO_CHANGE)
			reloadSettings();
}

void eStreamWatchdog::reloadSettings()
{
	FILE *bitstream=fopen("/proc/bus/bitstream", "rt");
	if (bitstream)
	{
		char buffer[100];
		int aspect=0;
		while (fgets(buffer, 100, bitstream))
		{
			if (!strncmp(buffer, "A_RATIO: ", 9))
				aspect=atoi(buffer+9);
		}
		fclose(bitstream);
		switch (aspect)
		{
			case 1:
			case 2:
				isanamorph=0;
			break;
			case 3:
			case 4:
				isanamorph=1;
		}
	}
	
	eDebug("Aratio changed");			
		/*emit*/ AspectRatioChanged(isanamorph);

	int videoDisplayFormat=VIDEO_LETTER_BOX;
	int doanamorph=0;
	unsigned int pin8;
	eConfig::getInstance()->getKey("/elitedvb/video/pin8", pin8);
	switch (pin8)
	{
	case 0:
		doanamorph=0;
		videoDisplayFormat=isanamorph?VIDEO_LETTER_BOX:VIDEO_PAN_SCAN;
		break;
	case 1:
		doanamorph=0;
		videoDisplayFormat=VIDEO_PAN_SCAN;
 		break;
	case 2:
		doanamorph=isanamorph;
		videoDisplayFormat=isanamorph?VIDEO_CENTER_CUT_OUT:VIDEO_PAN_SCAN;
		break;
	}

	eAVSwitch::getInstance()->setVideoFormat( videoDisplayFormat );

	eAVSwitch::getInstance()->setAspectRatio(doanamorph?r169:r43);
}

int eStreamWatchdog::isAnamorph()
{
	return isanamorph;
}

eStreamWatchdog::~eStreamWatchdog()
{
	if (instance==this)
		instance=0;

	if (handle>=0)
		close(handle);

	if (sn)
		delete sn;
}

eAutoInitP0<eStreamWatchdog> eStreamWatchdog_init(3, "stream watchdog");
