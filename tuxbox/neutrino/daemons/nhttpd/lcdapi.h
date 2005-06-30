/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2005 SnowHead

        $Id: lcdapi.h,v 1.2 2005/06/30 17:09:39 barf Exp $

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

#ifndef __nhttpd_lcdapi_h__
#define __nhttpd_lcdapi_h__

// c++
#include <map>
#include <string>

// tuxbox
#include <driver/lcdd.h>
#include <global.h>
#include <neutrino.h>
#include <system/settings.h>

#include <lcddisplay/lcddisplay.h>
#include <lcddisplay/fontrenderer.h>

#include <dbox/fp.h>
#include <fcntl.h>
#include <unistd.h>

// nhttpd
#include "helper.h"
#include "request.h"

class CWebserver;
class CWebserverRequest;
class CControlAPI;
class CLCDDisplay;
class LcdFontRenderClass;

#include "controlapi.h"

//-------------------------------------------------------------------------


class CLCDAPI
{
	CWebDbox			*Parent;
	CLCDDisplay			display;
	LcdFontRenderClass	*fontRenderer;
	LcdFont				*font;
	const char 			*style_name[2];
public:
	CLCDAPI(CWebDbox *webdbox);
	~CLCDAPI(void);
	void LockDisplay(int lock);
	void DrawText(int px, int py, int psize, int pcolor, int pfont, char *pmsg);
	void DrawLine(int x1, int y1, int x2, int y2, int col);
	void DrawRect(int x1, int y1, int x2, int y2, int coll, int colf);
	bool ShowPng(char *filename);
	void ShowRaw(int xpos, int ypos, int xsize, int ysize, char *screen);
	void Update(void);
	void Clear(void);
};

#endif /* __nhttpd_lcdapi_h__ */
