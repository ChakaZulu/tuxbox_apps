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
$Log: container.h,v $
Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef CONTAINER_H
#define CONTAINER_H

#include "zap.h"
#include "channels.h"
#include "fb.h"
#include "osd.h"
#include "settings.h"
#include "tuner.h"
#include "pat.h"
#include "pmt.h"
#include "eit.h"
#include "scan.h"

class container
{

public:
	settings *settings_obj;
	zap *zap_obj;
	channels *channels_obj;
	fbClass *fbClass_obj;
	osd *osd_obj;
	tuner *tuner_obj;
	pat *pat_obj;
	pmt *pmt_obj;
	eit *eit_obj;
	scan *scan_obj;

	container(zap *z, channels *c, fbClass *f, osd *o, settings *s, tuner *t, pat *pa, pmt *pm, eit *e, scan *sc);
};
#endif
