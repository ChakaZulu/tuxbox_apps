#include <setup_rc.h>

#include <lib/gui/slider.h>
#include <lib/gui/ebutton.h>
#include <lib/gui/elabel.h>
#include <lib/gui/combobox.h>
#include <lib/gui/enumber.h>
#include <lib/gui/echeckbox.h>
#include <lib/gui/eskin.h>
#include <lib/gui/actions.h>
#include <lib/system/econfig.h>
#include <lib/base/i18n.h>
#include <lib/dvb/dvbwidgets.h>
#include <lib/driver/rc.h>
#include <enigma.h>
#include <enigma_main.h>

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

eZapRCSetup::eZapRCSetup()
	:eWindow(0)
{
	init_eZapRCSetup();
}

void eZapRCSetup::init_eZapRCSetup()
{
	setText(_("Remotecontrol Setup"));
	cmove(ePoint(140, 136));
	cresize(eSize(470, 335));

	int fd=eSkin::getActive()->queryValue("fontsize", 20);

	eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
	rrate = 250 - rrate;

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
	srrate->setHelpText(_("change remote control repeat rate\nleft => less, right => more (... repeats)"));
	CONNECT( srrate->changed, eZapRCSetup::repeatChanged );
	
	srdelay=new eSlider(this, lrdelay, 0, 1000 );
	srdelay->setName("rdelay");
	srdelay->move(ePoint(200, 60));
	srdelay->resize(eSize(220, fd+4));
	srdelay->setHelpText(_("change remote control repeat delay\nleft => shorter, right => longer (...delay)"));
	CONNECT( srdelay->changed, eZapRCSetup::delayChanged );

	lrcStyle=new eLabel(this);
	lrcStyle->move(ePoint(20, 100));
	lrcStyle->resize(eSize(220, fd+4));
	lrcStyle->setText("Remotecontrol Style:");
	rcStyle=new eComboBox(this, 4, lrcStyle);
	rcStyle->move(ePoint(20, 140));
	rcStyle->resize(eSize(220, 35));
	rcStyle->setHelpText(_("select your favourite RC style (ok)"));
	rcStyle->loadDeco();
	CONNECT( rcStyle->selchanged, eZapRCSetup::styleChanged );
	eListBoxEntryText *current=0;
	const std::set<eString> &activeStyles=eActionMapList::getInstance()->getCurrentStyles();
	for (std::map<eString, eString>::const_iterator it(eActionMapList::getInstance()->getExistingStyles().begin())
		; it != eActionMapList::getInstance()->getExistingStyles().end(); ++it)
	{
		if (activeStyles.find(it->first) != activeStyles.end())
		{
			current = new eListBoxEntryText( *rcStyle, it->second, (void*) &it->first );
			curstyle = it->first;
		}
		else
			new eListBoxEntryText( *rcStyle, it->second, (void*) &it->first );
	}
	if (current)
		rcStyle->setCurrent( current );

	lNextCharTimeout = new eLabel(this);
	lNextCharTimeout->move(ePoint(20,185));
	lNextCharTimeout->resize(eSize(300,35));
	lNextCharTimeout->setText(_("Next Char Timeout:"));

	unsigned int t;
	if (eConfig::getInstance()->getKey("/ezap/rc/TextInputField/nextCharTimeout", t) )
		t=0;
	NextCharTimeout = new eNumber(this,1,0,3999,4,0,0,lNextCharTimeout);
	NextCharTimeout->move(ePoint(335,180));
	NextCharTimeout->resize(eSize(65,35));
	NextCharTimeout->loadDeco();
	NextCharTimeout->setHelpText(_("cursor to next char timeout(msek) in textinputfields"));
	NextCharTimeout->setNumber(t);
	CONNECT(NextCharTimeout->selected, eZapRCSetup::nextField);

	ok=new eButton(this);
	ok->setText(_("save"));
	ok->setShortcut("green");
	ok->setShortcutPixmap("green");
	ok->move(ePoint(20, clientrect.height()-100));
	ok->resize(eSize(220, 40));
	ok->setHelpText(_("save changes and return"));
	ok->loadDeco();
	CONNECT(ok->selected, eZapRCSetup::okPressed);

	statusbar=new eStatusBar(this);
	statusbar->move( ePoint(0, clientrect.height()-50 ) );
	statusbar->resize( eSize( clientrect.width(), 50) );
	statusbar->loadDeco();

	srdelay->setValue(rdelay);
	srrate->setValue(rrate);
	setHelpID(85);
}

eZapRCSetup::~eZapRCSetup()
{
}

void eZapRCSetup::nextField(int *)
{
	focusNext(eWidget::focusDirNext);
}

void eZapRCSetup::styleChanged( eListBoxEntryText* e)
{
	if (e)
	{
		eActionMapList::getInstance()->deactivateStyle( curstyle );
		eActionMapList::getInstance()->activateStyle( curstyle = *(eString*)e->getKey() );
	}
}

void eZapRCSetup::okPressed()
{
	// save current selected style
	eConfig::getInstance()->setKey("/ezap/rc/style", curstyle.c_str());

	eZap::getInstance()->getServiceSelector()->setKeyDescriptions();
	setStyle();

	rrate = 250 - srrate->getValue();
	eConfig::getInstance()->setKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->setKey("/ezap/rc/repeatDelay", rdelay);
	unsigned int t = (unsigned int) NextCharTimeout->getNumber();
	eConfig::getInstance()->setKey("/ezap/rc/TextInputField/nextCharTimeout", t );
	eConfig::getInstance()->flush();
	close(1);
}

int eZapRCSetup::eventHandler( const eWidgetEvent & e )
{
	switch(e.type)
	{
		case eWidgetEvent::execDone:
			setStyle();
			eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
			eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
			update();
			break;
		default:
			return eWindow::eventHandler( e );
	}
	return 1;
}

void eZapRCSetup::setStyle()
{
	eActionMapList::getInstance()->deactivateStyle(curstyle);

	char *style=0;
	if (eConfig::getInstance()->getKey("/ezap/rc/style", style) )
		eActionMapList::getInstance()->activateStyle("default");
	else
	{
		eActionMapList::getInstance()->activateStyle( style );
		free(style);
	}
}
