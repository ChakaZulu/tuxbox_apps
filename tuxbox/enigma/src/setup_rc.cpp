#include <setup_rc.h>

#include <lib/gui/slider.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/elabel.h>
#include <lib/gui/combobox.h>
#include <lib/gui/enumber.h>
#include <lib/gui/eskin.h>
#include <lib/gui/actions.h>
#include <lib/system/econfig.h>
#include <lib/base/i18n.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/driver/rc.h>

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
	resize(eSize(470, 330));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
	rrate = 250 - rrate;

	eDebug("rrate = %i, rdelay = %i", rrate, rdelay);

	lrrate=new eLabel(this);
	lrrate->setText(_("Repeat Rate:"));
	lrrate->move(ePoint(20, 20));
	lrrate->resize(eSize(170, fd+4));

	lrdelay=new eLabel(this);
	lrdelay->setText(_("Repeat Delay:"));
	lrdelay->move(ePoint(20, 60));
	lrdelay->resize(eSize(170, fd+4));

	srrate=new eSlider(this, lrrate, 0, 250 );
	srrate->setName("rrate");
	srrate->move(ePoint(200, 20));
	srrate->resize(eSize(220, fd+4));
	srrate->setHelpText(_("set RC repeat rate ( left / right )"));
	CONNECT( srrate->changed, eZapRCSetup::repeatChanged );

	srdelay=new eSlider(this, lrdelay, 0, 1000 );
	srdelay->setName("contrast");
	srdelay->move(ePoint(200, 60));
	srdelay->resize(eSize(220, fd+4));
	srdelay->setHelpText(_("set RC repeat delay ( left / right )"));
	CONNECT( srdelay->changed, eZapRCSetup::delayChanged );

	lrcStyle=new eLabel(this);
	lrcStyle->move(ePoint(20, 100));
	lrcStyle->resize(eSize(220, fd+4));
	lrcStyle->setText("Remotecontrol Style:");
	rcStyle=new eComboBox(this, 4, lrcStyle);
	rcStyle->move(ePoint(20, 140));
	rcStyle->resize(eSize(220, 35));
	rcStyle->setHelpText(_("select your favourite rc style (ok)"));
	rcStyle->loadDeco();
	CONNECT( rcStyle->selchanged, eZapRCSetup::styleChanged );
	eListBoxEntryText *current=0;
	for (std::map<eString, eString>::const_iterator it(eActionMapList::getInstance()->getExistingStyles().begin()); it != eActionMapList::getInstance()->getExistingStyles().end(); ++it)
	{
		if ( it->first == eActionMapList::getInstance()->getCurrentStyle() )
			current = new eListBoxEntryText( *rcStyle, it->second, (void*) &it->first );
		else
			new eListBoxEntryText( *rcStyle, it->second, (void*) &it->first );
	}
	if (current)
		rcStyle->setCurrent( current );
	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, clientrect.height()-80));
	ok->resize(eSize(170, 40));
	ok->setHelpText(_("close window and save changes"));
	ok->loadDeco();
	CONNECT(ok->selected, eZapRCSetup::okPressed);

	abort=new eButton(this);
	abort->setText(_("abort"));
	abort->move(ePoint(210, clientrect.height()-80));
	abort->resize(eSize(170, 40));
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

void eZapRCSetup::styleChanged( eListBoxEntryText* e)
{
	if (e)
		eActionMapList::getInstance()->setCurrentStyle( *(eString*)e->getKey() );
}

void eZapRCSetup::okPressed()
{
	// save current selected style
	eConfig::getInstance()->setKey("/ezap/rc/style", ((eString*)rcStyle->getCurrent()->getKey())->c_str() );
	eConfig::getInstance()->setKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->setKey("/ezap/rc/repeatDelay", rdelay);
	eConfig::getInstance()->flush();
	close(1);
}

void eZapRCSetup::abortPressed()
{
	char *style;
	if (eConfig::getInstance()->getKey("/ezap/rc/style", style) )
		eActionMapList::getInstance()->setCurrentStyle("default");
	else
		eActionMapList::getInstance()->setCurrentStyle( style );

	eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
	update();
	close(0);
}
