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
//  --  adapted source from fx2  (;-) )
//  --  2002-11  rasc
//



//
// -- Constructor
//

void CPIG::CPIG(int pig_nr)
{
    fd = open_pig (pig_nr);
}

void CPIG::CPIG(int x, int y, int w, int h)
{
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

		is_active = false;
		px = py = pw = ph = 0;
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
	is_active = false;
	px = py = pw = ph = 0;
   }
   return;
}





