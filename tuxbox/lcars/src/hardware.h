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

#ifndef HARDWARE_H
#define HARDWARE_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dbox/avs_core.h>

#include "settings.h"

class hardware
{
	int fblk;
	bool muted;
	int avs;
	settings setting;
	bool vcr_on;
public:	
	hardware(settings &s);
	void setfblk(int i);
	int getfblk() { return fblk; }
	bool switch_vcr();
	void switch_mute();
	bool isMuted() { return muted; }
	int vol_plus(int value);
	int vol_minus(int value);
	void fnc(int i);
	void shutdown();
	void reboot();
};

#endif
