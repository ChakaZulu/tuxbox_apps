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
$Log: zap.h,v $
Revision 1.5  2002/06/08 20:21:09  TheDOC
adding the cam-sources with slight interface-changes

Revision 1.4  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.3  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef ZAP_H
#define ZAP_H

#include "settings.h"
#include "osd.h"
#include "tuner.h"
#include "cam.h"

class zap
{
	int vid, aud, frontend, video, audio, pcr;
	int old_frequ;
	settings setting;
	osd osdd;
	tuner tune;
	cam ca;
	int old_TS;
public:
	zap(settings &set, osd &o, tuner &t, cam &c);
	~zap();

	void zap_allstop();
	void zap_to(pmt_data pmt, int VPID, int APID, int PCR, int ECM, int SID, int ONID, int TS, int PID1 = -1, int PID2 = -1);
	void zap_audio(int VPID, int APID, int ECM, int SID, int ONID);
	void close_dev();
	void dmx_start();
	void dmx_stop();
};
#endif // ZAP_H
