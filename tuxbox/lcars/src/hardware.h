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
$Log: hardware.h,v $
Revision 1.3  2001/12/11 13:38:44  TheDOC
new cdk-path-variables, about 10 new features and stuff

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#ifndef HARDWARE_H
#define HARDWARE_H

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <dbox/avs_core.h>
#include <ost/audio.h>

#include "settings.h"

#define OUTPUT_FBAS 0
#define OUTPUT_RGB 1

class hardware
{
	int fblk;
	bool muted;
	int avs;
	settings setting;
	bool vcr_on;
	bool old_DD_state;
public:	
	hardware(settings &s);
	void hardware::setOutputMode(int i);
	void setfblk(int i);
	int getfblk();
	bool switch_vcr();
	void switch_mute();
	bool isMuted() { return muted; }
	int vol_plus(int value);
	int vol_minus(int value);
	void fnc(int i);
	void shutdown();
	void reboot();
	void useDD(bool use);
};

#endif
