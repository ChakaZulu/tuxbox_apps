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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino.h>

#include <gui/rc_lock.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput.h>




//
// -- Menue Handler Interface
// -- Infinite Loop to lock remote control (until release lock key pressed)
// -- 2003-12-01 rasc
//

int CRCLock::exec(CMenuTarget* parent, const std::string &)
{

	if (parent)
		parent->hide();


	ShowMsgUTF(LOCALE_RCLOCK_TITLE, g_Locale->getText(LOCALE_RCLOCK_LOCKMSG  ), CMessageBox::mbrYes , CMessageBox::mbYes , "info.raw");



	// -- Lockup Box
	
	lockBox ();



	ShowMsgUTF(LOCALE_RCLOCK_TITLE, g_Locale->getText(LOCALE_RCLOCK_UNLOCKMSG), CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw");

	return  menu_return::RETURN_EXIT_ALL;
}






void CRCLock::lockBox(void)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	unsigned long long timeoutEnd;


	// -- Loop until release key pressed
	// -- Key sequence:  <RED> <DBOX> within 5 secs


	while  (1) {

		timeoutEnd = CRCInput::calcTimeoutEnd( 9999999 );
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);


		if ( msg == CRCInput::RC_red)  {
			timeoutEnd = CRCInput::calcTimeoutEnd( 5 );
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			if ( msg == CRCInput::RC_setup)  break;
		}

		if ( msg == CRCInput::RC_timeout ) continue;

		// -- Zwen told me: Eating only RC events would be nice
		// -- so be it...

		if ( msg >  CRCInput::RC_MaxRC ) {
			CNeutrinoApp::getInstance()->handleMsg(msg, data); 
		}

	}

	return;
}


