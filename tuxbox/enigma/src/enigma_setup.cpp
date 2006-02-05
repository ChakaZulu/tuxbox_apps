/*
 * enigma_setup.cpp
 *
 * Copyright (C) 2002 Felix Domke <tmbinc@tuxbox.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: enigma_setup.cpp,v 1.42 2006/02/05 23:41:01 pieterg Exp $
 */

#include <enigma_setup.h>
#include <lib/dvb/edvb.h>
#include <lib/gui/emessage.h>
#include <lib/system/econfig.h>
#include <lib/system/info.h>
#include <system_settings.h>
#include <enigma_bouquet.h>
#include <enigma_ci.h>
#include <enigma_scan.h>
#include <setup_extra.h>
#include <parentallock.h>

eZapSetup::eZapSetup()
	:eSetupWindow(_("Setup"), 10, 400)
{
	move(ePoint(160, 100));
	int entry=0;
	CONNECT((new eListBoxEntryMenu(&list, _("Service Organising"), eString().sprintf("(%d) %s", ++entry, _("open service organising")) ))->selected, eZapSetup::service_organising);
	CONNECT((new eListBoxEntryMenu(&list, _("Service Searching"), eString().sprintf("(%d) %s", ++entry, _("open service searching")) ))->selected, eZapSetup::service_searching);
	new eListBoxEntryMenuSeparator(&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	CONNECT((new eListBoxEntryMenu(&list, _("System Settings"), eString().sprintf("(%d) %s", ++entry, _("open system settings")) ))->selected, eZapSetup::system_settings);
#ifndef DISABLE_CI
	if ( eSystemInfo::getInstance()->hasCI() )
		CONNECT((new eListBoxEntryMenu(&list, _("Common Interface"), eString().sprintf("(%d) %s", ++entry, _("open common interface menu")) ))->selected, eZapSetup::common_interface);
#endif
	CONNECT((new eListBoxEntryMenu(&list, _("Parental Lock"), eString().sprintf("(%d) %s", ++entry, _("open parental setup")) ))->selected, eZapSetup::parental_lock );
	new eListBoxEntryMenuSeparator(&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	CONNECT((new eListBoxEntryMenu(&list, _("Expert Setup"), eString().sprintf("(%d) %s", ++entry, _("open expert setup")) ))->selected, eZapSetup::expert_setup);
	/* emit */ setupHook(this, &entry);
}

void eZapSetup::system_settings()
{
	hide();
	eSystemSettings setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::service_organising()
{
	hide();
	eZapBouquetSetup s;
#ifndef DISABLE_LCD
	s.setLCD(LCDTitle, LCDElement);
#endif
	s.show();
	s.exec();
	s.hide();
	show();
}

void eZapSetup::service_searching()
{
	hide();
	eZapScan setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

#ifndef DISABLE_CI
void eZapSetup::common_interface()
{
	hide();
	enigmaCI ci;
#ifndef DISABLE_LCD
	ci.setLCD(LCDTitle, LCDElement);
#endif
	ci.show();
	ci.exec();
	ci.hide();
	show();
}
#endif

void eZapSetup::expert_setup()
{
	hide();
	eExpertSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	eConfig::getInstance()->flush();
	show();
}

void eZapSetup::parental_lock()
{
	hide();
	eParentalSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

