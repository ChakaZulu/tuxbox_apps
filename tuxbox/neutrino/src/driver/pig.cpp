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



#ifdef !_V4L_API_

//  -- altes PIG-API





#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "pig.h"




//
//  -- Picture in Graphics  Control
//  -- This module is a Class to provide a PIG abstraction layer
//  --
//  --  some source grabbed from fx2  (;-) )
//  --  2002-11  rasc
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

 char  *pigdevs[] = {
		PIG_DEV "0"		// PIG device 0
		// PIG_DEV "1",		// PIG device 1
		// PIG_DEV "2",		// PIG device ...
		// PIG_DEV "3"
 };
		

    if ( (pig_nr<0) || (pig_nr >= (int)(sizeof (pigdevs)/sizeof(char *))) ) {
    
	if (fd < 0) {
		fd = open( pigdevs[pig_nr], O_RDONLY );
		if (fd >= 0) {
			status = HIDE;
			px = py = pw = ph = 0;
		}
		return fd;
	}

    }

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
   return;
}


//
// -- set PIG Position
// -- routines should be self explaining
//

void CPIG::set_coord (int x, int y, int w, int h)
{
	set_xy (x,y);
	set_size (w,h);
}


void CPIG::set_xy (int x, int y)
{

	if (fd >= 0) {

		if (( x != px ) || ( y != py )) {
			avia_pig_set_pos(fd,x,y);
			px = x;
			py = y;
		}
	
	}

}


void CPIG::set_size (int w, int h)
{

	if (fd >= 0) {

		if (( w != pw ) || ( h != ph )) {
			avia_pig_set_size(fd,w,h);
			pw = w;
			ph = h;
		}

	}

}

// $$$ ???? what this for?

void CPIG::set_source (int x, int y)
{

	if (fd >= 0) {

		if (( x != px ) || ( y != py )) {
			avia_pig_set_source(fd,x,y);
		}

	}

}


//
// -- routine set's stack position of PIG on display
//

void CPIG::set_stackpos (int pos)

{
	if (fd >= 0) {
		avia_pig_set_stack(fd,pos);
		stackpos = pos;
	}
}


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
	if ( fd >= 0 ) {
		avia_pig_show(fd);
		status = SHOW;
	}
}

void CPIG::hide (void)
{
	if ( fd >= 0 ) {
		avia_pig_hide(fd);
		status = HIDE;
	}
}


CPIG::PigStatus  CPIG::getStatus(void)
{
	return status;

}



#else

//  Video4Linux API  for PIG




//   Todo.....  (rasc)



#endif   // _V4L_API_


