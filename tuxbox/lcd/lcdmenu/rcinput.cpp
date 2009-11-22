/*
 * $Id: rcinput.cpp,v 1.6 2009/11/22 15:36:43 rhabarber1848 Exp $
 * 
 * Remote Control Handling Class
 *
 * Copyright (C) 2001, 2002 Andreas Oberritter <obi@saftware.de>
 *
 * based on rcinput.cpp from Neutrino which is
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
 */

#include "rcinput.h"

/**************************************************************************
 *	Constructor - opens rc-input device and starts threads
 **************************************************************************/
CRCInput::CRCInput()
{
	fd=open(RC_DEVICE, O_RDONLY);
	if (fd<0)
	{
		perror(RC_DEVICE);
		exit(-1);
	}
	prevrccode = KEY_RESERVED;

    tv_prev.tv_sec = 0;
    repeat_block = 150000; // 150ms
}

/**************************************************************************
 *	Destructor - close the input-device
 **************************************************************************/
CRCInput::~CRCInput()
{
	if (fd>=0)
		close(fd);
}

/**************************************************************************
 *	get rc-key from the rcdevice
 **************************************************************************/
int CRCInput::getKey()
{
	long long td;
	struct input_event ev;

	while (1)
	{
		if (read(fd, &ev, sizeof(struct input_event)) != sizeof(struct input_event))
		{
			printf("key: empty\n");
			//return -1; error!!!
		}
		else
		{
			td = ev.time.tv_usec - tv_prev.tv_usec;
			td+= ( ev.time.tv_sec - tv_prev.tv_sec )* 1000000;

			if ( ( ( prevrccode != ev.code ) || ( td > repeat_block ) ) && ( ev.value ) )
			{
				tv_prev    = ev.time;
				prevrccode = ev.code;
				return ev.code;
			}
		}
	}
}
