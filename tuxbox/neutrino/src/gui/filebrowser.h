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

#include <string.h>


#include <daemonc/remotecontrol.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <system/settings.h>

#include "color.h"
#include "infoviewer.h"

#include "widget/menue.h"
#include "widget/progresswindow.h"

#define VLC_URI "vlc://"

using namespace std;

class CFileFilter
{
	vector<string> Filter;
public:
	void addFilter(string filter){Filter.push_back(filter);};
	bool matchFilter(string name)
	{
		int ext_pos = 0;
		ext_pos = name.rfind(".");
		if( ext_pos > 0)
		{
			string extension;
			extension = name.substr(ext_pos + 1, name.length() - ext_pos);
			for(unsigned int i = 0; i < Filter.size();i++)
				if(strcasecmp(Filter[i].c_str(),extension.c_str()) == 0)
					return true;
		}
		return false;
	};
};

class CFile
{
public:
	enum 
	{
		FILE_UNKNOWN = 0,
		FILE_DIR,
		FILE_TEXT,
		FILE_MP3,
		FILE_MP3_PLAYLIST,
		FILE_PICTURE
	};

	int		getType();
	string	getFileName();
	string	getPath();

	CFile(){Marked = false; Size=0;Mode=0;Time=0;};
	off_t Size;
	string Name;
	mode_t Mode;
	bool Marked;
	time_t Time;
};

typedef vector<CFile> CFileList;

class CFileBrowser
{
	private:
		CFrameBuffer	*frameBuffer;

		CFileList		filelist;
		CFileList		selected_filelist;
		bool			readDir(string dirname);
		bool			readDir_vlc();
		bool			readDir_std();
		void			addRecursiveDir(CFileList * re_filelist, string path, bool bRootCall, CProgressWindow * progress = NULL);

		unsigned int		selected;
		unsigned int		liststart;
		unsigned int		listmaxshow;
		int 			fheight;	// Fonthoehe Filelist-Inhalt
		int 			theight;	// Fonthoehe Filelist-Titel
		int			foheight;	// Hoehe der button leiste
		int			smode;		// Sortierungsart

		string			name;
		std::string		m_baseurl;
		int 			width;
		int 			height;
		bool			use_filter;
		bool			bCancel;

		int 			x;
		int 			y;

		void ChangeDir(string filename);

		void paintItem(unsigned pos, unsigned int spalte = 0);
		void paint();
		void paintHead();
		void paintFoot();
		void hide();

	public:
		string			Path;
		bool			Multi_Select;
		bool			Dirs_Selectable;
		bool        Dir_Mode;
		CFileFilter *	Filter;

		CFileBrowser();
		~CFileBrowser();

		bool		exec(string Dirname);
		CFile		*getSelectedFile();
		CFileList	*getSelectedFiles();
		string getCurrentDir() {return Path;}
//		size_t CurlWriteToString(void *ptr, size_t size, size_t nmemb, void *data);
};




#endif
