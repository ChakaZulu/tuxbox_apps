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
 * $Id: enigma_setup.cpp,v 1.25 2002/10/15 23:31:29 Ghostrider Exp $
 */

#include <enigma_setup.h>

#include <timer.h>
#include <enigma_scan.h>
#include <setupnetwork.h>
#include <setupvideo.h>
#include <setup_language.h>
#include <setup_osd.h>
#include <setup_lcd.h>
#include <setup_rc.h>
#include <setup_harddisk.h>
#include <enigma_scan.h>
#include <setupskin.h>
#include <lib/gui/emessage.h>
#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>

eZapSetup::eZapSetup()
	:eListBoxWindow<eListBoxEntryMenu>(_("Setup"), 11, 220, true)
{
	eDebug("statusbar = %p", statusbar);
	move(ePoint(150, 116));
	CONNECT((new eListBoxEntryMenu(&list, _("[back]"), _("back to Mainmenu") ))->selected, eZapSetup::sel_close);
	CONNECT((new eListBoxEntryMenu(&list, _("Channels..."), _("open channel setup") ))->selected, eZapSetup::sel_channels);
	CONNECT((new eListBoxEntryMenu(&list, _("Network..."), _("open network setup") ))->selected, eZapSetup::sel_network);
	CONNECT((new eListBoxEntryMenu(&list, _("OSD..."), _("open osd setup") ))->selected, eZapSetup::sel_osd);
	CONNECT((new eListBoxEntryMenu(&list, _("LCD..."), _("open lcd setup") ))->selected, eZapSetup::sel_lcd);
	CONNECT((new eListBoxEntryMenu(&list, _("Remote Control..."), _("open remotecontrol setup") ))->selected, eZapSetup::sel_rc);
	CONNECT((new eListBoxEntryMenu(&list, _("Video..."), _("open video setup") ))->selected, eZapSetup::sel_video);
	CONNECT((new eListBoxEntryMenu(&list, _("Skin..."), _("open skin selector") ))->selected, eZapSetup::sel_skin);
	CONNECT((new eListBoxEntryMenu(&list, _("Language..."), _("open language selector") ))->selected, eZapSetup::sel_language);
	CONNECT((new eListBoxEntryMenu(&list, _("Timer..."), _("open timer view") ))->selected, eZapSetup::sel_timer);
	if (eDVB::getInstance()->getInfo("mID") == "05")
	{
		CONNECT((new eListBoxEntryMenu(&list, _("Harddisk..."), _("initialize harddisc") ))->selected, eZapSetup::sel_harddisk);
	}
//	CONNECT(list.selchanged, eZapSetup::onSelChanged );
}

/*void eZapSetup::onSelChanged( eListBoxEntryMenu* p)
{
	eDebug("Update Statusbar to %s", p->getHelpText().c_str() );
	eDebug("Statusbar pos is left = %i, top = %i, width = %i, height = %i", statusbar->getLabel().getPosition().x(), statusbar->getLabel().getPosition().y(), statusbar->getLabel().getSize().width(), statusbar->getLabel().getSize().height() );
	list.setHelpText( p->getHelpText() );		
}*/

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

void eZapSetup::sel_timer()
{
	hide();
	eTimerView setup;
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
		eMessageBox msg(_("Harddisk setup..."), _("sorry, no harddisks found!"));
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
