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


#ifndef __PIG_CONTROL__
#define __PIG_CONTROL__


//
//  -- Picture in Graphics  Control
//  --  adapted source from fx2  (;-) )
//  --  2002-11  rasc
//


#include <dbox/avia_gt_pig.h>


class CPIG
{
	public:
		void CPIG (int pig_nr = 0);
		void CPIG (int pig_nr, int x, int y, int w, int h);
		void ~CPIG (void);

		void set  (int x, int y, int w, int h);
		int  show (void);
		int  show (int x, int y, int w, int h);
		int  hide (void);

		void copy2buf (.....);

	private:
		int	fd;			// io descriptor
		int	px, py, pw, ph;		// pig frame
		int	is_active;		// on display?

		int  open_pig (int nr);
		void close_pig (void);


#define PIG_DEV "/dev/dbox/pig"
		static char  *pigdevs[] = {
			PIG_DEV "0"		// PIG device 0
			// PIG_DEV "1",		// PIG device 1
			// PIG_DEV "2",		// PIG device ...
			// PIG_DEV "3"
		}
};


#endif


