#include <sys/ioctl.h>
#include <dbox/fp.h>
#include "rcdbox.h"
#include "init.h"

	/* ----------------------- alte fernbedienung ---------------------- */
void eRCDeviceDBoxOld::handleCode(int rccode)
{
	if ((rccode&0xFF00)!=0x5C00)
		return;
	if (rccode==0x5CFE)		// old break code
	{
		timeout.stop();
		repeattimer.stop();
		if (ccode!=-1)
		{
			int old=ccode;
			ccode=-1;
			input->keyPressed(eRCKeyDBoxOld(this, old, eRCKey::flagBreak));
		}
	} else // if (rccode!=ccode)
	{
		timeout.start(300, 1);
		int old=ccode;
		ccode=rccode;
		if ((old!=-1) && (old!=rccode))
			emit input->keyPressed(eRCKeyDBoxOld(this, old, eRCKey::flagBreak));
		if (old != rccode)
		{
			repeattimer.start(rdelay, 1);
			input->keyPressed(eRCKeyDBoxOld(this, rccode, 0));
		}
	}
}

int eRCDeviceDBoxOld::timeOut()
{
	int oldcc=ccode;
	ccode=-1;
	repeattimer.stop();
	if (oldcc!=-1)
		input->keyPressed(eRCKeyDBoxOld(this, oldcc, eRCKey::flagBreak));
	return 0;
}

int eRCDeviceDBoxOld::repeat()
{
	if (ccode!=-1)
		input->keyPressed(eRCKeyDBoxOld(this, ccode, eRCKey::flagRepeat));
	repeattimer.start(rrate, 1);
	return 0;
}

eRCDeviceDBoxOld::eRCDeviceDBoxOld(eRCDriver *driver): eRCDevice(driver)
{
	ccode=-1;
	rrate=100;
	rdelay=200;
	connect(&timeout, SIGNAL(timeout()), SLOT(timeOut()));
	connect(&repeattimer, SIGNAL(timeout()), SLOT(repeat()));
}

const char *eRCDeviceDBoxOld::getDescription() const
{
	return "alte d-box Fernbedienung";
}

	/* ----------------------- neue fernbedienung ---------------------- */
void eRCDeviceDBoxNew::handleCode(int rccode)
{
	if ((rccode&0xFF00)!=0x0000)
		return;
	timeout.start(300, 1);
	int old=ccode;
	ccode=rccode;
	if ((old!=-1) && (old!=rccode))
		emit input->keyPressed(eRCKeyDBoxNew(this, old&0x3F, eRCKey::flagBreak));
	if (old != rccode)
	{
		repeattimer.start(rdelay, 1);
		input->keyPressed(eRCKeyDBoxNew(this, rccode&0x3F, 0));
	}
}

int eRCDeviceDBoxNew::timeOut()
{
	int oldcc=ccode;
	ccode=-1;
	repeattimer.stop();
	if (oldcc!=-1)
		input->keyPressed(eRCKeyDBoxNew(this, oldcc&0x3F, eRCKey::flagBreak));
	return 0;
}

int eRCDeviceDBoxNew::repeat()
{
	if (ccode!=-1)
		input->keyPressed(eRCKeyDBoxNew(this, ccode&0x3F, eRCKey::flagRepeat));
	repeattimer.start(rrate, 1);
	return 0;
}

eRCDeviceDBoxNew::eRCDeviceDBoxNew(eRCDriver *driver): eRCDevice(driver)
{
	ccode=-1;
	rrate=100;
	rdelay=400;
	connect(&timeout, SIGNAL(timeout()), SLOT(timeOut()));
	connect(&repeattimer, SIGNAL(timeout()), SLOT(repeat()));
}

const char *eRCDeviceDBoxNew::getDescription() const
{
	return "neue d-box Fernbedienung";
}

	/* ----------------------- dbox buttons ---------------------- */
void eRCDeviceDBoxButton::handleCode(int code)
{
	if ((code&0xFF00)!=0xFF00)
		return;
	
	code=(~code)&0xF;
	
	for (int i=0; i<4; i++)
		if ((last&~code) & (1<<i))
			emit input->keyPressed(eRCKeyDBoxButton(this, i, eRCKey::flagBreak));
		else if ((~last&code)&(1<<i))
			emit input->keyPressed(eRCKeyDBoxButton(this, i, 0));
	if (code)
		repeattimer.start(rdelay, 1);
	else
		repeattimer.stop();
	last=code;
}

int eRCDeviceDBoxButton::repeat()
{
	for (int i=0; i<4; i++)
		if (last&(1<<i))
			emit input->keyPressed(eRCKeyDBoxButton(this, i, eRCKey::flagRepeat));
	repeattimer.start(rrate, 1);
	return 0;
}

