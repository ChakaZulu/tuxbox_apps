/*
	Neutrino-GUI  -   DBoxII-Project


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/





#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "pig.h"

#ifdef HAVE_TRIPLEDRAGON
#include <zapit/client/zapitclient.h>
#endif


//  Video4Linux API  for PIG
//

//
//  -- Picture in Graphics  Control
//  -- This module is a Class to provide a PIG abstraction layer
//  --
//  --  2002-11  rasc
//  --  2003-06  rasc   V4L Api
//



//
// -- Constructor
//

CPIG::CPIG()
{
    fd = -1;
    status = CLOSED;
}


CPIG::CPIG(int pig_nr)
{
    fd = -1;
    status = CLOSED;
    fd = pigopen (pig_nr);
}


CPIG::CPIG(int pig_nr, int x, int y, int w, int h)
{
    fd = -1;
    status = CLOSED;
    fd = pigopen (pig_nr);
    set_coord (x,y, w,h);

}

CPIG::~CPIG()
{
	pigclose ();		// cleanup anyway!!

}


//
// -- open PIG #nr
// -- return >=0: ok
//

int CPIG::pigopen (int pig_nr)
{

#ifndef HAVE_TRIPLEDRAGON
 char  *pigdevs[] = {
		PIG_DEV "0"		// PIG device 0
		// PIG_DEV "1",		// PIG device 1
		// PIG_DEV "2",		// PIG device ...
		// PIG_DEV "3"
 };
		

    if ( (pig_nr>0) || (pig_nr < (int)(sizeof (pigdevs)/sizeof(char *))) ) {
    
	if (fd < 0) {
		fd = open( pigdevs[pig_nr], O_RDWR );
		if (fd >= 0) {
			status = HIDE;
			px = py = pw = ph = 0;
		}
		return fd;
	}

    }
#else
	status = HIDE;
	px = py = pw = ph = 0;
#endif

    return -1;
}


//
// -- close PIG 
//

void CPIG::pigclose ()
{
   if (fd >=0 ) {
	close (fd);
	fd = -1;
	status = CLOSED;
	px = py = pw = ph = 0;
   }
#ifdef HAVE_TRIPLEDRAGON
	CZapitClient z;
	z.setPIG(0, 0, 0, 0, true);
#endif
   return;
}


//
// -- set PIG Position
// -- routines should be self explaining
//

void CPIG::_set_window (int x, int y, int w, int h)
{
  // -- Modul interne Routine
#ifndef HAVE_TRIPLEDRAGON
#if HAVE_DVB_API_VERSION < 3
	avia_pig_set_pos(fd, x, y);
	avia_pig_set_size(fd, w, h);
#else
	struct v4l2_crop crop;
	struct v4l2_format coord;
	int    err;

	crop.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	err = ioctl(fd, VIDIOC_G_CROP, &crop);

	// take whole input frame
	crop.c.left = 0;
	crop.c.top = 0;
	crop.c.width = 720;
	crop.c.height = 576;
	err = ioctl(fd, VIDIOC_S_CROP, &crop);
	
	coord.type = V4L2_BUF_TYPE_VIDEO_OVERLAY;
	err = ioctl(fd, VIDIOC_G_FMT, &coord);

	// fit into small window
	coord.fmt.win.w.left = x;
	coord.fmt.win.w.top = y;
	coord.fmt.win.w.width  = w;
	coord.fmt.win.w.height = h;

	err = ioctl(fd, VIDIOC_S_FMT, &coord);
#endif
#endif
}


void CPIG::set_coord (int x, int y, int w, int h)
{

	if (( x != px ) || ( y != py )) {
		px = x;
		py = y;
		pw = w;
		ph = h;
		_set_window (px,py,pw,ph);
	}

}


void CPIG::set_xy (int x, int y)
{

	if (( x != px ) || ( y != py )) {
		px = x;
		py = y;
		_set_window (px,py,pw,ph);
	}

}


void CPIG::set_size (int w, int h)
{

	if (( w != pw ) || ( h != ph )) {
		pw = w;
		ph = h;
		_set_window (px,py,pw,ph);
	}

}


// $$$ ???? what this for?

//void CPIG::set_source (int x, int y)
//{
//
//	if (fd >= 0) {
//
//		if (( x != px ) || ( y != py )) {
//	//		avia_pig_set_source(fd,x,y);
//		}
//
//	}
//
//}


//
// -- routine set's stack position of PIG on display
//

//void CPIG::set_stackpos (int pos)
//
//{
//	if (fd >= 0) {
//		avia_pig_set_stack(fd,pos);
//		stackpos = pos;
//	}
//}


//
// -- Show PIG or hide PIG
//

void CPIG::show (int x, int y, int w, int h)
{
	set_coord (x,y, w,h);
	show ();
}

void CPIG::show (void)
{
#ifndef HAVE_TRIPLEDRAGON
	if ( fd >= 0 ) {
#if HAVE_DVB_API_VERSION < 3
		avia_pig_show(fd);
#else
		int pigmode = 1;
		int err;
		err = ioctl(fd, VIDIOC_OVERLAY, &pigmode);
		// old API: avia_pig_show(fd);
#endif
		status = SHOW;
	}
#else
	CZapitClient z;
	z.setPIG(px, py, pw, ph, true);
	status = SHOW;
#endif
}

void CPIG::hide (void)
{
#ifndef HAVE_TRIPLEDRAGON
	if ( fd >= 0 ) {
#if HAVE_DVB_API_VERSION < 3
		avia_pig_hide(fd);
#else
		int pigmode = 0;
		int err;
		err = ioctl(fd, VIDIOC_OVERLAY, &pigmode);
		// old API: avia_pig_hide(fd);
#endif
		status = HIDE;
	}
#else
	CZapitClient z;
	z.setPIG(0, 0, 0, 0, true);
	status = HIDE;
#endif
}


CPIG::PigStatus  CPIG::getStatus(void)
{
	return status;

}





//
// ToDo's:
//   -- Capability unavail check/status
//   -- Cropping for resizing (possible zoom/scaling?)
//   -- Capturing
//

