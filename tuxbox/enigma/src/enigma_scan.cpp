/*
 * enigma_scan.cpp
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
 * $Id: enigma_scan.cpp,v 1.8 2002/08/20 15:13:13 Ghostrider Exp $
 */

#include "enigma_scan.h"

#include <apps/enigma/satconfig.h>
#include <apps/enigma/scan.h>
#include <core/dvb/edvb.h>
#include <core/dvb/frontend.h>
#include <core/gui/ewindow.h>
#include <core/gui/eskin.h>
#include <core/gui/elabel.h>
#include <core/base/i18n.h>

eZapScan::eZapScan()
	:eListBoxWindow<eListBoxEntryMenu>(_("Channels"), 8, 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListBoxEntryMenu(&list, _("[back]")))->selected, eZapScan::sel_close);
	CONNECT((new eListBoxEntryMenu(&list, _("Transponder scan")))->selected, eZapScan::sel_scan);	
	if ( eFrontend::getInstance()->Type() == eFrontend::feSatellite )  // only when a sat box is avail we shows a satellite config
		CONNECT((new eListBoxEntryMenu(&list, _("Satellites...")))->selected, eZapScan::sel_satconfig);	
	CONNECT((new eListBoxEntryMenu(&list, _("Bouquets...")))->selected, eZapScan::sel_bouquet);	
}

eZapScan::~eZapScan()
{
}

void eZapScan::sel_close()
{
	close(0);
}

void eZapScan::sel_scan()
{
	TransponderScan setup(LCDTitle, LCDElement);
	hide();
	setup.exec();
	show();
}

void eZapScan::sel_bouquet()
{
	eDVB::getInstance()->settings->sortInChannels();
}

void eZapScan::sel_satconfig()
{
	hide();
	eSatelliteConfigurationManager satconfig;
	satconfig.setLCD(LCDTitle, LCDElement);
	satconfig.show();
	satconfig.exec();
	satconfig.hide();
	show();
}
