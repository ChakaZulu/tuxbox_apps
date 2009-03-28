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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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

#define ALPHA_SETUP_ICON_DESELECTED      "volumeslider2.raw"
#define ALPHA_SETUP_ICON_ALPHA1_SELECTED "volumeslider2red.raw"
#define ALPHA_SETUP_ICON_ALPHA2_SELECTED "volumeslider2green.raw"


CAlphaSetup::CAlphaSetup(const neutrino_locale_t Name, unsigned char* Alpha1, unsigned char* Alpha2, CChangeObserver* Observer)
{
	frameBuffer = CFrameBuffer::getInstance();
	observer = Observer;
	name = Name;
	width = 360;
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+ mheight*3;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;

	alpha1 = Alpha1;
	alpha2 = Alpha2;
	frameBuffer->setBlendLevel(*alpha1, *alpha2);
}

int CAlphaSetup::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;
	if (parent)
	{
		parent->hide();
	}
	unsigned char alpha1_alt= *alpha1;
	unsigned char alpha2_alt= *alpha2;

	frameBuffer->setBlendLevel(*alpha1, *alpha2);
	paint();

	int selected = 0;
	int max = 1;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

		if ( msg <= CRCInput::RC_MaxRC )
		{
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_MENU]);
		}

		switch (msg & ~CRCInput::RC_Repeat)
		{
		case CRCInput::RC_down:
		{
			if(selected<max)
			{
				paintSlider(x + 10, y + hheight          , alpha1, LOCALE_GTXALPHA_ALPHA1, ALPHA_SETUP_ICON_DESELECTED     , false);
				paintSlider(x + 10, y + hheight + mheight, alpha2, LOCALE_GTXALPHA_ALPHA2, ALPHA_SETUP_ICON_DESELECTED     , false);

				selected++;

				switch (selected)
				{
				case 0:
					paintSlider(x + 10, y + hheight          , alpha1, LOCALE_GTXALPHA_ALPHA1, ALPHA_SETUP_ICON_ALPHA1_SELECTED, true );
					break;
				case 1:
					paintSlider(x + 10, y + hheight + mheight, alpha2, LOCALE_GTXALPHA_ALPHA2, ALPHA_SETUP_ICON_ALPHA2_SELECTED, true );
					break;
				}
			}
			break;
    	        }
		case CRCInput::RC_up:
		{
			if (selected > 0)
			{
				paintSlider(x + 10, y + hheight          , alpha1, LOCALE_GTXALPHA_ALPHA1, ALPHA_SETUP_ICON_DESELECTED     , false);
				paintSlider(x + 10, y + hheight + mheight, alpha2, LOCALE_GTXALPHA_ALPHA2, ALPHA_SETUP_ICON_DESELECTED     , false);
				
				selected--;

				switch (selected)
				{
				case 0:
					paintSlider(x + 10, y + hheight          , alpha1, LOCALE_GTXALPHA_ALPHA1, ALPHA_SETUP_ICON_ALPHA1_SELECTED, true );
					break;
				case 1:
					paintSlider(x + 10, y + hheight + mheight, alpha2, LOCALE_GTXALPHA_ALPHA2, ALPHA_SETUP_ICON_ALPHA2_SELECTED, true );
					break;
				}
			}
			break;
		}
		case CRCInput::RC_right:
		{
			switch (selected)
			{
			case 0:
				if (*alpha1<8)
				{
					*alpha1+=1;
					paintSlider(x + 10, y + hheight          , alpha1, LOCALE_GTXALPHA_ALPHA1, ALPHA_SETUP_ICON_ALPHA1_SELECTED, true );
					frameBuffer->setBlendLevel(*alpha1, *alpha2);
				}
				break;
			case 1:
				if (*alpha2<8)
				{
					*alpha2+=1;
					paintSlider(x + 10, y + hheight + mheight, alpha2, LOCALE_GTXALPHA_ALPHA2, ALPHA_SETUP_ICON_ALPHA2_SELECTED, true );
					frameBuffer->setBlendLevel(*alpha1, *alpha2);
				}
				break;
			}
			break;
		}
		case CRCInput::RC_left:
		{
			switch (selected)
			{
			case 0:
				if (*alpha1>=1)
				{
					*alpha1-=1;
					paintSlider(x + 10, y + hheight          , alpha1, LOCALE_GTXALPHA_ALPHA1, ALPHA_SETUP_ICON_ALPHA1_SELECTED, true );
					frameBuffer->setBlendLevel(*alpha1, *alpha2);
				}
				break;
			case 1:
				if (*alpha2>=1)
				{
					*alpha2-=1;
					paintSlider(x + 10, y + hheight + mheight, alpha2, LOCALE_GTXALPHA_ALPHA2, ALPHA_SETUP_ICON_ALPHA2_SELECTED, true );
					frameBuffer->setBlendLevel(*alpha1, *alpha2);
				}
				break;
			}
			break;
		}
		case CRCInput::RC_home:
				if ((*alpha1 != alpha1_alt) || (*alpha2 != alpha2_alt))
				{
					if (ShowLocalizedMessage(name, LOCALE_MESSAGEBOX_DISCARD, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel)
					{
						break;
					}
				}

				// sonst abbruch...
				*alpha1 = alpha1_alt;
				*alpha2 = alpha2_alt;
				loop = false;
				break;
			case CRCInput::RC_ok:
				if (msg != CRCInput::RC_ok) // ignore repeat
					break;
				/* else fallthrough */
			case CRCInput::RC_timeout:
				loop = false;
				break;

			default:
				if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
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
	int c_rad_mid = RADIUS_MID;
	frameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD_PLUS_0, c_rad_mid, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+hheight, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x, y + hheight, width, height - hheight, COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);

	paintSlider(x + 10, y + hheight          , alpha1, LOCALE_GTXALPHA_ALPHA1, ALPHA_SETUP_ICON_ALPHA1_SELECTED, true );
	paintSlider(x + 10, y + hheight + mheight, alpha2, LOCALE_GTXALPHA_ALPHA2, ALPHA_SETUP_ICON_DESELECTED     , false);
}

void CAlphaSetup::paintSlider(const int x, const int y, const unsigned char * const spos, const neutrino_locale_t text, const char * const iconname, const bool /*selected*/) // UTF-8
{
	int startx = 170;
	char wert[5];

	if (!spos)
		return;
	int sspos = (*spos)*100/8;

	frameBuffer->paintBoxRel(x + startx, y, 120, mheight, COL_MENUCONTENT_PLUS_0);

	frameBuffer->paintIcon("volumebody.raw", x + startx            , y + 2 + mheight / 4);
	frameBuffer->paintIcon(iconname        , x + startx + 3 + sspos, y     + mheight / 4);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x,y+mheight, width, g_Locale->getText(text), COL_MENUCONTENT, 0, true); // UTF-8

	sprintf(wert, "%3d", (*spos)); // UTF-8 encoded
	frameBuffer->paintBoxRel(x + startx + 120 + 10, y, 50, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x + startx + 120 + 10, y+mheight, width, wert, COL_MENUCONTENT, 0, true); // UTF-8
}
