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
 * $Id: enigma_setup.cpp,v 1.17 2002/07/02 00:19:33 Ghostrider Exp $
 */

#include "enigma_setup.h"

#include <apps/enigma/enigma_scan.h>
#include <apps/enigma/setupnetwork.h>
#include <apps/enigma/setupvideo.h>
#include <apps/enigma/setup_language.h>
#include <apps/enigma/setup_osd.h>
#include <apps/enigma/enigma_scan.h>
#include <apps/enigma/setupskin.h>
#include <core/gui/emessage.h>
#include <core/base/i18n.h>
#include <core/dvb/edvb.h>
#include <core/gui/eskin.h>
#include <core/gui/elabel.h>

eZapSetup::eZapSetup()
	:eListBoxWindow<eListBoxEntryMenu>(_("Setup"), 8, 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListBoxEntryMenu(&list, _("[back]")))->selected, eZapSetup::sel_close);
	CONNECT((new eListBoxEntryMenu(&list, _("Channels...")))->selected, eZapSetup::sel_channels);
	CONNECT((new eListBoxEntryMenu(&list, _("Network...")))->selected, eZapSetup::sel_network);
//	CONNECT((list, _("Audio...")))->selected, sel_sound);
	CONNECT((new eListBoxEntryMenu(&list, _("OSD...")))->selected, eZapSetup::sel_osd);
	CONNECT((new eListBoxEntryMenu(&list, _("Video...")))->selected, eZapSetup::sel_video);
	CONNECT((new eListBoxEntryMenu(&list, _("Skin...")))->selected, eZapSetup::sel_skin);
	CONNECT((new eListBoxEntryMenu(&list, _("Language...")))->selected, eZapSetup::sel_language);
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
	setup.show();
	setup.exec();
	setup.hide();
	show();
}
