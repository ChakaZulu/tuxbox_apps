#include "setup_rc.h"

#include <core/gui/slider.h>
#include <core/gui/ebutton.h>
#include <core/gui/elabel.h>
#include <core/gui/enumber.h>
#include <core/gui/eskin.h>
#include <core/system/econfig.h>
#include <core/base/i18n.h>
#include <core/dvb/dvbwidgets.h>
#include <core/driver/rc.h>

void eZapRCSetup::repeatChanged( int i )
{
	eDebug("Repeat rate changed to %i", i);
	rrate = 250-i;
	update();
}

void eZapRCSetup::delayChanged( int i )
{
	eDebug("Repeat delay changed to %i", i);
	rdelay = i;
	update();
}

void eZapRCSetup::update()
{
	eRCInput::getInstance()->config.set(rdelay, rrate);
}

eZapRCSetup::eZapRCSetup(): eWindow(0)
{
	setText(_("Remotecontrol Setup"));
	move(ePoint(150, 136));
	resize(eSize(450, 280));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
	rrate = 250 - rrate;

	eDebug("rrate = %i, rdelay = %i", rrate, rdelay);

	lrrate=new eLabel(this);
	lrrate->setText(_("Repeat Rate:"));
	lrrate->move(ePoint(20, 20));
	lrrate->resize(eSize(160, fd+4));

	lrdelay=new eLabel(this);
	lrdelay->setText(_("Repeat Delay:"));
	lrdelay->move(ePoint(20, 60));
	lrdelay->resize(eSize(160, fd+4));

	srrate=new eSlider(this, lrrate, 0, 250 );
	srrate->setName("rrate");
	srrate->move(ePoint(180, 20));
	srrate->resize(eSize(220, fd+4));
	srrate->setHelpText(_("set RC repeat rate ( left / right )"));
	CONNECT( srrate->changed, eZapRCSetup::repeatChanged );

	srdelay=new eSlider(this, lrdelay, 0, 1000 );
	srdelay->setName("contrast");
	srdelay->move(ePoint(180, 60));
	srdelay->resize(eSize(220, fd+4));
	srdelay->setHelpText(_("set RC repeat delay ( left / right )"));
	CONNECT( srdelay->changed, eZapRCSetup::delayChanged );

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->move(ePoint(20, 135));
	ok->resize(eSize(90, fd+4));
	ok->setHelpText(_("close window and save changes"));
	ok->loadDeco();
	CONNECT(ok->selected, eZapRCSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(140, 135));
	abort->resize(eSize(100, fd+4));
	abort->setHelpText(_("close window (no changes are saved)"));
	abort->loadDeco();
	CONNECT(abort->selected, eZapRCSetup::abortPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-30 ) );
	statusbar->resize( eSize( clientrect.width(), 30) );
	statusbar->loadDeco();

	srdelay->setValue(rdelay);
	srrate->setValue(rrate);
}

eZapRCSetup::~eZapRCSetup()
{
}

void eZapRCSetup::okPressed()
{
	eConfig::getInstance()->setKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->setKey("/ezap/rc/repeatDelay", rdelay);
	eConfig::getInstance()->flush();
	close(1);
}

void eZapRCSetup::abortPressed()
{
	eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
	update();
	close(0);
}
