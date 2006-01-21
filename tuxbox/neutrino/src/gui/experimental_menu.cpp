/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino.h>
#include <gui/widget/icons.h>

#include <gui/experimental_menu.h>
//#include <gui/ch_mosaic.h>



int CExperimentalSettingsMenuHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int           res = menu_return::RETURN_EXIT_ALL;


	if (parent) {
		parent->hide();
	}

	doMenu ();
	return res;
}

int CExperimentalSettingsMenuHandler::doMenu ()
{
	char id[5];

	CMenuWidget ExperimentalSettings(LOCALE_EXPERIMENTALSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS, 280);
	
        ExperimentalSettings.addItem(GenericMenuSeparator);

	// the following to lines are examples
	//ExperimentalSettings.addItem( new CMenuOptionNumberChooser(NONEXISTANT_LOCALE, (int*) &g_settings.show_ca_status, true, 0, 1, 0, 0, LOCALE_OPTIONS_OFF, "show CA Status"));
	//ExperimentalSettings.addItem(new CMenuForwarderNonLocalized("experimental1", true, NULL, new					CChMosaicHandler(), id, CRCInput::RC_nokey, ""), false);
	
	
	return ExperimentalSettings.exec(NULL, "");
}

