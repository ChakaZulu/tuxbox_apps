/*
  Neutrino-GUI  -   DBoxII-Project

  Copyright (C) 2003,2004 gagga
  Homepage: http://www.giggo.de/dbox

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

#ifndef __bookmarkmanager__
#define __bookmarkmanager__

#include <config.h>
#include <configfile.h>
#include <stdio.h>

#include <string>
#include <vector>
#include <map>

#include <driver/framebuffer.h>
#include <gui/widget/menue.h>

#define MAXBOOKMARKS 10
#define BOOKMARKFILE "/var/tuxbox/config/bookmarks"

class CBookmark
{
    private:
        std::string name;
        std::string url;
        std::string time;

    public:
    	CBookmark(std::string name, std::string url, std::string time);
    	CBookmark();
    	~CBookmark();
    	std::string getName();                
    	std::string getUrl();                
    	std::string getTime();
    	void setName(std::string name);                
    	void setUrl(std::string url);
    	void setTime(std::string time);

            
};

//-----------------------------------------

class CBookmarkManager
{
 private:
	std::vector<CBookmark> bookmarks;
	CConfigFile	bookmarkfile;
	
    CFrameBuffer		*frameBuffer;
	unsigned int		selected;
	unsigned int		liststart;
	unsigned int		listmaxshow;
	int					fheight; // Fonthoehe Timerlist-Inhalt
	int					theight; // Fonthoehe Timerlist-Titel
	int                 buttonHeight;
	bool				visible;			
	int 			width;
	int 			height;
	int 			x;
	int 			y;

	
	//int bookmarkCount;
	bool bookmarksmodified;
	void readBookmarkFile();
	void writeBookmarkFile();
	CBookmark getBookmark();
	int addBookmark(CBookmark inBookmark);
	
    void paintItem(int pos);
	void paint();
	void paintHead();
	void paintFoot();
	void hide();



 public:
	CBookmarkManager();
	~CBookmarkManager();
	int createBookmark(std::string name, std::string url, std::string time);
	int createBookmark(std::string url, std::string time);
	int createBookmark(CBookmark bookmark);
	void removeBookmark(unsigned int index);
	void renameBookmark(unsigned int index);
	int getBookmarkCount();
	int getMaxBookmarkCount();
	void flush();
	
	CBookmark * getBookmark(CMenuTarget* parent);
};

#endif
