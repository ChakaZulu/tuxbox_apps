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

#include <stdlib.h>

#include "global.h"
#include "neutrino.h"
#include "sleeptimer.h"

#include "gui/widget/stringinput.h"
#include "system/timer.h"

#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"

//
// -- Input Widget for setting shutdown time
// -- Menue Handler Interface
// -- to fit the MenueClasses from McClean
// -- Add current channel to Favorites and display user messagebox
//

int CSleepTimerWidget::exec(CMenuTarget* parent, string)
{
	int    res = menu_return::RETURN_EXIT_ALL;
	int    shutdown_min, t;
	char   value[16];
	CStringInput  *inbox;

	if (parent)
	{
		parent->hide();
	}
   

	// remaining shutdown time?
	t = g_Timer->actionGetShutdown ();
	shutdown_min = t / 60;
	if ( (t>0) && ((t%60) > 30) ) {
		shutdown_min++;
	}

	sprintf(value,"%03d",shutdown_min);
	inbox = new CStringInput("sleeptimerbox.title",value,3,"sleeptimerbox.hint1","sleeptimerbox.hint2","0123456789 ");
	inbox->exec (NULL, "");
	inbox->hide ();

	delete inbox;

	shutdown_min = atoi (value);
	g_Timer->actionSetShutdown (shutdown_min * 60);

	return res;
}