eRCDeviceDBoxButton::eRCDeviceDBoxButton(eRCDriver *driver): eRCDevice(driver)
{
	rrate=100;
	rdelay=300;
	last=0;
	connect(&repeattimer, SIGNAL(timeout()), SLOT(repeat()));
}

const char *eRCDeviceDBoxButton::getDescription() const
{
	return "d-box Buttons";
}

eRCDBoxDriver::eRCDBoxDriver(): eRCShortDriver("/dev/dbox/rc0")
{
	if (rc.handle()>0)
		ioctl(rc.handle(), RC_IOCTL_BCODES, 1);
}

const char *eRCKeyDBoxOld::getDescription() const
{
	if ((code&0xFF00)!=0x5C00)
		return 0;
	switch (code&0xFF)
	{
	case 0x0C: return "power";
	case 0x20: return "home";
	case 0x27: return "d-box";
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
	case 0x3B: return "Blau";
	case 0x52: return "Gelb";
	case 0x55: return "Gruen";
	case 0x2D: return "Rot";
	case 0x54: return "Doppelpfeil oben";
	case 0x53: return "Doppelpfeil unten";
	case 0x0E: return "oben";
	case 0x0F: return "unten";
	case 0x2F: return "links";
	case 0x2E: return "rechts";
	case 0x30: return "ok";
	case 0x16: return "Lautstaerke plus";
	case 0x17: return "Lautstaerke minus";
	case 0x28: return "Mute";
	case 0x82: return "?";
	}
	return 0;
}

int eRCKeyDBoxOld::getCompatibleCode() const
{
	if ((code&0xFF00)==0x5C00)
	{
		switch (code&0xFF)
		{
		case 0x0C: return eRCInput::RC_STANDBY;
		case 0x20: return eRCInput::RC_HOME;
		case 0x27: return eRCInput::RC_DBOX;
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
		case 0x3B: return eRCInput::RC_BLUE;
		case 0x52: return eRCInput::RC_YELLOW;
		case 0x55: return eRCInput::RC_GREEN;
		case 0x2D: return eRCInput::RC_RED;
		case 0x54: return eRCInput::RC_UP_LEFT;
		case 0x53: return eRCInput::RC_DOWN_LEFT;
		case 0x0E: return eRCInput::RC_UP;
 		case 0x0F: return eRCInput::RC_DOWN;
		case 0x2F: return eRCInput::RC_LEFT;
 		case 0x2E: return eRCInput::RC_RIGHT;
		case 0x30: return eRCInput::RC_OK;
 		case 0x16: return eRCInput::RC_PLUS;
 		case 0x17: return eRCInput::RC_MINUS;
 		case 0x28: return eRCInput::RC_MUTE;
 		case 0x82: return eRCInput::RC_HELP;
		default:
			return -1;
		}
	}
	return -1;
}

const char *eRCKeyDBoxNew::getDescription() const
{
	switch (code)
	{
	case 0: return "0";
	case 1: return "1";
	case 2: return "2";
	case 3: return "3";
	case 4: return "4";
	case 5: return "5";
	case 6: return "6";
	case 7: return "7";
	case 8: return "8";
	case 9: return "9";
	case 10: return "rechts";
	case 11: return "links";
	case 12: return "oben";
	case 13: return "unten";
	case 14: return "ok";
	case 15: return "mute";
	case 16: return "power";
	case 17: return "gruen";
	case 18: return "gelb";
	case 19: return "rot";
	case 20: return "blau";
	case 21: return "Lautstaerke plus";
	case 22: return "Lautstaerke minus";
	case 23: return "?";
	case 24: return "d-Box";
	case 27: return "oben links";
	case 28: return "oben rechts";
	case 29: return "unten links";
	case 30: return "unten rechts";
	case 31: return "home";
	}
}

int eRCKeyDBoxNew::getCompatibleCode() const
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

const char *eRCKeyDBoxButton::getDescription() const
{
	switch (code)
	{
	case 1: return "power";
	case 2: return "down";
	case 3: return "up";
	}
}

int eRCKeyDBoxButton::getCompatibleCode() const
{
	switch (code)
	{
	case 1: return eRCInput::RC_STANDBY;
	case 2: return eRCInput::RC_RIGHT;
	case 3: return eRCInput::RC_LEFT;
	}
}

class eDBoxRCHardware
{
  eRCDBoxDriver driver;
  eRCDeviceDBoxOld deviceOld;
  eRCDeviceDBoxNew deviceNew;
  eRCDeviceDBoxButton deviceButton;
public:
  eDBoxRCHardware(): deviceOld(&driver), deviceNew(&driver), deviceButton(&driver)
  {
  }
};

eAutoInitP0<eDBoxRCHardware, 2> init_rcdbox("d-Box RC Hardware");
