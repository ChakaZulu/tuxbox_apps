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
 * $Id: enigma_setup.cpp,v 1.38 2003/05/10 17:40:18 tmbinc Exp $
 */

#include <enigma_setup.h>

#include <enigma_scan.h>
#include <setupnetwork.h>
#include <setupvideo.h>
#include <setup_audio.h>
#include <wizard_language.h>
#include <setup_osd.h>
#include <setup_lcd.h>
#include <setup_rc.h>
#include <setup_harddisk.h>
#include <setup_rfmod.h>
#include <enigma_ci.h>
#include <enigma_scan.h>
#include <setupskin.h>
#include <setupengrab.h>
#include <lib/gui/emessage.h>
#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include "upgrade.h"

#include <lib/system/info.h>

eZapSetup::eZapSetup()
	:eListBoxWindow<eListBoxEntryMenu>(_("Setup"), 8, 450, true)
{
	move(ePoint(135, 120));

	int havenetwork = 0, haveci = 0, haveharddisk = 0, havelcd = 0, haverfmod = 0;

	havenetwork = eSystemInfo::getInstance()->hasNetwork();
	haveci = eSystemInfo::getInstance()->hasCI() > 0;
	haveharddisk = eSystemInfo::getInstance()->hasHDD();
	havelcd = eSystemInfo::getInstance()->hasLCD();
	haverfmod = eSystemInfo::getInstance()->hasRFMod();
	
	list.setColumns(2);
	addActionMap(&i_shortcutActions->map);
		
	int entry=0;
	
	CONNECT((new eListBoxEntryMenu(&list, _("[back]"), eString().sprintf("(%d) %s", ++entry, _("back to main menu")) ))->selected, eZapSetup::sel_close);
	CONNECT((new eListBoxEntryMenu(&list, _("Channels..."), eString().sprintf("(%d) %s", ++entry, _("open channel setup")) ))->selected, eZapSetup::sel_channels);
	if (havenetwork)
		CONNECT((new eListBoxEntryMenu(&list, _("Network..."), eString().sprintf("(%d) %s", ++entry, _("open network setup")) ))->selected, eZapSetup::sel_network);
	CONNECT((new eListBoxEntryMenu(&list, _("OSD..."), eString().sprintf("(%d) %s", ++entry, _("open OSD setup")) ))->selected, eZapSetup::sel_osd);
	if (havelcd)
		CONNECT((new eListBoxEntryMenu(&list, _("LCD..."), eString().sprintf("(%d) %s", ++entry, _("open LCD setup")) ))->selected, eZapSetup::sel_lcd);
	CONNECT((new eListBoxEntryMenu(&list, _("Remote Control..."), eString().sprintf("(%d) %s", ++entry, _("open remote control setup")) ))->selected, eZapSetup::sel_rc);
	CONNECT((new eListBoxEntryMenu(&list, _("Video..."), eString().sprintf("(%d) %s", ++entry, _("open video setup")) ))->selected, eZapSetup::sel_video);
	CONNECT((new eListBoxEntryMenu(&list, _("Audio..."), eString().sprintf("(%d) %s", ++entry, _("open audio setup")) ))->selected, eZapSetup::sel_sound);
	CONNECT((new eListBoxEntryMenu(&list, _("Skin..."), eString().sprintf("(%d) %s", ++entry, _("open skin selector")) ))->selected, eZapSetup::sel_skin);
	CONNECT((new eListBoxEntryMenu(&list, _("Language..."), eString().sprintf("(%d) %s", ++entry, _("open language selector")) ))->selected, eZapSetup::sel_language);
	CONNECT((new eListBoxEntryMenu(&list, _("Ngrab..."), eString().sprintf("(%d) %s", ++entry, _("open ngrab config")) ))->selected, eZapSetup::sel_engrab);
	if (haveharddisk)
		CONNECT((new eListBoxEntryMenu(&list, _("Harddisk..."), eString().sprintf("(%d) %s", ++entry, _("open harddisk setup")) ))->selected, eZapSetup::sel_harddisk);
	if (haveci)
		CONNECT((new eListBoxEntryMenu(&list, _("Common Interface..."), eString().sprintf("(%d) %s", ++entry, _("show CI Menu")) ))->selected, eZapSetup::sel_ci);
	if (havenetwork)
		CONNECT((new eListBoxEntryMenu(&list, _("Upgrade..."), eString().sprintf("(%d) %s", ++entry, _("upgrade firmware")) ))->selected, eZapSetup::sel_upgrade);
	if (haverfmod)
		CONNECT((new eListBoxEntryMenu(&list, _("RF-Modulator..."), eString().sprintf("(%d) %s", ++entry, _("setup modulator")) ))->selected, eZapSetup::sel_rfmod);

	list.selchanged(list.getCurrent());
}

