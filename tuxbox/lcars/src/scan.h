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
$Log: scan.h,v $
Revision 1.4  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.3  2001/12/17 01:00:34  tux
scan.cpp fix

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef SCAN_H
#define SCAN_H

#include "settings.h"
#include "channels.h"
#include "pat.h"
#include "pmt.h"
#include "nit.h"
#include "sdt.h"
#include "osd.h"
#include "tuner.h"

#include "config.h"

class scan
{
	settings *setting;
	pat *pat_obj;
	pmt *pmt_obj;
	nit *nit_obj;
	sdt *sdt_obj;
	osd *osd_obj;
	tuner *tuner_obj;
	channels *channels_obj;
public:
	scan::scan(settings *s, pat *p1, pmt *p2, nit *n, sdt *s1, osd *o, tuner *t, channels *c);
	channels scanChannels(bool full = false, int start_frequency = -1, int start_symbol = -1, int start_polarization = -1, int start_fec = -1);
	void updateChannels(channels *chan);
	void readUpdates();
};

#endif
