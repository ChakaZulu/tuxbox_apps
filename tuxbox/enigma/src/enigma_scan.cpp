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
 * $Id: enigma_scan.cpp,v 1.16 2003/09/07 00:03:56 ghostrider Exp $
 */

#include <enigma_scan.h>

#include <satconfig.h>
#include <rotorconfig.h>
#include <scan.h>
#include <tpeditwindow.h>
#include <lib/base/i18n.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/frontend.h>
#include <lib/gui/ewindow.h>
#include <lib/gui/eskin.h>
#include <lib/gui/elabel.h>
#include <lib/gui/emessage.h>
#include <lib/system/info.h>

eZapScan::eZapScan()
	:eSetupWindow(_("Service Searching"), 7, 400)
{
	int entry=0;
	move(ePoint(160, 140));
	if ( eSystemInfo::getInstance()->getFEType() == eSystemInfo::feSatellite )  // only when a sat box is avail we shows a satellite config
	{
		CONNECT((new eListBoxEntryMenu(&list, _("Satellite Configuration"), eString().sprintf("(%d) %s", ++entry, _("open satellite config"))))->selected, eZapScan::sel_satconfig);
		CONNECT((new eListBoxEntryMenu(&list, _("Motor Setup"), eString().sprintf("(%d) %s", ++entry, _("goto Motor Setup"))))->selected, eZapScan::sel_rotorConfig);
		CONNECT((new eListBoxEntryMenu(&list, _("Transponder Edit"), eString().sprintf("(%d) %s", ++entry, _("for automatic transponder scan"))))->selected, eZapScan::sel_transponderEdit);
		new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	}
	CONNECT((new eListBoxEntryMenu(&list, _("Automatic Transponder Scan"), eString().sprintf("(%d) %s", ++entry, _("open automatic transponder scan"))))->selected, eZapScan::sel_autoScan);
	CONNECT((new eListBoxEntryMenu(&list, _("Manual Transponder Scan"), eString().sprintf("(%d) %s", ++entry, _("open manual transponder scan"))))->selected, eZapScan::sel_manualScan);
}

void eZapScan::sel_autoScan()
{
#ifndef DISABLE_LCD
	TransponderScan setup(LCDTitle, LCDElement);
#else
	TransponderScan setup;
#endif
	hide();
	setup.exec(TransponderScan::stateAutomatic);
	show();
}

void eZapScan::sel_manualScan()
{
#ifndef DISABLE_LCD
	TransponderScan setup(LCDTitle, LCDElement);
#else
	TransponderScan setup;
#endif
	hide();
	setup.exec(TransponderScan::stateManual);
	show();
}

void eZapScan::sel_satconfig()
{
	hide();
	eSatelliteConfigurationManager satconfig;
#ifndef DISABLE_LCD
	satconfig.setLCD(LCDTitle, LCDElement);
#endif
	satconfig.show();
	satconfig.exec();
	satconfig.hide();
	show();
}

eLNB* eZapScan::getRotorLNB(int silent)
{
	int c=0;
	std::list<eLNB>::iterator RotorLnb = eTransponderList::getInstance()->getLNBs().end();
	std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin());
	for (; it != eTransponderList::getInstance()->getLNBs().end(); it++ )
	{
		if ( it->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 )
		{
			if (!c++)
				RotorLnb=it;
		}
	}
	if ( c > 1 )  // we have more than one LNBs with DiSEqC 1.2
	{
		eMessageBox mb( _("More than one LNB have DiSEqC 1.2 enabled, please select the LNB where the motor is connected"), _("Info"), eMessageBox::iconWarning|eMessageBox::btOK );
		mb.show();
		mb.exec();
		mb.hide();
		eLNBSelector sel;
		sel.show();
		int ret = sel.exec();
		sel.hide();
		return (eLNB*) ret;
	}
	else if ( !c )
	{
		if (!silent)
		{
			eMessageBox mb( _("Found no LNB with DiSEqC 1.2 enabled,\nplease goto Satellite Config first, and enable DiSEqC 1.2"), _("Warning"), eMessageBox::iconWarning|eMessageBox::btOK );
			mb.show();
			mb.exec();
			mb.hide();
		}
		return 0;
	}
	else // only one lnb with DiSEqC 1.2 is found.. this is correct :)
		return &(*RotorLnb);
}

void eZapScan::sel_transponderEdit()
{
	hide();
	eTransponderEditWindow wnd;
#ifndef DISABLE_LCD
	wnd.setLCD(LCDTitle, LCDElement);
#endif
	wnd.show();
	wnd.exec();
	wnd.hide();
	show();
}

void eZapScan::sel_rotorConfig()
{
	hide();
	eLNB* lnb = getRotorLNB(0);
	if (lnb)
	{
		RotorConfig c(lnb);
#ifndef DISABLE_LCD
		c.setLCD( LCDTitle, LCDElement );
#endif
		c.show();
		c.exec();
		c.hide();
	}
	show();
}

eLNBSelector::eLNBSelector()
	:eListBoxWindow<eListBoxEntryText>(_("Select LNB"), 5, 300, true)
{
	move(ePoint(140, 156));
	new eListBoxEntryText(&list, _("back"), 0, 0, _("go to prev menu") );
	new eListBoxEntrySeparator( (eListBox<eListBoxEntry>*)&list, eSkin::getActive()->queryImage("listbox.separator"), 0, true );
	int cnt=0;
	for ( std::list<eLNB>::iterator it( eTransponderList::getInstance()->getLNBs().begin()); it != eTransponderList::getInstance()->getLNBs().end(); it++, cnt++ )
	{
		if ( it->getDiSEqC().DiSEqCMode == eDiSEqC::V1_2 )
			new eListBoxEntryText( &list, eString().sprintf("LNB %d", cnt), (void*)&(*it), 0, eString().sprintf(_("use LNB %d for Motor"), cnt ).c_str());
	}
	CONNECT( list.selected, eLNBSelector::selected );
}

void eLNBSelector::selected( eListBoxEntryText *e )
{
	if ( e && e->getKey() )
		close((int)e->getKey());
	else
		close(0);
}
