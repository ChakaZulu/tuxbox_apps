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


eZapTimeshiftSetup::eZapTimeshiftSetup()
	:eWindow(0)
{
	delay=new eNumber(this,1,1, 60, 3, 0, 0); delay->setName("delay");
	minutes=new eNumber(this,1,1, MAX_PERMANENT_TIMESHIFT_MINUTES, 3, 0, 0); minutes->setName("minutes");
	active=new eCheckbox(this);active->setName("active");
	pause=new eCheckbox(this);pause->setName("pause");
	store=new eButton(this); store->setName("store");

	int tmp = 0;
	eConfig::getInstance()->getKey("/enigma/timeshift/permanent", tmp );
	unsigned char permactive = (unsigned char) tmp;
	active->setCheck(permactive);

	int tmp2 = 0;
	eConfig::getInstance()->getKey("/enigma/timeshift/activatepausebutton", tmp2 );
	unsigned char permpause = (unsigned char) tmp2;
	pause->setCheck(permpause);

	int permdelay = 30;
	eConfig::getInstance()->getKey("/enigma/timeshift/permanentdelay", permdelay );
	delay->setNumber(permdelay);

	int permbuffersize = 30;
	eConfig::getInstance()->getKey("/enigma/timeshift/permanentminutes", permbuffersize );
	minutes->setNumber(permbuffersize);

	sbar = new eStatusBar(this); sbar->setName("statusbar");

	if (eSkin::getActive()->build(this, "SetupTimeshift"))
		eFatal("skin load of \"SetupTimeshift\" failed");

	CONNECT(store->selected, eZapTimeshiftSetup::storePressed);

}


eZapTimeshiftSetup::~eZapTimeshiftSetup()
{
}

void eZapTimeshiftSetup::storePressed()
{
	int tmp = 0;
	eConfig::getInstance()->getKey("/enigma/timeshift/permanent", tmp );
	unsigned char permactive = (unsigned char) tmp;

	eConfig::getInstance()->setKey("/enigma/timeshift/permanent", active->isChecked()?255:0 );
	eConfig::getInstance()->setKey("/enigma/timeshift/activatepausebutton", pause->isChecked()?255:0 );
	eConfig::getInstance()->setKey("/enigma/timeshift/permanentdelay", delay->getNumber() );
	eConfig::getInstance()->setKey("/enigma/timeshift/permanentminutes", minutes->getNumber() );
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
