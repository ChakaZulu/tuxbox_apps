work in progress   (rasc)


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

void CPIG::CPIG(int pig_nr)
{
    fd = -1;
    status = CLOSED;
    fd = open_pig (pig_nr);
}


void CPIG::CPIG(int pig_nr, int x, int y, int w, int h)
{
    fd = -1;
    status = CLOSED;
    fd = open_pig (pig_nr);
    set_coord (x,y, w,h);

}


//
// -- open PIG #nr
// -- return >=0: ok
//

int CPIG::open_pig (int pig_nr)
{

    if (pig_nr < sizeof (char *pigdevs[])/sizeof(char *) ) {
    
	if (fd == -1) {
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

void CPIG::close_pig (int pig_nr)
{
   if (fd != -1) {
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

	if (fd == -1) return;

	if (( x != px ) || ( y != py )) {
		avia_pig_set_pos(fd,x,y);
		px = x;
		py = y;
	}

}


void CPIG::set_size (int w, int h)
{

	if (fd == -1) return;

	if (( w != pw ) || ( h != ph )) {
		avia_pig_set_size(fd,width,height);
		pw = w;
		ph = h;
	}

}

// $$$ ???? what this for?

void CPIG::set_source (int x, int y)
{

	if (fd == -1) return;

	if (( w != pw ) || ( h != ph )) {
		avia_pig_set_source(fd,x,y);
	}

}


//
// -- routine set's stack position of PIG on display
//

void CPIG::set_stack (int pos)

{
	if (fd == -1) return;

	avia_pig_set_stack(fd,pos);
	stackpos = pos;
}


//
// -- Show PIG or hide PIG
//

void CPIG::show (int x, int y, int w, int h)
{
	set_coord (x,y, w,h);
	show (void);
}

void CPIG::show (void)
{
	if ( fd != -1 ) {
		avia_pig_show(fd);
		status = SHOW;
	}
}

void CPIG::hide (void)
{
	if ( fd != -1 ) {
		avia_pig_hide(fd);
		status = HIDE;
	}
}


CPIG::PigStatus  CPIG::getStatus(void)
{
	return status;

}
