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
 * $Id: enigma_setup.cpp,v 1.11 2002/05/03 23:59:31 waldi Exp $
 */

#include "enigma_setup.h"
#include "setupnetwork.h"
#include "setupvideo.h"
#include "setup_language.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "eskin.h"
#include "elabel.h"
#include "satconfig.h"

#include <core/base/i18n.h>

eZapSetup::eZapSetup()
	:eLBWindow(_("Setup"), eListbox::tBorder, 8, eSkin::getActive()->queryValue("fontsize", 20), 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListboxEntryText(list, _("[back]")))->selected, eZapSetup::sel_close);
	CONNECT((new eListboxEntryText(list, _("Bouquets...")))->selected, eZapSetup::sel_bouquet);
	CONNECT((new eListboxEntryText(list, _("Network...")))->selected, eZapSetup::sel_network);
//	CONNECT((list, _("Audio...")))->selected, sel_sound);
	CONNECT((new eListboxEntryText(list, _("Video...")))->selected, eZapSetup::sel_video);
	CONNECT((new eListboxEntryText(list, _("Satellites...")))->selected, eZapSetup::sel_satconfig);
	CONNECT((new eListboxEntryText(list, _("Language...")))->selected, eZapSetup::sel_language);
}

eZapSetup::~eZapSetup()
{
}

void eZapSetup::sel_close(eListboxEntry *lbe)
{
	close(0);
}

void eZapSetup::sel_bouquet(eListboxEntry *lbe)
{
	eDVB::getInstance()->sortInChannels();
}

void eZapSetup::sel_network(eListboxEntry *lbe)
{
	eZapNetworkSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	hide();
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eZapSetup::sel_sound(eListboxEntry *lbe)
{
}

void eZapSetup::sel_video(eListboxEntry *lbe)
{
	eZapVideoSetup setup;
	setup.setLCD(LCDTitle, LCDElement);
	hide();
	setup.show();
	setup.exec();
	setup.hide();
	show();
}


void eZapSetup::sel_satconfig(eListboxEntry *lbe)
{
	eSatelliteConfigurationManager satconfig;
	hide();
	satconfig.show();
	satconfig.exec();
	satconfig.hide();
	show();
}

void eZapSetup::sel_language(eListboxEntry *lbe)
{
	eZapLanguageSetup setup;
	hide();
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

