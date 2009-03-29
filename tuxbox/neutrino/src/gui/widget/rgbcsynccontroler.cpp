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

#include <gui/widget/rgbcsynccontroler.h>

#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <gui/color.h>
#include <gui/widget/messagebox.h>

#include <global.h>
#include <neutrino.h>

#include <math.h>

/* in percent; will be divided through 100 */
#define CSYNCFACTOR 31 


CRGBCSyncControler::CRGBCSyncControler(const neutrino_locale_t Name, unsigned char* Csync, CChangeObserver* Observer)
{
	frameBuffer = CFrameBuffer::getInstance();
	hheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight     = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight() + 6;
	observer = Observer;
	name = Name;
	width = 390;
	height = hheight+ mheight* 1+ +mheight/2;
	x=((720-width) >> 1) -20;
	y=(576-height)>>1;
	csync=Csync;
}

void CRGBCSyncControler::setCSync()
{
//	printf("contrast: %d brightness: %d brightness standby: %d\n", contrast, brightness, brightnessstandby);
	g_Controld->setRGBCsync(*csync);
}

int CRGBCSyncControler::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = menu_return::RETURN_REPAINT;
	unsigned char csync_alt;

	if (parent)
	{
		parent->hide();
	}
	csync_alt = *csync;

	setCSync();
	paint();

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
			case CRCInput::RC_right:
				if (*csync < 31)
				{
					(*csync)++;
					paintSlider(x+10, y+hheight, *csync, CSYNCFACTOR, LOCALE_VIDEOMENU_CSYNC, true);
					setCSync();
				}
				break;

			case CRCInput::RC_left:
				if (*csync > 0)
			   {
					(*csync)--;
					paintSlider(x+10, y+hheight, *csync, CSYNCFACTOR, LOCALE_VIDEOMENU_CSYNC, true);
					setCSync();
				}
				break;

			case CRCInput::RC_home:
				if ( ( (*csync != csync_alt) ) &&
				     (ShowLocalizedMessage(name, LOCALE_MESSAGEBOX_DISCARD, CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel) == CMessageBox::mbrCancel))
					break;

				// sonst abbruch...
				*csync = csync_alt;
				setCSync();
				loop = false;
				break;

			case CRCInput::RC_ok:
				/* ignore repeat... */
				if (msg == CRCInput::RC_ok)
					loop = false;
				break;

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

void CRGBCSyncControler::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CRGBCSyncControler::paint()
{
	int c_rad_mid = RADIUS_MID;
	
	frameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD_PLUS_0, c_rad_mid, CORNER_TOP);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+10,y+hheight, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8
	frameBuffer->paintBoxRel(x, y + hheight, width, height - hheight, COL_MENUCONTENT_PLUS_0, c_rad_mid, CORNER_BOTTOM);

	paintSlider(x+10, y+hheight, *csync, CSYNCFACTOR, LOCALE_VIDEOMENU_CSYNC, true);

//	frameBuffer->paintHLineRel(x+10, width-20, y+hheight+mheight*3+mheight/4, COL_MENUCONTENT_PLUS_3);
}

void CRGBCSyncControler::paintSlider(int _x, int _y, unsigned int spos, int factor, const neutrino_locale_t text, bool selected)
{
	int startx = 200;
	char wert[5];

	frameBuffer->paintBoxRel(_x + startx, _y, 120, mheight, COL_MENUCONTENT_PLUS_0);
	frameBuffer->paintIcon("volumebody.raw", _x + startx, _y+2+mheight/4);
	frameBuffer->paintIcon(selected ? "volumeslider2blue.raw" : "volumeslider2.raw", (int)(_x + (startx+3)+(100*spos / factor)), _y+mheight/4);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x, _y+mheight, width, g_Locale->getText(text), COL_MENUCONTENT, 0, true); // UTF-8
	sprintf(wert, "%3d", spos); // UTF-8 encoded
	frameBuffer->paintBoxRel(_x + startx + 120 + 10, _y, 50, mheight, COL_MENUCONTENT_PLUS_0);
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(_x + startx + 120 + 10, _y+mheight, width, wert, COL_MENUCONTENT, 0, true); // UTF-8
}
