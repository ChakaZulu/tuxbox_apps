work in progress...  rasc

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

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>

#include "ch_mosaic.h"


/*
   -- Channel/Service Mosaic
   -- Display multiple channel images on screen
   -- capture used from outdoor (tmbinc)
   -- 2002-11   rasc
 */




//
//  -- init Channel Mosaic Handler Class
//  -- to be used for calls from Menue
// 

int CChMosaicHandler::exec(CMenuTarget* parent, string)
{
	int       res = menu_return::RETURN_EXIT_ALL;
	CChMosaic mosaic;


	if (parent) {
		parent->hide();
	}


	mosaic.doMosaic (void);

	return res;
}




#define SCREEN_X	720
#define SCREEN_Y	572


//
//  -- Channel Mosaic Class
//  -- do Mosaic
// 

void CChMosaic::CChMosaic(void)
{
	pig = new ( CPIG(0) );
}


void CChMosaic::~CChMosaic(void)
{
	delete pig;

}


void CChMosaic::doMosaic(void)
{




}


