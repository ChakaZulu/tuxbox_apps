/*
	nhttpd  -  DBoxII-Project

    Copyright (C) 2005 SnowHead

	$Id: lcdapi.cpp,v 1.2 2005/06/30 17:09:39 barf Exp $

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

// c++
#include <cstdlib>
#include <cstring>

// tuxbox
#include <driver/lcdd.h>
#include <global.h>
#include <neutrino.h>
#include <system/settings.h>

#include <lcddisplay/lcddisplay.h>

#include <dbox/fp.h>
#include <fcntl.h>
#include <unistd.h>

// nhttpd
#include "debug.h"
#include "lcdapi.h"

static char font_name[3][10]={"Micron","Micron","Pakenham"};

//-------------------------------------------------------------------------

void CLCDAPI::Clear(void)
{
	display.draw_fill_rect(-1, -1, 120, 64, CLCDDisplay::PIXEL_OFF); // clear lcd
}

void CLCDAPI::Update(void)
{
	display.update();
}

void CLCDAPI::LockDisplay(int plock)
{
	if (plock)
	{
		FILE *lfh=fopen("/tmp/lcd.locked","w");
		if(lfh)
		{
			fprintf(lfh,"lcdlock");
			fclose(lfh);
			usleep(100000L);
		}
	}
	else
	{
		remove("/tmp/lcd.locked");
	}
}

void CLCDAPI::DrawLine(int x1, int y1, int x2, int y2, int col)
{
	int color=(col==0)?(CLCDDisplay::PIXEL_OFF):((col==2)?(CLCDDisplay::PIXEL_INV):(CLCDDisplay::PIXEL_ON));
	display.draw_line(x1,y1,x2,y2,color);
}

void CLCDAPI::DrawRect(int x1, int y1, int x2, int y2, int coll, int colf)
{
	int colorl=(coll==0)?(CLCDDisplay::PIXEL_OFF):((coll==2)?(CLCDDisplay::PIXEL_INV):(CLCDDisplay::PIXEL_ON));
	int colorf=(colf==0)?(CLCDDisplay::PIXEL_OFF):((colf==2)?(CLCDDisplay::PIXEL_INV):(CLCDDisplay::PIXEL_ON));
	display.draw_rectangle(x1,y1,x2,y2,colorl,colorf);
}

void CLCDAPI::DrawText(int px, int py, int psize, int pcolor, int pfont, char *pmsg)
{
	int color=(pcolor==0)?(CLCDDisplay::PIXEL_OFF):((pcolor==2)?(CLCDDisplay::PIXEL_INV):(CLCDDisplay::PIXEL_ON));
	if(!(font = fontRenderer->getFont(font_name[pfont], style_name[pfont], psize)))
	{
		printf("[nhttpd] Kein Font gefunden.\n");
		return;
	}
	font->RenderString(px, py, 130, pmsg, color, 0, true); // UTF-8
}

bool CLCDAPI::ShowPng(char *filename)
{
	return display.load_png(filename);
}

void CLCDAPI::ShowRaw(int xpos, int ypos, int xsize, int ysize, char *ascreen)
{
	int sbyte,dbit,dxpos,dypos,wasinc=0,gotval;
	char *sptr=ascreen;
	raw_display_t rscreen;
	
	display.dump_screen(&rscreen);
	gotval=sscanf(sptr,"%d",&sbyte);
	for(dypos=ypos; gotval && (dypos<(ypos+ysize)); dypos++)
	{
		dbit=7;
		for(dxpos=xpos; gotval && (dxpos<(xpos+xsize)); dxpos++)
		{
			wasinc=0;
			if((dypos<LCD_ROWS*8) && (dxpos<LCD_COLS))
			{
				rscreen[dypos][dxpos]=(sbyte & (1<<dbit))?1:0;
			}
			if(--dbit<0)
			{
				dbit=7;
				if((sptr=strchr(sptr,','))!=NULL)
				{
					gotval=sscanf(++sptr,"%d",&sbyte);
				}
				else
				{
					gotval=0;
				}
				wasinc=1;
			}
		}
		if(!wasinc)
		{
			if((sptr=strchr(sptr,','))!=NULL)
			{
				gotval=sscanf(++sptr,"%d",&sbyte);
			}
			else
			{
				gotval=0;
			}
		}
	}

	display.load_screen(&rscreen);
}
	

//-------------------------------------------------------------------------
// Konstruktor und destruktor
//-------------------------------------------------------------------------

CLCDAPI::CLCDAPI(CWebDbox *webdbox)
{
//	int i;
	
	Parent=webdbox;

//	display = new CLCDDisplay();
	fontRenderer = new LcdFontRenderClass(&display);
	style_name[0] = fontRenderer->AddFont("/share/fonts/micron.ttf");
	style_name[1] = fontRenderer->AddFont("/share/fonts/micron_bold.ttf");
	style_name[2] = fontRenderer->AddFont("/share/fonts/pakenham.ttf");
	fontRenderer->InitFontCache();
/*	for(i=0; i<3; i++)
	{
		if(font=fontRenderer->getFont(font_name[i], style_name[i], 14))
		{
			font->RenderString(10, 10, 30, "X", CLCDDisplay::PIXEL_OFF, 0, true);
		}
	}
*/
}
//-------------------------------------------------------------------------

CLCDAPI::~CLCDAPI(void)
{
	if(fontRenderer)
	{
		delete fontRenderer;
	}
/*	if(display)
	{
		delete display;
	}
*/
}
