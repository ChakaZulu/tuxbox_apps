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

#include "global.h"
#include "neutrino.h"
#include "sleeptimer.h"

#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "../system/timer.h"


//
// -- Input Widget for setting shutdown time
// -- Menue Handler Interface
// -- to fit the MenueClasses from McClean
// -- Add current channel to Favorites and display user messagebox
//

int CSleepTimerWidget::exec(CMenuTarget* parent, string)
{
	int    res = menu_return::RETURN_EXIT_ALL;
	int    shutdown_min;

	if (parent)
	{
		parent->hide();
	}
   

	shutdown_min = g_Timer->actionGetShutdown () / 60;


shutdown_min = 5;
ShowMsg ( "Sleeptimer", "... TEST: set to 5 minutes fix", CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw" );



	g_Timer->actionSetShutdown (shutdown_min * 60);

	return res;
}


