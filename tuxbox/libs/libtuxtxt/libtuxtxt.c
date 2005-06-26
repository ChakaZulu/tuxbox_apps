/******************************************************************************
 *                      <<< TuxTxt - Teletext Plugin >>>                      *
 *                                                                            *
 *             (c) Thomas "LazyT" Loewe 2002-2003 (LazyT@gmx.net)             *
 *                                                                            *
 *    TOP-Text Support 2004 by Roland Meier <RolandMeier@Siemens.com>         *
 *    Info entnommen aus videotext-0.6.19991029,                              *
 *    Copyright (c) 1994-96 Martin Buck  <martin-2.buck@student.uni-ulm.de>   *
 *                                                                            *
 ******************************************************************************/

#define DEBUG 1

#include <sys/ioctl.h>
#include <fcntl.h>

#include "tuxtxt_def.h"
#include "tuxtxt_common.h"






/******************************************************************************
 * Initialize                                                                 *
 ******************************************************************************/

int Initialize()
{
	/* init data */
	memset(&astCachetable, 0, sizeof(astCachetable));
	memset(&astP29, 0, sizeof(astP29));

	clear_cache();
	receiving = 0;
	thread_starting = 0;
	vtxtpid = -1;
	return init_demuxer();
}

/******************************************************************************
 * Interface to caller                                                        *
 ******************************************************************************/

int tuxtxt_stop()
{
	if (!receiving) return 1;
	receiving = 0;

	return stop_thread();
}
void tuxtxt_start(int tpid)
{
	if (!initialized) initialized = Initialize();

	if (initialized)
	{
		if (vtxtpid != tpid)
		{
			tuxtxt_stop();
			clear_cache();
			page = 0x100;
			vtxtpid = tpid;
			start_thread();
		}
		else if (!thread_starting)
		{
			start_thread();
		}
	}

}
void tuxtxt_close()
{
#if DEBUG
	printf ("cleaning up\n");
#endif
	tuxtxt_stop();
	if (dmx != -1)
    	    close(dmx);
	dmx = -1;
	clear_cache();
}

/* Local Variables: */
/* indent-tabs-mode:t */
/* tab-width:3 */
/* c-basic-offset:3 */
/* comment-column:0 */
/* fill-column:120 */
/* End: */
