#include <enigma_vcr.h>

#include <lib/gui/actions.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/driver/eavswitch.h>

struct enigmaVCRActions
{
	eActionMap map;
	eAction volumeUp, volumeDown;
	enigmaVCRActions():
		map("enigmaVCR", "enigma VCR"),
		volumeUp(map, "volumeUp", "volume up", eAction::prioDialog),
		volumeDown(map, "volumeDown", "volume down", eAction::prioDialog)
	{
	}
};

eAutoInitP0<enigmaVCRActions> i_enigmaVCRActions(eAutoInitNumbers::actions, "enigma vcr actions");

enigmaVCR::enigmaVCR(eString string, eString caption)
	:eMessageBox(string,caption)
{
	addActionMap(&i_enigmaVCRActions->map);
}

int enigmaVCR::eventHandler(const eWidgetEvent &event)
{
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_enigmaVCRActions->volumeUp)
			volumeUp();
		else if (event.action == &i_enigmaVCRActions->volumeDown)
			volumeDown();
		else
			break;
		return 1;
	default:
		break;
	}
	return eMessageBox::eventHandler(event);
}

void enigmaVCR::volumeUp()
{
	eAVSwitch::getInstance()->changeVCRVolume(0, -4);
}

void enigmaVCR::volumeDown()
{
	eAVSwitch::getInstance()->changeVCRVolume(0, +4);
}
