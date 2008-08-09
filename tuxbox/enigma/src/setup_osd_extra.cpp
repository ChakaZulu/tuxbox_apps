#include <enigma.h>
#include <setup_osd_extra.h>
#include <lib/dvb/decoder.h>
#include <lib/gui/emessage.h>
#include <lib/system/info.h>

eOSDExpertSetup::eOSDExpertSetup()
	:eSetupWindow(_("OSD Settings"), 10, 400)
{
	init_eOSDExpertSetup();
}

void eOSDExpertSetup::init_eOSDExpertSetup()
{
	cmove(ePoint(170, 115));

	int showosd=1;
	if ( eConfig::getInstance()->getKey("/ezap/osd/showOSDOnSwitchService", showosd) )
		eConfig::getInstance()->setKey("/ezap/osd/showOSDOnSwitchService", showosd);

	list.setFlags(list.getFlags()|eListBoxBase::flagNoPageMovement);

	timeout_infobar = new eListBoxEntryMulti(&list, _("infobar timeout (left, right)"));
	timeout_infobar->add((eString)"  " + eString().sprintf(_("Infobar timeout %d sec"), 2) + (eString)" >", 2);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 3) + (eString)" >", 3);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 4) + (eString)" >", 4);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 5) + (eString)" >", 5);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 6) + (eString)" >", 6);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 7) + (eString)" >", 7);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 8) + (eString)" >", 8);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 9) + (eString)" >", 9);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 10) + (eString)" >", 10);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 11) + (eString)" >", 11);
	timeout_infobar->add((eString)"< " + eString().sprintf(_("Infobar timeout %d sec"), 12) + (eString)"  ", 12);
	int timeoutInfobar = 6;
	eConfig::getInstance()->getKey("/enigma/timeoutInfobar", timeoutInfobar);
	timeout_infobar->setCurrent(timeoutInfobar);
	CONNECT( list.selchanged, eOSDExpertSetup::selInfobarChanged );

	new eListBoxEntryCheck(&list, _("show infobar on service switch"), "/ezap/osd/showOSDOnSwitchService", _("show infobar when switching to another service"));
	CONNECT((new eListBoxEntryCheck(&list,_("Serviceselector help buttons"),"/ezap/serviceselector/showButtons",_("show colored help buttons in service selector")))->selected, eOSDExpertSetup::colorbuttonsChanged );
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite)
		new eListBoxEntryCheck(&list, _("Show Sat position"), "/extras/showSatPos", _("show sat position in the infobar"));
	new eListBoxEntryCheck(&list, _("Skip confirmations"), "/elitedvb/extra/profimode", _("enable/disable confirmations"));
	new eListBoxEntryCheck(&list, _("Hide error windows"), "/elitedvb/extra/hideerror", _("don't show zap error messages"));
	new eListBoxEntryCheck(&list, _("Auto show Infobar"), "/ezap/osd/showOSDOnEITUpdate", _("always show infobar when new event info is avail"));
	new eListBoxEntryCheck(&list, _("Show remaining Time"), "/ezap/osd/showCurrentRemaining", _("show event remaining time in the infobar"));
	new eListBoxEntryCheck(&list, _("Hide shortcut icons"), "/ezap/osd/hideshortcuts", _("hide shortcut icons in menus"));
}

void eOSDExpertSetup::selInfobarChanged(eListBoxEntryMenu* e)
{
	if ( e == (eListBoxEntryMenu*)timeout_infobar )
		eConfig::getInstance()->setKey("/enigma/timeoutInfobar", (int)e->getKey());
}

void eOSDExpertSetup::colorbuttonsChanged(bool b)
{
	eServiceSelector *sel = eZap::getInstance()->getServiceSelector();
	sel->setStyle( sel->getStyle(), true );
}
