#include <config.h>
#include <lib/driver/rcinput.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include <lib/base/ebase.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/driver/input_fake.h>

void eRCDeviceInputDev::handleCode(int rccode)
{
	struct input_event *ev = (struct input_event *)rccode;

	if (ev->type!=EV_KEY)
		return;

	int km = iskeyboard ? input->getKeyboardMode() : eRCInput::kmNone;
	
	if (km == eRCInput::kmAll)
		return;
	
	if (km == eRCInput::kmAscii)
	{
		eDebug("filtering..");
		switch (ev->code)
		{
		case KEY_0: case KEY_1: case KEY_2: case KEY_3: case KEY_4: case KEY_5: case KEY_6: case KEY_7: case KEY_8: case KEY_9:
		case KEY_A: case KEY_B: case KEY_C: case KEY_D: case KEY_E: case KEY_F: case KEY_G: case KEY_H: case KEY_I: case KEY_J:
		case KEY_K: case KEY_L: case KEY_M: case KEY_N: case KEY_O: case KEY_P: case KEY_Q: case KEY_R: case KEY_S: case KEY_T:
		case KEY_U: case KEY_V: case KEY_W: case KEY_X: case KEY_Y: case KEY_Z:
		case KEY_SPACE:
			/* FIXME: some are still missing */
			return;
		default:
			break;
		}
		eDebug("passed!");
	}

	switch (ev->value)
	{
	case 0:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, eRCKey::flagBreak));
		break;
	case 1:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, 0));
		memcpy(&cur, ev, sizeof(struct input_event) );
		break;
	case 2:
		/*emit*/ input->keyPressed(eRCKey(this, ev->code, eRCKey::flagRepeat));
		break;
	}
}

eRCDeviceInputDev::eRCDeviceInputDev(eRCInputEventDriver *driver)
: eRCDevice(driver->getDeviceName(), driver)
{
	eString tmp=id;
	tmp.upper();
	iskeyboard = !!strstr(tmp.c_str(), "KEYBOARD");
	eDebug("Input device \"%s\" is %sa keyboard.", id.c_str(), iskeyboard ? "" : "not ");
}

const char *eRCDeviceInputDev::getDescription() const
{
	return id.c_str();
}

const char *eRCDeviceInputDev::getKeyDescription(const eRCKey &key) const
{
	switch (key.code)
	{
	case KEY_0: return "0";
	case KEY_1: return "1";
	case KEY_2: return "2";
	case KEY_3: return "3";
	case KEY_4: return "4";
	case KEY_5: return "5";
	case KEY_6: return "6";
	case KEY_7: return "7";
	case KEY_8: return "8";
	case KEY_9: return "9";
	case KEY_RIGHT: return "rechts";
	case KEY_LEFT: return "links";
	case KEY_UP: return "oben";
	case KEY_DOWN: return "unten";
	case KEY_OK: return "ok";
	case KEY_MUTE: return "mute";
	case KEY_POWER: return "power";
	case KEY_GREEN: return "gruen";
	case KEY_YELLOW: return "gelb";
	case KEY_RED: return "rot";
	case KEY_BLUE: return "blau";
	case KEY_VOLUMEUP: return "Lautstaerke plus";
	case KEY_VOLUMEDOWN: return "Lautstaerke minus";
	case KEY_HELP: return "?";
	case KEY_SETUP: return "d-Box";
#if 0
	case KEY_TOPLEFT: return "oben links";
	case KEY_TOPRIGHT: return "oben rechts";
	case KEY_BOTTOMLEFT: return "unten links";
	case KEY_BOTTOMRIGHT: return "unten rechts";
#endif
	case KEY_HOME: return "home";
	default: return 0;
	}
}

int eRCDeviceInputDev::getKeyCompatibleCode(const eRCKey &key) const
{
	return key.code;
}

class eInputDeviceInit
{
	eRCInputEventDriver driver;
	eRCDeviceInputDev deviceInputDev;
public:
	eInputDeviceInit(): driver("/dev/input/event0"), deviceInputDev(&driver)
	{
	}
};

eAutoInitP0<eInputDeviceInit> init_rcinputdev(eAutoInitNumbers::rc+1, "input device driver");
