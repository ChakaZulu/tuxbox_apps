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
 * $Id: enigma_setup.cpp,v 1.31 2002/12/19 21:17:27 Ghostrider Exp $
 */

#include <enigma_setup.h>

#include <enigma_scan.h>
#include <setupnetwork.h>
#include <setupvideo.h>
#include <setup_language.h>
#include <setup_osd.h>
#include <setup_lcd.h>
#include <setup_rc.h>
#include <setup_harddisk.h>
#include <enigma_ci.h>
#include <enigma_scan.h>
#include <setupskin.h>
#include <lib/gui/emessage.h>
#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include "upgrade.h"

eZapSetup::eZapSetup()
	:eListBoxWindow<eListBoxEntryMenu>(_("Setup"), 12, 300, true)
{
	move(ePoint(150, 90)); 
	CONNECT((new eListBoxEntryMenu(&list, _("[back]"), _("back to Mainmenu") ))->selected, eZapSetup::sel_close);
	CONNECT((new eListBoxEntryMenu(&list, _("Channels..."), _("open channel setup") ))->selected, eZapSetup::sel_channels);
	if (eDVB::getInstance()->getInfo("mID") != "06")
	{
		CONNECT((new eListBoxEntryMenu(&list, _("Network..."), _("open network setup") ))->selected, eZapSetup::sel_network);
	}	
	CONNECT((new eListBoxEntryMenu(&list, _("OSD..."), _("open osd setup") ))->selected, eZapSetup::sel_osd);
	if (eDVB::getInstance()->getInfo("mID") != "06")
	{
		CONNECT((new eListBoxEntryMenu(&list, _("LCD..."), _("open lcd setup") ))->selected, eZapSetup::sel_lcd);
	}	
	CONNECT((new eListBoxEntryMenu(&list, _("Remote Control..."), _("open remotecontrol setup") ))->selected, eZapSetup::sel_rc);
	CONNECT((new eListBoxEntryMenu(&list, _("Video..."), _("open video setup") ))->selected, eZapSetup::sel_video);
	CONNECT((new eListBoxEntryMenu(&list, _("Skin..."), _("open skin selector") ))->selected, eZapSetup::sel_skin);
	CONNECT((new eListBoxEntryMenu(&list, _("Language..."), _("open language selector") ))->selected, eZapSetup::sel_language);
	if (eDVB::getInstance()->getInfo("mID") == "05")
	{
		CONNECT((new eListBoxEntryMenu(&list, _("Harddisk..."), _("initialize harddisc") ))->selected, eZapSetup::sel_harddisk);
		CONNECT((new eListBoxEntryMenu(&list, _("Common Interface..."), _("show CI Menu") ))->selected, eZapSetup::sel_ci);
		CONNECT((new eListBoxEntryMenu(&list, _("Upgrade..."), _("upgrade firmware") ))->selected, eZapSetup::sel_upgrade);
	}
	if (eDVB::getInstance()->getInfo("mID") == "06")
	{
		CONNECT((new eListBoxEntryMenu(&list, _("Common Interface..."), _("show CI Menu") ))->selected, eZapSetup::sel_ci);
	}
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

void eZapSetup::sel_language()
{
	hide();
	eZapLanguageSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
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
