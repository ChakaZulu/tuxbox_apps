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

#include <gui/alphasetup.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>

#include <gui/widget/messagebox.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>

#include <dbox/fb.h>

#include <global.h>
#include <neutrino.h>


CAlphaSetup::CAlphaSetup(const char * const Name, unsigned char* Alpha1, unsigned char* Alpha2, CChangeObserver* Observer)
{
	frameBuffer = CFrameBuffer::getInstance();
	observer = Observer;
	name = Name;
	width = 360;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight+ mheight*3;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;

	alpha1 = Alpha1;
	alpha2 = Alpha2;
	setAlpha();
}

void CAlphaSetup::setAlpha()
{
	int fd, c;

	c=(*alpha2)<<8 | (*alpha1);

	if ((fd = open("/dev/fb/0",O_RDWR)) <= 0)
	{
		perror("open");
		return;
	}

	if (ioctl(fd,AVIA_GT_GV_SET_BLEV, c) < 0)
	{
		perror("AVIA_GT_GV_SET_BLEV:");
		return;
	}
	close(fd);
}

int CAlphaSetup::exec(CMenuTarget* parent, std::string)
{
	int res = menu_return::RETURN_REPAINT;
	if (parent)
	{
		parent->hide();
	}
	unsigned char alpha1_alt= *alpha1;
	unsigned char alpha2_alt= *alpha2;

	setAlpha();
	paint();

	int selected = 0;
	int max = 1;

	uint msg; uint data;
	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd, true );

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_menu );

		switch ( msg )
		{
			case CRCInput::RC_down:
				{
					if(selected<max)
					{
						paintSlider(x+10, y+ hheight, alpha1, g_Locale->getText("gtxalpha.alpha1"),"red", false);
						paintSlider(x+10, y+ hheight+ mheight, alpha2, g_Locale->getText("gtxalpha.alpha2"),"green", false);
						selected++;
						switch (selected)
						{
							case 0:
								paintSlider(x+ 10, y+ hheight, alpha1, g_Locale->getText("gtxalpha.alpha1"),"red", true);
								break;
							case 1:
								paintSlider(x+ 10, y+ hheight+ mheight, alpha2, g_Locale->getText("gtxalpha.alpha2"),"green", true);
								break;
						}
					}
					break;
    	        }
			case CRCInput::RC_up:
				if(selected>0)
				{
					paintSlider(x+10, y+hheight, alpha1, g_Locale->getText("gtxalpha.alpha1"),"red", false);
					paintSlider(x+10, y+hheight+mheight, alpha2, g_Locale->getText("gtxalpha.alpha2"),"green", false);
					selected--;
					switch (selected)
					{
						case 0:
							paintSlider(x+10, y+hheight, alpha1, g_Locale->getText("gtxalpha.alpha1"),"red", true);
							break;
						case 1:
							paintSlider(x+10, y+hheight+mheight, alpha2, g_Locale->getText("gtxalpha.alpha2"),"green", true);
							break;
					}
				}
				break;

			case CRCInput::RC_right:
				switch (selected)
				{
					case 0:
						if (*alpha1<8)
						{
							*alpha1+=1;
							paintSlider(x+10, y+hheight, alpha1,g_Locale->getText("gtxalpha.alpha1"),"red", true);
							setAlpha();
						}
						break;
					case 1:
						if (*alpha2<8)
						{
							*alpha2+=1;
							paintSlider(x+10, y+hheight+mheight, alpha2,g_Locale->getText("gtxalpha.alpha2"),"green", true);
							setAlpha();
						}
						break;
				}
				break;

			case CRCInput::RC_left:
				switch (selected)
				{
					case 0:
						if (*alpha1>=1)
						{
							*alpha1-=1;
							paintSlider(x+10, y+hheight, alpha1,g_Locale->getText("gtxalpha.alpha1"),"red", true);
							setAlpha();
						}
						break;
					case 1:
						if (*alpha2>=1)
						{
							*alpha2-=1;
							paintSlider(x+10, y+hheight+mheight, alpha2,g_Locale->getText("gtxalpha.alpha2"),"green", true);
							setAlpha();
						}
						break;
				}
				break;

			case CRCInput::RC_home:
				if ((*alpha1 != alpha1_alt) || (*alpha2 != alpha2_alt))
				{
					if (ShowMsgUTF(name.c_str(), g_Locale->getText("messagebox.discard"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel) // UTF-8
					{
						break;
					}
				}

				// sonst abbruch...
				*alpha1 = alpha1_alt;
				*alpha2 = alpha2_alt;

			case CRCInput::RC_timeout:
			case CRCInput::RC_ok:
				loop = false;
				break;

			default:
				if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
				{
					loop = false;
					res = menu_return::RETURN_EXIT_ALL;
				}
		}
	}

	hide();

	if(observer)
		observer->changeNotify(name, NULL);

	return res;
}

void CAlphaSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CAlphaSetup::paint()
{
	frameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+10,y+hheight, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x,y+hheight, width,height-hheight, COL_MENUCONTENT);

	paintSlider(x+10, y+hheight, alpha1,g_Locale->getText("gtxalpha.alpha1"),"red", true);
	paintSlider(x+10, y+hheight+mheight, alpha2,g_Locale->getText("gtxalpha.alpha2"),"green", false);
}

void CAlphaSetup::paintSlider(const int x, const int y, const unsigned char * const spos, const std::string text, const std::string iconname, const bool selected) // UTF-8
{
	if (!spos)
		return;
	int sspos = (*spos)*100/8;
	frameBuffer->paintBoxRel(x+70,y,120,mheight, COL_MENUCONTENT);
	frameBuffer->paintIcon("volumebody.raw",x+70,y+2+mheight/4);
	std::string iconfile = "volumeslider2";
	if (selected)
		iconfile += iconname;
	iconfile +=".raw";
	frameBuffer->paintIcon(iconfile,x+73+sspos,y+mheight/4);

	g_Fonts->menu->RenderString(x,y+mheight, width, text.c_str(), COL_MENUCONTENT, 0, true); // UTF-8
}
