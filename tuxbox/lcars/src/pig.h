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

#ifndef PIG_H
#define PIG_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <dbox/avia_pig.h>

class pig
{
	int fd;
public:	
	pig();
	~pig();
	void show();
	void hide();
	void setSize(int x, int y);
	void setPosition(int x, int y);
	void setStack(int i);
	void setSource(int x1, int y1, int x2, int y2);
};

#endif
