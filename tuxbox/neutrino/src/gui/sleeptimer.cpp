/*
 * Code Konflikt - will be included in timerdaemon...
 * so this module will be removed...
*/

#include <stdlib.h>

#include "global.h"
#include "neutrino.h"
#include "sleeptimer.h"

#include "gui/widget/stringinput.h"

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

	if (parent)
	{
		parent->hide();
	}
   

	// retrieve old code from cvs please, if needed

	ShowMsg ( "Sleeptimer", "timer module removed, this will re-coded later into the timerd", CMessageBox::mbrBack, CMessageBox::mbBack, "info.raw" );



	return res;
}


