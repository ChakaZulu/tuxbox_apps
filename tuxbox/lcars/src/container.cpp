/***************************************************************************
    copyright            : (C) 2001 by TheDOC
    email                : thedoc@chatville.de
	homepage			 : www.chatville.de
	modified by			 : -
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*
$Log: container.cpp,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include "container.h"

container::container(zap *z, channels *c, fbClass *f, osd *o, settings *s, tuner *t, pat *pa, pmt *pm, eit *e, scan *sc)
{
	zap_obj = z;
	channels_obj = c;
	fbClass_obj = f;
	osd_obj = o;
	settings_obj = s;
	tuner_obj = t;
	pat_obj = pa;
	pmt_obj = pm;
	eit_obj = e;
	scan_obj = sc;
	printf("CONTAINER: %d\n", (*channels_obj).numberChannels());
}
