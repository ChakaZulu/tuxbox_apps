/*
 * time_settings.cpp
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
 * $Id: time_settings.cpp,v 1.4 2004/06/15 10:35:51 ghostrider Exp $
 */

#include <time_settings.h>
#include <setup_timezone.h>
#include <time_correction.h>
#include <timer.h>
#include <lib/dvb/edvb.h>
#include <lib/dvb/dvbservice.h>
#include <lib/gui/emessage.h>

eTimeSettings::eTimeSettings()
	:eSetupWindow(_("Time Settings"), 5, 350)
{
	move(ePoint(130, 135));
	int entry=0;
	CONNECT((new eListBoxEntryMenu(&list, _("Timezone Configuration"), eString().sprintf("(%d) %s", ++entry, _("open timezone selector")) ))->selected, eTimeSettings::time_zone);
	CONNECT((new eListBoxEntryMenu(&list, _("Time Correction"), eString().sprintf("(%d) %s", ++entry, _("open time correction window")) ))->selected, eTimeSettings::time_correction);
}

void eTimeSettings::time_zone()
{
	hide();
	eZapTimeZoneSetup setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}

void eTimeSettings::time_correction()
{
	eDVBServiceController *sapi = eDVB::getInstance()->getServiceAPI();
	if (sapi && sapi->transponder)
  {
		tsref ref = *sapi->transponder;
		hide();
		eTimeCorrectionEditWindow w(ref);
#ifndef DISABLE_LCD
		w.setLCD(LCDTitle, LCDElement);
#endif
		w.show();
		w.exec();
		w.hide();
		show();
	}
	else
	{
		hide();
		eMessageBox mb( _("To change time correction you must tune first to any transponder"), _("time correction change error"), eMessageBox::btOK|eMessageBox::iconInfo );
		mb.show();
		mb.exec();
		mb.hide();
		show();
	}
}

