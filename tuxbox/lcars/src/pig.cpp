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
$Log: pig.cpp,v $
Revision 1.6  2003/01/05 19:52:47  TheDOC
forgot include

Revision 1.5  2003/01/05 19:28:45  TheDOC
lcars should be old-api-compatible again

Revision 1.4  2002/03/03 22:56:27  TheDOC
lcars 0.20

Revision 1.2  2001/12/16 22:36:05  tux
IP Eingaben erweitert

Revision 1.3  2001/11/19 10:08:10  TheDOC
pig fixed

Revision 1.2  2001/11/15 00:43:45  TheDOC
 added

*/
#include "pig.h"
#include "devices.h"

#ifdef HAVE_LINUX_DVB_VERSION_H
pig::pig()
{
	fd = open("/dev/v4l/video0", O_RDWR);
}

pig::~pig()
{
	close(fd);
}

void pig::show()
{
	//avia_pig_show(fd);
}

void pig::hide()
{
	//avia_pig_hide(fd);
}

void pig::setSize(int x, int y)
{
	//avia_pig_set_size(fd, x, y);
}

void pig::setPosition(int x, int y)
{
	//avia_pig_set_pos(fd, x, y);
}

void pig::setStack(int i)
{
	//avia_pig_set_stack(fd, i);
}

void pig::setSource(int x1, int y1, int x2, int y2)
{
}

#elif HAVE_OST_DMX_H

pig::pig()
{
	fd = open("/dev/dbox/pig0", O_RDWR);
}

pig::~pig()
{
	close(fd);
}

void pig::show()
{
	avia_pig_show(fd);
}

void pig::hide()
{
	avia_pig_hide(fd);
}

void pig::setSize(int x, int y)
{
	avia_pig_set_size(fd, x, y);
}

void pig::setPosition(int x, int y)
{
	avia_pig_set_pos(fd, x, y);
}

void pig::setStack(int i)
{
	avia_pig_set_stack(fd, i);
}

void pig::setSource(int x1, int y1, int x2, int y2)
{
}

#endif
