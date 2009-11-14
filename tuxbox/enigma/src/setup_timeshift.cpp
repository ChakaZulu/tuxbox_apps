#ifndef DISABLE_HDD
#ifndef DISABLE_FILE
#include <setup_timeshift.h>

#include <config.h>
#include <lib/gui/slider.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/elabel.h>
#include <lib/gui/combobox.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/gui/actions.h>
#include <lib/system/econfig.h>
#include <lib/dvb/servicedvb.h>
#include <enigma_main.h>
#include <sselect.h>


eZapTimeshiftSetup::eZapTimeshiftSetup()
	:eWindow(0)
{
	init_eZapTimeshiftSetup();
}
void eZapTimeshiftSetup::init_eZapTimeshiftSetup()
{
	int permdelay = 30;
	eConfig::getInstance()->getKey("/enigma/timeshift/permanentdelay", permdelay );
	delay=CreateSkinnedNumber("delay",permdelay,1,1, 60, 3, 0);

	int permbuffersize = 30;
	eConfig::getInstance()->getKey("/enigma/timeshift/permanentminutes", permbuffersize );
	minutes=CreateSkinnedNumber("minutes",permbuffersize,1,1, MAX_PERMANENT_TIMESHIFT_MINUTES, 3, 0);

	active=CreateSkinnedCheckbox("active",0,"/enigma/timeshift/permanent");

	pause=CreateSkinnedCheckbox("pause",0,"/enigma/timeshift/activatepausebutton");

	path=CreateSkinnedTextInputField("path",MOVIEDIR,"/enigma/timeshift/storagedir");
	seldir=CreateSkinnedButton("seldir");
	CONNECT(CreateSkinnedButton("store")->selected, eZapTimeshiftSetup::storePressed);

	BuildSkin("SetupTimeshift");
	CONNECT(seldir->selected, eZapTimeshiftSetup::selectDir);

}
void eZapTimeshiftSetup::selectDir()
{
	eFileSelector sel(path->getText());
#ifndef DISABLE_LCD
	sel.setLCD(LCDTitle, LCDElement);
#endif
	hide();

	const eServiceReference *ref = sel.choose(-1);

	if (ref)
		path->setText(sel.getPath().current().path);
	show();
	setFocus(seldir);

}

eZapTimeshiftSetup::~eZapTimeshiftSetup()
{
}

void eZapTimeshiftSetup::storePressed()
{
	int tmp = 0;
	eConfig::getInstance()->getKey("/enigma/timeshift/permanent", tmp );
	unsigned char permactive = (unsigned char) tmp;
	eString startPath =path->getText();
	if (startPath.empty() || startPath[startPath.length() -1] != '/')
		startPath+= "/";

	eConfig::getInstance()->setKey("/enigma/timeshift/permanent", active->isChecked()?255:0 );
	eConfig::getInstance()->setKey("/enigma/timeshift/activatepausebutton", pause->isChecked()?255:0 );
	eConfig::getInstance()->setKey("/enigma/timeshift/permanentdelay", delay->getNumber() );
	eConfig::getInstance()->setKey("/enigma/timeshift/permanentminutes", minutes->getNumber() );
	eConfig::getInstance()->setKey("/enigma/timeshift/storagedir", startPath.c_str() );
	if (permactive && !active->isChecked())
	{
		eZapMain::getInstance()->stopPermanentTimeshift();
	}
	else if (!permactive && active->isChecked())
	{
		eZapMain::getInstance()->beginPermanentTimeshift();
	}
	close(0);
}

#endif // DISABLE_FILE
#endif // DISABLE_HDD
