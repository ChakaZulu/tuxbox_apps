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
 * $Id: enigma_scan.cpp,v 1.1 2002/05/06 16:17:16 tmbinc Exp $
 */

#include "enigma_scan.h"
#include "elistbox.h"
#include "ewindow.h"
#include "edvb.h"
#include "eskin.h"
#include "elabel.h"
#include "scan.h"
#include "satconfig.h"
#include "scan.h" 


#include <core/base/i18n.h>

eZapScan::eZapScan()
	:eLBWindow(_("Channels"), eListbox::tBorder, 8, eSkin::getActive()->queryValue("fontsize", 20), 220)
{
	move(ePoint(150, 136));
	CONNECT((new eListboxEntryText(list, _("[back]")))->selected, eZapScan::sel_close);
	CONNECT((new eListboxEntryText(list, _("Transponder scan")))->selected, eZapScan::sel_scan);	
	CONNECT((new eListboxEntryText(list, _("Satellites...")))->selected, eZapScan::sel_satconfig);	
	CONNECT((new eListboxEntryText(list, _("Bouquets...")))->selected, eZapScan::sel_bouquet);	
}

eZapScan::~eZapScan()
{
}

void eZapScan::sel_close(eListboxEntry *)
{
	close(0);
}

void eZapScan::sel_scan(eListboxEntry *)
{
	TransponderScan setup;
	hide();
	setup.exec();
	show();
}

void eZapScan::sel_bouquet(eListboxEntry *)
{
	eDVB::getInstance()->sortInChannels();
}

void eZapScan::sel_satconfig(eListboxEntry *)
{
	eSatelliteConfigurationManager satconfig;
	hide();
	satconfig.show();
	satconfig.exec();
	satconfig.hide();
	show();
}
