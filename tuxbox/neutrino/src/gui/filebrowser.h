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


#ifndef __FILEBROWSER_HPP__
#define __FILEBROWSER_HPP__

#include <string>
#include <vector>



#include <daemonc/remotecontrol.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <system/settings.h>
#include <driver/mp3play.h>

#include "color.h"
#include "infoviewer.h"

#include "widget/menue.h"


using namespace std;

struct CFile
{
	off_t Size;
	string Name;
	mode_t Mode;

};

typedef std::vector<CFile> CCFileBrowser;

class CFileBrowser
{
	private:
		CFrameBuffer	*frameBuffer;
		CCFileBrowser	filelist;
		bool			readDir(string dirname);
		unsigned int	selected;
		unsigned int	current_event;
		unsigned int	liststart;
		unsigned int	listmaxshow;
		int 			fheight; // Fonthoehe Filelist-Inhalt
		int 			theight; // Fonthoehe Filelist-Titel

		string			name;

		int 			width;
		int 			height;
		int 			x;
		int 			y;

		void ChangeDir(string filename);

		void paintItem(unsigned pos, unsigned int spalte = 0);
		void paint();
		void paintHead();
		void hide();

	public:
		CFile		current_file;
		string		path;

		CFileBrowser();
		~CFileBrowser();

		string exec(const std::string& dirname);
};


#endif
