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
#include <gui/widget/menue.h>

extern CRemoteControl		* g_RemoteControl; /* neutrino.cpp */
extern CAPIDChangeExec		* APIDChanger;
extern CAudioSetupNotifier	* audioSetupNotifier;

#include <gui/audio_select.h>



//
//  -- AUDIO Selector Menue Handler Class
//  -- to be used for calls from Menue
//  -- (2005-08-31 rasc)
// 



// -- this is a copy from neutrino.cpp!!
#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, LOCALE_AUDIOMENU_STEREO    },
	{ 1, LOCALE_AUDIOMENU_MONOLEFT  },
	{ 2, LOCALE_AUDIOMENU_MONORIGHT }
};



int CAudioSelectMenuHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int           res = menu_return::RETURN_EXIT_ALL;


	if (parent) {
		parent->hide();
	}

	doMenu ();
	return res;
}




int CAudioSelectMenuHandler::doMenu ()
{
	CMenuWidget AudioSelector(LOCALE_AUDIOSELECT_MENU_HEAD, "audio.raw", 360);


	// -- setup menue due to Audio PIDs

	if (g_RemoteControl->current_PIDs.APIDs.size() > 1) {

	   for( unsigned int count=0; count < g_RemoteControl->current_PIDs.APIDs.size(); count++ ) {
		char apid[5];
		sprintf(apid, "%d", count);
		AudioSelector.addItem(new CMenuForwarderNonLocalized(
				g_RemoteControl->current_PIDs.APIDs[count].desc, true, NULL,
				APIDChanger, apid, CRCInput::convertDigitToKey(count + 1)),
				(count == g_RemoteControl->current_PIDs.PIDs.selected_apid));
	   }

	}


	// -- setup menue for to Dual Channel Stereo


	if (1) {

	   AudioSelector.addItem(GenericMenuSeparatorLine);

	   CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_ANALOGOUT,
				&g_settings.audio_AnalogMode,
				AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT,
				true, audioSetupNotifier);

	   AudioSelector.addItem( oj );

	}


	AudioSelector.addItem(GenericMenuSeparator);

	return AudioSelector.exec(NULL, "");
}



