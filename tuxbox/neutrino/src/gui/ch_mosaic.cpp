
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

#include <unistd.h>


#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <daemonc/remotecontrol.h>

#include <gui/channellist.h>

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

int CChMosaicHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	int       res = menu_return::RETURN_EXIT_ALL;
	CChMosaic * mosaic;


	if (parent) {
		parent->hide();
	}

	mosaic = new CChMosaic;
	mosaic->doMosaic ();
	delete mosaic;

	return res;
}




#define SCREEN_X	720
#define SCREEN_Y	572
#define RATIO(y)	(((y)*100)/126)		// w/h PAL ratio


//
//  -- Channel Mosaic Class
//  -- do Mosaic
// 

CChMosaic::CChMosaic()
{
	pig = new CPIG (0);
	current_pig_pos = 0;
}


CChMosaic::~CChMosaic()
{
	delete pig;

}


void CChMosaic::doMosaic()
{
  struct PIG_COORD  coord[] = {
	  	{ 10, 10, 170,RATIO(170) },
		{150,110, 170,RATIO(170) },
		{250,210, 170,RATIO(170) },
		{300,300, 170,RATIO(170) }
	};

  CChannelList  *channellist;
  int    	channel;


  channellist = CNeutrinoApp::getInstance()->channelList;
  channel     = channellist->getActiveChannelNumber();




  //  $$$ mute



   // experimental
  int i;
  for  (i=0; i < (int)(sizeof(coord)/sizeof(coord[0])); i++) {

	printf ("pig: %d \n",i);
	pig->show (coord[i].x,coord[i].y, coord[i].w, coord[i].h);

	channellist->zapTo(channel++);
	sleep (1);

	// zap, sleep 0.5 sec
	// capture frame
	
	// loop 4 times frame "empty"?
	// --> wait 0,5 sec, re-capture
	
	// display frame

	// display add info (sendername, epg info)
  }

  //  $$$ unmute

}





