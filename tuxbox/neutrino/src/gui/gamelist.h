/*
	Neutrino-GUI  -   DBoxII-Project
 
	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/
 
	Kommentar:
 
	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.
	
 
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
 
$Id: gamelist.h,v 1.7 2002/01/03 20:03:20 McClean Exp $
 
$Log: gamelist.h,v $
Revision 1.7  2002/01/03 20:03:20  McClean
cleanup

Revision 1.6  2001/12/05 21:17:50  rasc
gamelist 2-spaltig mit description... Font muss noch verbessert werden
 
 
*/

#ifndef __gamelist__
#define __gamelist__

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"
#include "daemonc/remotecontrol.h"
#include "helpers/settings.h"
#include "plugins/gameplugins.h"
#include "menue.h"
#include "color.h"

#include <string>
#include <vector>

using namespace std;

class CGameList : public CMenuTarget
{
		struct game
		{
			int         number;
			string      filename;
			string      name;
			string		desc;
			string		depend;
		};

		unsigned int		liststart;
		unsigned int		listmaxshow;
		unsigned int		selected;
		int					key;
		string				name;
		vector<game*>	gamelist;

		int			fheight; // Fonthoehe Channellist-Inhalt
		int			theight; // Fonthoehe Channellist-Titel

		int			fheight1,fheight2;

		int 			width;
		int 			height;
		int 			x;
		int 			y;

		void paintItem(int pos);
		void paint();
		void paintHead();
		void runGame(int selected );

	public:
		CGameList( string Name );
		~CGameList();

		void hide();
		int exec(CMenuTarget* parent, string actionKey);

};


#endif