eZapSetup::~eZapSetup()
{
}

void eZapSetup::sel_close()
{
	close(0);
}

void eZapSetup::sel_channels()
{
	hide();
	eZapScan setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();	
}

void eZapSetup::sel_network()
{
	hide();
	eZapNetworkSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_sound()
{
	hide();
	eZapAudioSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_osd()
{
	hide();
	eZapOsdSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_lcd()
{
	hide();
	eZapLCDSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_rc()
{
	hide();
	eZapRCSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_skin()
{
	hide();
	eSkinSetup setup;
	int res;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	res=setup.exec();
	setup.hide();
	if (!res)
	{
		eMessageBox msg(_("You have to reboot to apply the new skin"), _("Skin changed"));
		msg.show();
		msg.exec();
		msg.hide();
	}
	show();
}

void eZapSetup::sel_video()
{
	hide();
	eZapVideoSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}
void eZapSetup::sel_engrab()
{
	hide();
	ENgrabSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_language()
{
/*	hide();
	eZapLanguageSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show(); */
	eWizardLanguage::run();
}

void eZapSetup::sel_harddisk()
{
	hide();
	eHarddiskSetup setup;

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

void eZapSetup::sel_ci()
{
	hide();
	enigmaCI ci;
	ci.setLCD(LCDTitle, LCDElement);
	ci.show();
	ci.exec();
	ci.hide();
	show();
}

void eZapSetup::sel_upgrade()
{
	hide();
	{
		eUpgrade up;
		up.show();
		up.exec();
		up.hide();
	}
	show();
}

void eZapSetup::sel_rfmod()
{
	hide();
	eZapRFmodSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

class eZapSetupSelectN
{
	int n;
public:
	eZapSetupSelectN(int n): n(n) { }
	bool operator()(eListBoxEntryMenu &e)
	{
		if (!n--)
		{
			e.selected();
			return 1;
		}
		return 0;
	}
};

void eZapSetup::sel_num(int n)
{
	list.forEachEntry(eZapSetupSelectN(n));
}

int eZapSetup::eventHandler(const eWidgetEvent &event)
{
	int num=-1;
	switch (event.type)
	{
	case eWidgetEvent::evtAction:
		if (event.action == &i_shortcutActions->number0)
			num=9;
		else if (event.action == &i_shortcutActions->number1)
			num=0;
		else if (event.action == &i_shortcutActions->number2)
			num=1;
		else if (event.action == &i_shortcutActions->number3)
			num=2;
		else if (event.action == &i_shortcutActions->number4)
			num=3;
		else if (event.action == &i_shortcutActions->number5)
			num=4;
		else if (event.action == &i_shortcutActions->number6)
			num=5;
		else if (event.action == &i_shortcutActions->number7)
			num=6;
		else if (event.action == &i_shortcutActions->number8)
			num=7;
		else if (event.action == &i_shortcutActions->number9)
			num=8;
		else if (event.action == &i_cursorActions->cancel)
			close(0);
		else
			break;
		if (num != -1)
			sel_num(num);
		return 1;
	default:
		break;
	}
	return eWidget::eventHandler(event);
}

