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

#ifndef TUNER_H
#define TUNER_H

#include "settings.h"

class tuner
{
	settings setting;
public:
	tuner(settings &s);
	int tune(int frequ, int symbol, int polarization = -1, int fec = -1, int diseqc = 1);
};

#endif
