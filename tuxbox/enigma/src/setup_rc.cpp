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
	eConfig::getInstance()->getKey("/ezap/rc/repeatRate", rrate);
	eConfig::getInstance()->getKey("/ezap/rc/repeatDelay", rdelay);
	rrate = 250 - rrate;

	srrate=CreateSkinnedSlider("srrate", "lrrate", 0, 250 );
	CONNECT( srrate->changed, eZapRCSetup::repeatChanged );
	
	srdelay=CreateSkinnedSlider("srdelay", "lrdelay", 0, 1000 );
	CONNECT( srdelay->changed, eZapRCSetup::delayChanged );

	rcStyle=CreateSkinnedComboBoxWithLabel("rcStyle", 4, "lrcStyle");
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
	unsigned int t;
	if (eConfig::getInstance()->getKey("/ezap/rc/TextInputField/nextCharTimeout", t) )
		t=0;
	NextCharTimeout = CreateSkinnedNumberWithLabel("NextCharTimeout",t,1,0,3999,4,0,0, "lNextCharTimeout");
	CONNECT(NextCharTimeout->selected, eZapRCSetup::nextField);

	CONNECT(CreateSkinnedButton("ok")->selected, eZapRCSetup::okPressed);

	BuildSkin("eZapRCSetup");
	if (current)
		rcStyle->setCurrent( current );

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
