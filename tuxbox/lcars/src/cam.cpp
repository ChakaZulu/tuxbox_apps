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
$Log: cam.cpp,v $
Revision 1.4  2001/11/15 00:39:13  TheDOC
log-retry

*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <memory.h>

#include <ost/ca.h>

#include "cam.h"


cam::cam()
{
}

void cam::initialize()
{
	pid_count = 0;
}

void cam::cam_answer()
{
}

void cam::sendCAM(void *data, unsigned int len)
{
}

void cam::readCAID()
{
}

bool cam::isfree()
{
}

void cam::init()
{
}

void cam::init2()
{
}

void cam::reset()
{
}

void cam::start()
{
}

void cam::descramble()
{
}

void cam::startEMM()
{
}
