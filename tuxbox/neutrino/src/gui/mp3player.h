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
*/

#ifndef __mp3playergui__
#define __mp3playergui__


#include "driver/framebuffer.h"
#include "driver/mp3play.h"
#include "gui/widget/menue.h"
#include "gui/filebrowser.h"
#include "id3tag.h"

using namespace std;

class CMP3
{
	string Filename;
};

typedef std::vector<string> CPlayList;


class CMP3PlayerGui : public CMenuTarget
{
	public:

	private:
		CFrameBuffer		*frameBuffer;
		CFileBrowser		filebrowser;
//		CMP3Player			MP3Player;
		unsigned int		selected;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		int					fheight; // Fonthoehe Playlist-Inhalt
		int					theight; // Fonthoehe Playlist-Titel
		int					buttonHeight;
		bool				visible;			

		CPlayList			playlist;
		string				Path;

		int 			width;
		int 			height;
		int 			x;
		int 			y;

		void paintItem(int pos);
		void paint();
		void paintHead();
		void paintFoot();
		void hide();

	public:
		CMP3PlayerGui();
		~CMP3PlayerGui();
		int  show();
		int  exec(CMenuTarget* parent, string actionKey);
};


#endif


