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
$Log: tuner.h,v $
Revision 1.5  2002/05/18 02:55:24  TheDOC
LCARS 0.21TP7

Revision 1.4  2002/03/03 22:57:59  TheDOC
lcars 0.20

Revision 1.3  2001/12/07 14:10:33  rasc
Fixes for SAT tuning and Diseqc. Diseqc doesn't work properly for me (diseqc 2.0 switch).
Someone should check this please..

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef TUNER_H
#define TUNER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <memory.h>
#include <ost/dmx.h>
#include <ost/video.h>
#include <ost/frontend.h>
#include <ost/audio.h>
#include <ost/sec.h>
#include <ost/sec.h>
#include <ost/ca.h>
#include <dbox/avs_core.h>

#include "settings.h"

class tuner
{
	settings *setting;
public:
	tuner(settings *s);
	CodeRate getFEC(int fec);
	int tune(int frequ, int symbol, int polarization = -1, int fec = 0, int diseqc = 0);
};

#endif
