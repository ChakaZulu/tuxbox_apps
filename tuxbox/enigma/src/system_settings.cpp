/*
 * system_settings.cpp
 *
 * Copyright (C) 2003 Andreas Monzner <ghostrider@tuxbox.org>
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
 * $Id: system_settings.cpp,v 1.6 2004/05/05 13:30:43 ghostrider Exp $
 */

#include <system_settings.h>
#include <setup_osd.h>
#include <wizard_language.h>
#include <time_settings.h>
#include <setup_rfmod.h>
#include <setup_lcd.h>
#include <setup_harddisk.h>
#include <setup_keyboard.h>
#include <setupvideo.h>
#include <lib/dvb/edvb.h>
#include <lib/gui/emessage.h>
#include <lib/system/info.h>

eSystemSettings::eSystemSettings()
	:eSetupWindow(_("System Settings"), 10, 350)
{
	move(ePoint(180, 100));
	int entry=0;
	CONNECT((new eListBoxEntryMenu(&list, _("Time Settings"), eString().sprintf("(%d) %s", ++entry, _("open time settings")) ))->selected, eSystemSettings::time_settings);
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	CONNECT((new eListBoxEntryMenu(&list, _("A/V Settings"), eString().sprintf("(%d) %s", ++entry, _("open A/V settings")) ))->selected, eSystemSettings::av_settings);
#ifdef ENABLE_RFMOD
	if ( eSystemInfo::getInstance()->hasRFMod() )
		CONNECT((new eListBoxEntryMenu(&list, _("UHF Modulator"), eString().sprintf("(%d) %s", ++entry, _("open UHF-Modulator setup")) ))->selected, eSystemSettings::uhf_modulator);
#endif
#ifndef DISABLE_FILE
	if ( eSystemInfo::getInstance()->hasHDD() )
		CONNECT((new eListBoxEntryMenu(&list, _("Harddisc Setup"), eString().sprintf("(%d) %s", ++entry, _("open harddisc setup")) ))->selected, eSystemSettings::harddisc_setup);
#endif
#ifdef ENABLE_KEYBOARD
	if ( eSystemInfo::getInstance()->hasKeyboard() )
		CONNECT((new eListBoxEntryMenu(&list, _("Keyboard Setup"), eString().sprintf("(%d) %s", ++entry, _("open keyboard setup")) ))->selected, eSystemSettings::keyboard_setup);
#endif
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	CONNECT((new eListBoxEntryMenu(&list, _("OSD Settings"), eString().sprintf("(%d) %s", ++entry, _("open on screen display settings")) ))->selected, eSystemSettings::osd_settings);
	CONNECT((new eListBoxEntryMenu(&list, _("OSD Language"), eString().sprintf("(%d) %s", ++entry, _("open language selector")) ))->selected, eSystemSettings::osd_language);
#ifndef DISABLE_LCD
	if ( eSystemInfo::getInstance()->hasLCD() )
		CONNECT((new eListBoxEntryMenu(&list, _("LCD Settings"), eString().sprintf("(%d) %s", ++entry, _("open LCD settings")) ))->selected, eSystemSettings::lcd_settings);
#endif
}

void eSystemSettings::osd_settings()
{
	hide();
	eZapOsdSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eSystemSettings::osd_language()
{
	eWidget *oldfocus=focus;
	hide();
	eWizardLanguage::run();
	show();
	setFocus(oldfocus);
}

void eSystemSettings::time_settings()
{
	hide();
	eTimeSettings setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eSystemSettings::av_settings()
{
	hide();
	eZapVideoSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

#ifndef DISABLE_FILE
void eSystemSettings::harddisc_setup()
{
	hide();
	eHarddiskSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	if (!setup.getNr())
	{
		eMessageBox msg(_("sorry, no harddisks found!"), _("Harddisk setup..."));
		msg.show();
		msg.exec();
		msg.hide();
	} else
	{
		setup.show();
		setup.exec();
		setup.hide();
	}
	show();
}
#endif

#ifdef ENABLE_KEYBOARD
void eSystemSettings::keyboard_setup()
{
	hide();
	eZapKeyboardSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}
#endif

#ifdef ENABLE_RFMOD
void eSystemSettings::uhf_modulator()
{
	hide();
	eZapRFmodSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}
#endif

#ifndef DISABLE_LCD
void eSystemSettings::lcd_settings()
{
	hide();
	eZapLCDSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}
#endif

