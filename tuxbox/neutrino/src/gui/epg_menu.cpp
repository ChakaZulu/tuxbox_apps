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



#include <global.h>
#include <neutrino.h>
#include <gui/widget/icons.h>

#include <gui/epg_menu.h>

#include <gui/eventlist.h>
#include <gui/infoviewer.h>
#include <gui/epgplus.h>
#include <gui/streaminfo.h>



//
//  -- EPG Menue Handler Class
//  -- to be used for calls from Menue
//  -- (2004-03-06 rasc)
// 

int CEPGMenuHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int           res = menu_return::RETURN_EXIT_ALL;


	if (parent) {
		parent->hide();
	}

	doMenu ();
	return res;
}




int CEPGMenuHandler::doMenu ()
{

	CMenuWidget EPGSelector("EPGMenu.head", "features.raw", 350);
	EPGSelector.addItem(GenericMenuSeparator);


	EPGSelector.addItem(new CMenuForwarder("EPGMenu.eventlist", true, NULL, new CEventListHandler(), "", true, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED), false);

	EPGSelector.addItem(new CMenuForwarder("EPGMenu.epgplus", true, NULL, new CEPGplusHandler(), "", true, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), false);

	EPGSelector.addItem(new CMenuForwarder("EPGMenu.eventinfo", true, NULL, new CInfoViewerHandler(), "", true, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);

	// -- Stream Info
	EPGSelector.addItem(new CMenuForwarder("EPGMenu.streaminfo", true, NULL, new CStreamInfo(), "", true, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE), false);


	EPGSelector.addItem(GenericMenuSeparator);
	return EPGSelector.exec(NULL, "");
}





