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
 * $Id: enigma_setup.cpp,v 1.14 2002/05/12 20:46:28 tmbinc Exp $
 */

#include "enigma_setup.h"
#include "enigma_scan.h"
#include "setupnetwork.h"
#include "setupvideo.h"
#include "setup_language.h"
#include "enigma_scan.h"
#include "setupskin.h"

#include <core/gui/emessage.h>
#include <core/base/i18n.h>
#include <core/dvb/edvb.h>
#include <core/gui/elistbox.h>
#include <core/gui/eskin.h>
#include <core/gui/elabel.h>
#include <core/gui/ewindow.h>

eZapSetup::eZapSetup()
	:eLBWindow(_("Setup"), 8, eSkin::getActive()->queryValue("fontsize", 20), 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListboxEntryText(&list, _("[back]")))->selected, eZapSetup::sel_close);
	CONNECT((new eListboxEntryText(&list, _("Channels...")))->selected, eZapSetup::sel_channels);
	CONNECT((new eListboxEntryText(&list, _("Network...")))->selected, eZapSetup::sel_network);
//	CONNECT((list, _("Audio...")))->selected, sel_sound);
	CONNECT((new eListboxEntryText(&list, _("Video...")))->selected, eZapSetup::sel_video);
	CONNECT((new eListboxEntryText(&list, _("Skin...")))->selected, eZapSetup::sel_skin);
	CONNECT((new eListboxEntryText(&list, _("Language...")))->selected, eZapSetup::sel_language);
}

eZapSetup::~eZapSetup()
{
}

void eZapSetup::sel_close(eListboxEntry *lbe)
{
	close(0);
}

void eZapSetup::sel_channels(eListboxEntry *lbe)
{
	hide();
	eZapScan setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();	
}

void eZapSetup::sel_network(eListboxEntry *lbe)
{
	hide();
	eZapNetworkSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_sound(eListboxEntry *lbe)
{
}

void eZapSetup::sel_skin(eListboxEntry *lbe)
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

void eZapSetup::sel_video(eListboxEntry *lbe)
{
	hide();
	eZapVideoSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_language(eListboxEntry *lbe)
{
	hide();
	eZapLanguageSetup setup;
	setup.show();
	setup.exec();
	setup.hide();
	show();
}
