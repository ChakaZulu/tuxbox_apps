/*
 * $Id: movieplayer.h,v 1.9 2006/01/24 20:00:42 digi_casi Exp $
 *
 * (C) 2005 by digi_casi <digi_casi@tuxbox.org>
 *          based on vlc plugin by mechatron
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
 
#include <lib/gui/emessage.h>
#include <lib/gui/eskin.h>
#include <lib/gui/listbox.h>
#include <lib/gui/ewindow.h>
#include <lib/system/econfig.h>
#include <lib/movieplayer/movieplayer.h>
#include <lib/movieplayer/mpconfig.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>

class PLAYLIST
{
public:
	int Filetype;
	eString Filename, Fullname;
	eListBoxEntryText *listEntry;
};

typedef std::vector<PLAYLIST> PlayList;

class eSCGui: public eWindow
{
	enum {GOUP, DIRS, FILES};
	enum {DATA, VCD, SVCD, DVD};

	struct server_value
	{
		eString IP, IF_PORT, AUTH;
	} send_parms;

	PlayList playList;
	
	struct serverConfig server;

	eString startdir, cddrive;
	
	bool menu;
	unsigned int val;

	eListBox<eListBoxEntryText> *list;
	eTimer *timer;
	eMessageBox *bufferingBox;
	eStatusBar *status;

	void loadList(int mode, eString path);
	void viewList();
	void setStatus(int val);
	bool supportedFileType(eString filename);

	void listSelected(eListBoxEntryText *item);
	void listSelChanged(eListBoxEntryText *item);
	int eventHandler(const eWidgetEvent &);

	void timerHandler();
	void playerStart(int val);
	void showMenu();

	CURLcode sendGetRequest (const eString& url, eString& response);
public:
	eSCGui();
	~eSCGui();
};

class eSCGuiInfo: public eWindow
{
	eListBox<eListBoxEntryText> *list;
public:
	eSCGuiInfo();
};





