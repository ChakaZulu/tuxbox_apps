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
	int    shutdown_min;
	char   value[16];
	CStringInput  *inbox;

	if (parent)
	{
		parent->hide();
	}
   
	CTimerdClient * timerdclient = new CTimerdClient;

	shutdown_min = timerdclient->getSleepTimerRemaining();  // remaining shutdown time?
//	if(shutdown_min == 0)		// no timer set
//		shutdown_min = 10;		// set to 10 min default

	sprintf(value,"%03d",shutdown_min);
	inbox = new CStringInput("sleeptimerbox.title",value,3,"sleeptimerbox.hint1","sleeptimerbox.hint2","0123456789 ");
	inbox->exec (NULL, "");
	inbox->hide ();

	delete inbox;

	shutdown_min = atoi (value);
	printf("sleeptimer min: %d\n",shutdown_min);
	if (shutdown_min == 0)			// if set to zero remove existing sleeptimer
	{
		if(timerdclient->getSleeptimerID() > 0)
		{
			timerdclient->removeTimerEvent(timerdclient->getSleeptimerID());
		}
	}
	else							// set the sleeptimer to actual time + shutdown mins and announce 1 min before
		timerdclient->setSleeptimer(time(NULL) + ((shutdown_min -1) * 60),time(NULL) + shutdown_min * 60,0);
	delete timerdclient;
	return res;
}


