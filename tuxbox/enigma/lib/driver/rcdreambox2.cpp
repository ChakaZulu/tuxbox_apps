#include <lib/driver/rcdreambox2.h>
#include <dbox/fp.h>

#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/base/eerror.h>
#include <sys/time.h>

/* static void dumpreltime()
{
	static timeval lasttv;
	timeval tv;
	gettimeofday(&tv, 0);
	
	int ms=(tv.tv_sec - lasttv.tv_sec)*1000+(tv.tv_usec-lasttv.tv_usec)/1000;
	eDebug("+ %d ms", ms);
	lasttv=tv;
} */

	/* ----------------------- dreambox fernbedienung ---------------------- */
void eRCDeviceDreambox2::handleCode(int rccode)
{
	if (rccode == 0x00FF) // break code
	{
		timeout.stop();
		repeattimer.stop();
		timeOut();
		ccode=rccode;
		return;
	}
	
	timeout.start(1500, 1);
	int old=ccode;
	ccode=rccode;
	if ((old!=-1) && ( ((old&0x7FFF)!=(rccode&0x7FFF)) || !(rccode&0x8000)) )
		/*emit*/ input->keyPressed(eRCKey(this, (old&0x7FFF), eRCKey::flagBreak));
	if ((old^rccode)&0x7FFF)
	{
		repeattimer.start(eRCInput::getInstance()->config.rdelay, 1);
		input->keyPressed(eRCKey(this, rccode&0x7FFF, 0));
	}
}

void eRCDeviceDreambox2::timeOut()
{
	int oldcc=ccode;
	ccode=-1;
	repeattimer.stop();
	if (oldcc!=-1)
		input->keyPressed(eRCKey(this, oldcc&0x7FFF, eRCKey::flagBreak));
}

void eRCDeviceDreambox2::repeat()
{
	if (ccode!=-1)
		input->keyPressed(eRCKey(this, ccode&0x7FFF, eRCKey::flagRepeat));
	repeattimer.start(eRCInput::getInstance()->config.rrate, 1);
}

eRCDeviceDreambox2::eRCDeviceDreambox2(eRCDriver *driver)
			: eRCDevice("Dreambox2", driver), timeout(eApp), repeattimer(eApp)
{
	ccode=-1;
	CONNECT(timeout.timeout, eRCDeviceDreambox2::timeOut);
	CONNECT(repeattimer.timeout, eRCDeviceDreambox2::repeat);
}

const char *eRCDeviceDreambox2::getDescription() const
{
	return "dreambox Fernbedienung 2";
}

const char *eRCDeviceDreambox2::getKeyDescription(const eRCKey &key) const
{
	switch (key.code)
	{
	case 0x00: return "0";
	case 0x01: return "1";
	case 0x02: return "2";
	case 0x03: return "3";
	case 0x04: return "4";
	case 0x05: return "5";
	case 0x06: return "6";
	case 0x07: return "7";
	case 0x08: return "8";
	case 0x09: return "9";
	case 0x0a: return "volume up";
	case 0x0b: return "volume down";
	case 0x0c: return "tv";
	case 0x0d: return "bouquet up";
	case 0x0e: return "bouquet down";
	case 0x0f: return "standby";
	case 0x20: return "Dream";
	case 0x21: return "up";
	case 0x22: return "down";
	case 0x23: return "left";
	case 0x24: return "right";
	case 0x25: return "ok";
	case 0x26: return "audio";
	case 0x27: return "video";
	case 0x28: return "info";
	case 0x40: return "red";
	case 0x41: return "green";
	case 0x42: return "yellow";
	case 0x43: return "blue";
	case 0x44: return "mute";
	case 0x45: return "text";
	case 0x50: return "forward";
	case 0x51: return "back";
	case 0x52: return "lame";
	case 0x53: return "text";
	case 0x54: return "help";
	}
	return 0;
}

