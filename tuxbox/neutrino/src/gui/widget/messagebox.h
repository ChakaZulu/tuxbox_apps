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


#ifndef __messagebox__
#define __messagebox__

#include <driver/fb_window.h>

#include "menue.h"

#include <string>
#include <vector>


class CMessageBoxNotifier
{
  public:
	virtual void onYes( ) = NULL;
	virtual void onNo( ) = NULL;
};



class CMessageBox
{
 private:

	CFBWindow *              window;

	int                      width, height;

	int                      fheight;
	int                      theight;
	
	std::string              caption;
	std::vector<std::string> text;
	std::string              iconfile;
	CMessageBoxNotifier*     notifier;

	int                      selected;
	int                      showbuttons;
	bool                     utf8;        // utf8_encoded: Caption & Text
	
	void paintHead();
	void paintButtons();
	
	void yes();
	void no();
	void cancel();

 public:
	enum result_
		{
			mbrYes,
			mbrNo,
			mbrCancel,
			mbrBack
		} result;
	
	enum buttons_
		{
			mbYes= 0x01,
			mbNo = 0x02,
			mbCancel = 0x04,
			mbAll = 0x07,
			mbBack = 0x08
		} buttons;
	
	// utf8_encoded: Caption & Text
	CMessageBox(const std::string Caption, std::string Text, CMessageBoxNotifier* Notifier, const std::string Icon = "", int Width = 500, uint Default= mbrYes, uint ShowButtons= mbAll, const bool utf8_encoded = false);
	~CMessageBox(void);

	int exec(int timeout = -1);
	
};

// utf8_encoded: Caption & Text
int ShowMsg(const std::string Caption, std::string Text, uint Default, uint ShowButtons, const std::string Icon = "", int Width = 450, int timeout = -1, const bool utf8_encoded = false);


#endif
