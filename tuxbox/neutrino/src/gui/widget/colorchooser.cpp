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

#include <gui/widget/colorchooser.h>

#include <global.h>
#include <neutrino.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/widget/messagebox.h>

#define VALUE_R     0
#define VALUE_G     1
#define VALUE_B     2
#define VALUE_ALPHA 3

static const std::string iconnames[4] = {
	"red",
	"green",
	"blue",
	"alpha"
};

CColorChooser::CColorChooser(const char * const Name, unsigned char *R, unsigned char *G, unsigned char *B, unsigned char* Alpha, CChangeObserver* Observer) // UTF-8
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	observer = Observer;
	name = Name;
	width = 360;
	height = hheight+ mheight* 4;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;

	value[VALUE_R]     = R;
	value[VALUE_G]     = G;
	value[VALUE_B]     = B;
	value[VALUE_ALPHA] = Alpha;
}

void CColorChooser::setColor()
{
	int color = convertSetupColor2RGB(*(value[VALUE_R]), *(value[VALUE_G]), *(value[VALUE_B]));
	int tAlpha = (value[VALUE_ALPHA]) ? (convertSetupAlpha2Alpha(*(value[VALUE_ALPHA]))) : 0;
	frameBuffer->paletteSetColor(254, color, tAlpha);
	frameBuffer->paletteSet();
	/*
	char colorstr[30];
	sprintf((char*)&colorstr, "%02x.%02x.%02x", *r, *g, *b);
	frameBuffer->paintBoxRel(x+218,y+107, 80, 20, COL_MENUCONTENT);
	fonts->epg_date->RenderString(x+218,y+120, 80, colorstr, COL_MENUCONTENT);
	*/
}

int CColorChooser::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;
	if (parent)
	{
		parent->hide();
	}
	unsigned char r_alt= *value[VALUE_R];
	unsigned char g_alt= *value[VALUE_G];
	unsigned char b_alt= *value[VALUE_B];
	unsigned char a_alt = (value[VALUE_ALPHA]) ? (*(value[VALUE_ALPHA])) : 0;

	const char * names[4] = {
		g_Locale->getText("colorchooser.red"),
		g_Locale->getText("colorchooser.green"),
		g_Locale->getText("colorchooser.blue"),
		g_Locale->getText("colorchooser.alpha")
	};

	setColor();
	paint();
	setColor();

	int selected = 0;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_menu );

	bool loop=true;
	while (loop)
	{
		g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd, true);

		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = CRCInput::calcTimeoutEnd( g_settings.timing_menu );

		switch ( msg )
		{
		case CRCInput::RC_down:
		{
			if (selected < ((value[VALUE_ALPHA]) ? 3 : 2))
			{
				paintSlider(x + 10, y + hheight + mheight * selected, value[selected], names[selected], iconnames[selected], false);
				selected++;
				paintSlider(x + 10, y + hheight + mheight * selected, value[selected], names[selected], iconnames[selected], true);
			}
			break;
			
		}
		case CRCInput::RC_up:
		{
			if (selected > 0)
			{
				paintSlider(x + 10, y + hheight + mheight * selected, value[selected], names[selected], iconnames[selected], false);
				selected--;
				paintSlider(x + 10, y + hheight + mheight * selected, value[selected], names[selected], iconnames[selected], true);
			}
			break;
		}
		case CRCInput::RC_right:
		{
			if ((*value[selected]) < 100)
			{
				if ((*value[selected]) < 95)
					(*value[selected]) += 5;
				else
					(*value[selected]) = 100;
				
				paintSlider(x + 10, y + hheight + mheight * selected, value[selected], names[selected], iconnames[selected], true);
				setColor();
			}
			break;
		}
		case CRCInput::RC_left:
		{
			if ((*value[selected]) > 0)
			{
				if ((*value[selected]) > 5)
					(*value[selected]) -= 5;
				else
					(*value[selected]) = 0;
				
				paintSlider(x + 10, y + hheight + mheight * selected, value[selected], names[selected], iconnames[selected], true);
				setColor();
			}
			break;
		}
		case CRCInput::RC_home:
			if (((*value[VALUE_R] != r_alt) || (*value[VALUE_G] != g_alt) || (*value[VALUE_B] != b_alt) || ((value[VALUE_ALPHA]) && (*(value[VALUE_ALPHA]) != a_alt))) &&
			    (ShowMsgUTF(name.c_str(), g_Locale->getText("messagebox.discard"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel)) // UTF-8
				break;

				// sonst abbruch...
				*value[VALUE_R] = r_alt;
				*value[VALUE_G] = g_alt;
				*value[VALUE_B] = b_alt;
				if (value[VALUE_ALPHA])
					*value[VALUE_ALPHA] = a_alt;

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

void CColorChooser::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CColorChooser::paint()
{
	frameBuffer->paintBoxRel(x,y, width,hheight, COL_MENUHEAD);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+hheight, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x,y+hheight, width,height-hheight, COL_MENUCONTENT);

	const char * names[4] = {
		g_Locale->getText("colorchooser.red"),
		g_Locale->getText("colorchooser.green"),
		g_Locale->getText("colorchooser.blue"),
		g_Locale->getText("colorchooser.alpha")
	};
	for (int i = 0; i < 4; i++)
		paintSlider(x + 10, y + hheight + mheight * i, value[i], names[i], iconnames[i], (i == 0));

	//color preview
	frameBuffer->paintBoxRel(x+220,y+hheight+5,    mheight*4,   mheight*4-10,   COL_MENUHEAD);
	frameBuffer->paintBoxRel(x+222,y+hheight+2+5,  mheight*4-4 ,mheight*4-4-10, 254);
}

void CColorChooser::paintSlider(int x, int y, unsigned char *spos, const char * const text, const std::string & iconname, const bool selected)
{
	if (!spos)
		return;
	frameBuffer->paintBoxRel(x+70,y,120,mheight, COL_MENUCONTENT);
	frameBuffer->paintIcon("volumebody.raw",x+70,y+2+mheight/4);
	std::string iconfile = "volumeslider2";
	if (selected)
		iconfile += iconname;
	iconfile +=".raw";
	frameBuffer->paintIcon(iconfile,x+73+(*spos),y+mheight/4);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x,y+mheight, width, text, COL_MENUCONTENT, 0, true); // UTF-8
}
