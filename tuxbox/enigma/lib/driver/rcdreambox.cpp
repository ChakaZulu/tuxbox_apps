#include <dbox/fp.h>
#include "rcdreambox.h"
#include "init.h"

	/* ----------------------- dreambox fernbedienung ---------------------- */
void eRCDeviceDreambox::handleCode(int rccode)
{
	if ((rccode&0xF700)!=0x1400)
		return;
	timeout.start(300, 1);
	int old=ccode;
	ccode=rccode;
	if ((old!=-1) && (old!=rccode))
		/*emit*/ input->keyPressed(eRCKey(this, old&0xF7FF, eRCKey::flagBreak));
	if (old != rccode)
	{
		repeattimer.start(rdelay, 1);
		input->keyPressed(eRCKey(this, rccode&0xF7FF, 0));
	}
}

void eRCDeviceDreambox::timeOut()
{
	int oldcc=ccode;
	ccode=-1;
	repeattimer.stop();
	if (oldcc!=-1)
		input->keyPressed(eRCKey(this, oldcc&0xF7FF, eRCKey::flagBreak));
}

void eRCDeviceDreambox::repeat()
{
	if (ccode!=-1)
		input->keyPressed(eRCKey(this, ccode&0xF7FF, eRCKey::flagRepeat));
	repeattimer.start(rrate, 1);
}

eRCDeviceDreambox::eRCDeviceDreambox(eRCDriver *driver): eRCDevice("Dreambox", driver)
{
	ccode=-1;
	rrate=30;
	rdelay=500;
	CONNECT(timeout.timeout, eRCDeviceDreambox::timeOut);
	CONNECT(repeattimer.timeout, eRCDeviceDreambox::repeat);
}

const char *eRCDeviceDreambox::getDescription() const
{
	return "dreambox Fernbedienung";
}

const char *eRCDeviceDreambox::getKeyDescription(const eRCKey &key) const
{
	if ((key.code&0xFF00)!=0x1400)
		return 0;
	switch (key.code&0xFF)
	{
	case 0: return "dream-box";
	case 1: return "power";
	case 2: return "tv";
	case 3: return "radio";
	case 4: return "text";
	case 5: return "help";
	case 6: return "ch up";
	case 7: return "ch down";
	case 8: return "LAME!";
	case 9: return "mute";
	case 0xA: return "vol up";
	case 0xB: return "vol down";
	case 0xC: return "guide";
	case 0xD: return "info";
	case 0xE: return "audio";
	case 0xF: return "video";
	case 0x10: return "up";
	case 0x11: return "down";
	case 0x12: return "left";
	case 0x13: return "right";
	case 0x14: return "select";
	case 0x15: return "red";
	case 0x16: return "green";
	case 0x17: return "yellow";
	case 0x18: return "blue";
	case 0x19: return "1";
	case 0x1a: return "2";
	case 0x1b: return "3";
	case 0x1c: return "4";
	case 0x1d: return "5";
	case 0x1e: return "6";
	case 0x1f: return "7";
	case 0x20: return "8";
	case 0x21: return "9";
	case 0x22: return "0";
	case 0x23: return "back";
	case 0x24: return "forward";
	case 0x25: return "A";
	case 0x26: return "B";
	case 0x27: return "C";
	case 0x28: return "D";
	case 0x29: return "E";
	case 0x2A: return "F";
	}
	return 0;
}

int eRCDeviceDreambox::getKeyCompatibleCode(const eRCKey &key) const
{
	if ((key.code&0xFF00)!=0x1400)
		return 0;
	switch (key.code&0xFF)
	{
	case 0: return eRCInput::RC_DBOX;
	case 1: return eRCInput::RC_STANDBY;
	case 2: return eRCInput::RC_HOME;
	case 5: return eRCInput::RC_HELP;
	case 6: return eRCInput::RC_RIGHT;
	case 7: return eRCInput::RC_LEFT;
	case 8: return eRCInput::RC_HELP;
	case 9: return eRCInput::RC_MUTE;
	case 0xA: return eRCInput::RC_PLUS;
	case 0xB: return eRCInput::RC_MINUS;
	case 0xC: return eRCInput::RC_HELP;
	case 0xD: return eRCInput::RC_DBOX;
	case 0xE: return eRCInput::RC_YELLOW;
	case 0xF: return eRCInput::RC_GREEN;
	case 0x10: return eRCInput::RC_UP;
	case 0x11: return eRCInput::RC_DOWN;
	case 0x12: return eRCInput::RC_LEFT;
	case 0x13: return eRCInput::RC_RIGHT;
	case 0x14: return eRCInput::RC_OK;
	case 0x15: return eRCInput::RC_RED;
	case 0x16: return eRCInput::RC_GREEN;
	case 0x17: return eRCInput::RC_YELLOW;
	case 0x18: return eRCInput::RC_BLUE;
	case 0x19: return eRCInput::RC_1;
	case 0x1a: return eRCInput::RC_2;
	case 0x1b: return eRCInput::RC_3;
	case 0x1c: return eRCInput::RC_4;
	case 0x1d: return eRCInput::RC_5;
	case 0x1e: return eRCInput::RC_6;
	case 0x1f: return eRCInput::RC_7;
	case 0x20: return eRCInput::RC_8;
	case 0x21: return eRCInput::RC_9;
	case 0x22: return eRCInput::RC_0;
	}
	return -1;
}

eRCDreamboxDriver::eRCDreamboxDriver(): eRCShortDriver("/dev/rawir")
{
}

class eDreamboxRCHardware
{
	eRCInput input;
	eRCDreamboxDriver driver;
	eRCDeviceDreambox device;
public:
	eDreamboxRCHardware(): device(&driver)
	{
	}
};

eAutoInitP0<eDreamboxRCHardware> init_rcdreambox(2, "DreamBox RC Hardware");