int eRCDeviceDreambox2::getKeyCompatibleCode(const eRCKey &key) const
{
	switch (key.code&0xFF)
	{
	case 0x00: return eRCInput::RC_0;
	case 0x01: return eRCInput::RC_1;
	case 0x02: return eRCInput::RC_2;
	case 0x03: return eRCInput::RC_3;
	case 0x04: return eRCInput::RC_4;
	case 0x05: return eRCInput::RC_5;
	case 0x06: return eRCInput::RC_6;
	case 0x07: return eRCInput::RC_7;
	case 0x08: return eRCInput::RC_8;
	case 0x09: return eRCInput::RC_9;
	case 0x0a: return eRCInput::RC_PLUS;
	case 0x0b: return eRCInput::RC_MINUS;
	case 0x0c: return eRCInput::RC_HOME;
	case 0x0d: return eRCInput::RC_PLUS;
	case 0x0e: return eRCInput::RC_MINUS;
	case 0x0f: return eRCInput::RC_STANDBY;
	case 0x20: return eRCInput::RC_DBOX;
	case 0x21: return eRCInput::RC_UP;
	case 0x22: return eRCInput::RC_DOWN;
	case 0x23: return eRCInput::RC_LEFT;
	case 0x24: return eRCInput::RC_RIGHT;
	case 0x25: return eRCInput::RC_OK;
	case 0x26: return eRCInput::RC_YELLOW;
	case 0x27: return eRCInput::RC_GREEN;
	case 0x28: return eRCInput::RC_HELP;
	case 0x40: return eRCInput::RC_RED;
	case 0x41: return eRCInput::RC_GREEN;
	case 0x42: return eRCInput::RC_YELLOW;
	case 0x43: return eRCInput::RC_BLUE;
	case 0x44: return eRCInput::RC_MUTE;
	case 0x45: return eRCInput::RC_HOME;
	case 0x50: return eRCInput::RC_RIGHT;
	case 0x51: return eRCInput::RC_LEFT;
	case 0x52: return eRCInput::RC_HELP;
	case 0x53: return eRCInput::RC_STANDBY;
	case 0x54: return eRCInput::RC_HELP;
	}
	return -1;
}

eRCDreamboxDriver2::eRCDreamboxDriver2(): eRCShortDriver("/dev/rawir2")
{
}

	/* ----------------------- dbox buttons ---------------------- */
void eRCDeviceDreamboxButton::handleCode(int code)
{
	code=(~code)&0x7;
	
	for (int i=0; i<4; i++)
		if ((last&~code) & (1<<i))
		{
			/*emit*/ input->keyPressed(eRCKey(this, i, eRCKey::flagBreak));
		} else if ((~last&code)&(1<<i))
		{
			/*emit*/ input->keyPressed(eRCKey(this, i, 0));
		}
	if (code)
		repeattimer.start(eRCInput::getInstance()->config.rdelay, 1);
	else
		repeattimer.stop();
	last=code;
}

void eRCDeviceDreamboxButton::repeat()
{
	for (int i=0; i<4; i++)
		if (last&(1<<i))
			/*emit*/ input->keyPressed(eRCKey(this, i, eRCKey::flagRepeat));
	repeattimer.start(eRCInput::getInstance()->config.rrate, 1);
}

eRCDeviceDreamboxButton::eRCDeviceDreamboxButton(eRCDriver *driver): eRCDevice("DreamboxButton", driver), repeattimer(eApp)
{
	last=0;
	CONNECT(repeattimer.timeout, eRCDeviceDreamboxButton::repeat);
}

const char *eRCDeviceDreamboxButton::getDescription() const
{
	return "dreambox Buttons";
}

const char *eRCDeviceDreamboxButton::getKeyDescription(const eRCKey &key) const
{
	switch (key.code)
	{
	case 1: return "down";
	case 2: return "up";
	case 3: return "power";
	default: return 0;
	}
}

int eRCDeviceDreamboxButton::getKeyCompatibleCode(const eRCKey &key) const
{
	switch (key.code)
	{
	case 1: return eRCInput::RC_LEFT;
	case 2: return eRCInput::RC_RIGHT;
	case 3: return eRCInput::RC_STANDBY;
	}
	return -1;
}

eRCDreamboxButtonDriver::eRCDreamboxButtonDriver(): eRCShortDriver("/dev/dbox/fpkeys0")
{
}

class eDreamboxRCHardware2
{
	eRCDreamboxButtonDriver buttondriver;
	eRCDeviceDreamboxButton buttondevice;
	eRCDreamboxDriver2 driver;
	eRCDeviceDreambox2 device;
public:
	eDreamboxRCHardware2(): buttondevice(&buttondriver), device(&driver)
	{
	}
};

eAutoInitP0<eDreamboxRCHardware2> init_rcdreambox2(eAutoInitNumbers::rc+1, "DreamBox RC Hardware 2");
