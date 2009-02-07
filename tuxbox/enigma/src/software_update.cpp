/*
 * software_update.cpp
 *
 * Copyright (C) 2003 Andreas Monzner <ghost@tuxbox.org>
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
 * $Id: software_update.cpp,v 1.7 2009/02/07 10:06:31 dbluelle Exp $
 */

#include <software_update.h>
#include <flashtool.h>
#include <lib/gui/emessage.h>
#include <upgrade.h>

eSoftwareUpdate::eSoftwareUpdate()
	:eSetupWindow(_("Software Update"), 6, 400)
{
	init_eSoftwareUpdate();
}
void eSoftwareUpdate::init_eSoftwareUpdate()
{
	move(ePoint(140, 100));
#ifndef DISABLE_NETWORK
	int entry=0;
	CONNECT((new eListBoxEntryMenu(&list, _("Internet Update"), eString().sprintf("(%d) %s", ++entry, _("open internet update")) ))->selected, eSoftwareUpdate::internet_update );
	CONNECT((new eListBoxEntryMenu(&list, _("Manual Update"), eString().sprintf("(%d) %s", ++entry, _("open manual update")) ))->selected, eSoftwareUpdate::manual_update );
#ifdef ENABLE_FLASHTOOL
	CONNECT((new eListBoxEntryMenu(&list, _("Expert Flash Save/Restore"), eString().sprintf("(%d) %s", ++entry, _("open expert flash tool")) ))->selected, eSoftwareUpdate::flash_tool);
#endif
#endif
}

#ifndef DISABLE_NETWORK
void eSoftwareUpdate::internet_update()
{
	hide();
	eUpgrade up(false);
#ifndef DISABLE_LCD
	up.setLCD(LCDTitle, LCDElement);
#endif
	up.show();
	up.exec();
	up.hide();
	show();
}

void eSoftwareUpdate::manual_update()
{
	hide();
	int ret = eMessageBox::ShowBox(_("Upload your Image via FTP or Samba to the '/tmp' folder."
										"Then rename it to 'root.cramfs' and press ok."
										"In the upcomming list select 'manual update' and follow the instructions."), _("Manual update"), eMessageBox::iconInfo|eMessageBox::btOK );
	if ( ret == eMessageBox::btOK )
	{
		eUpgrade up(true);
#ifndef DISABLE_LCD
		up.setLCD(LCDTitle, LCDElement);
#endif
		up.show();
		up.exec();
		up.hide();
	}
	show();
}
#endif

#ifdef ENABLE_FLASHTOOL
void eSoftwareUpdate::flash_tool()
{
	hide();
	eFlashtoolMain setup;
#ifndef DISABLE_LCD
	setup.setLCD(LCDTitle, LCDElement);
#endif
	setup.show();
	setup.exec();
	setup.hide();
	show();
}
#endif

