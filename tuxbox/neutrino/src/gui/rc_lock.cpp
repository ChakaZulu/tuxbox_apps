/*
	Neutrino-GUI  -   DBoxII-Project

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

#include <gui/rc_lock.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>




//
// -- Menue Handler Interface
// -- to fit the MenueClasses from McClean
//
// -- Infinite Loop to lock remote control (until release lock key pressed)
// -- 2003-12-01 rasc
//

int CRCLock::exec(CMenuTarget* parent, const std::string &)
{
	unsigned long long timeoutEnd;
	uint 		msg;
	uint 		data;



	if (parent)
		parent->hide();


	ShowMsgUTF("rclock.header", g_Locale->getText("rclock.lockmsg"),
		CMessageBox::mbrYes, CMessageBox::mbYes, "info.raw");


	// -- show hint on LCD
	// $$$ TODO
	
	

	// -- Loop until release key pressed
	// -- Key sequence:  <RED> <DBOX> within 5 secs


	while  (1) {

		timeoutEnd = CRCInput::calcTimeoutEnd( 9999999 );
		g_RCInput->getMsgAbsoluteTimeout( &msg, (uint*) (&data), &timeoutEnd );


		if ( msg == CRCInput::RC_timeout ) continue;

		if ( msg == CRCInput::RC_red)  {
			timeoutEnd = CRCInput::calcTimeoutEnd( 5 );
			g_RCInput->getMsgAbsoluteTimeout( &msg, (uint*) (&data), &timeoutEnd );

			if ( msg == CRCInput::RC_timeout ) continue;
			if ( msg == CRCInput::RC_setup)  break;
		}


	}



	ShowMsgUTF("rclock.header", g_Locale->getText("rclock.releasemsg"),
		CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw");

	return  menu_return::RETURN_EXIT_ALL;
}
