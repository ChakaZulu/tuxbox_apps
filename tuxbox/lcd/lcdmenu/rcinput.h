/*
 * $Id: rcinput.h,v 1.4 2002/12/26 09:14:03 Jolt Exp $
 *
 * RemoteControle Handling Class
 *
 * Copyright (C) 2001, 2002 Andreas Oberritter <obi@saftware.de>
 *
 * based on rcinput.h from Neutrino which is
 * 
 * Copyright (C) 2001 Steffen Hehn
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Log: rcinput.h,v $
 * Revision 1.4  2002/12/26 09:14:03  Jolt
 * Ported to Input dev api
 *
 * Revision 1.3  2002/01/03 17:18:59  obi
 * some reorganization.
 * removed buffer classes.
 * removed threading from rcinput.
 * moved timer from rc-class to menu-class.
 * manually add fonts to fontRendererClass.
 * fixed small bugs that noone would ever find and added new ones.
 *
 *
 */

#ifndef __RCINPUT_H__
#define __RCINPUT_H__

#define RC_DEVICE "/dev/input/event0"

#include <linux/input.h>
#include <stdio.h>
#include <asm/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <utime.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <string>

using namespace std;

class CRCInput
{
	private:

		int             fd;
		struct timeval  tv_prev;
		__u16           prevrccode;

	public:
		
		int repeat_block;

		CRCInput(); 
		~CRCInput();

		int getKey();
};

#endif /* __RCINPUT_H__ */
