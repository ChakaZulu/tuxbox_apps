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


#include <gui/widget/hintbox.h>

#include <global.h>
#include <neutrino.h>

#define borderwidth 4


CHintBox::CHintBox(const char * const Caption, const char * const Text, const int Width, const char * const Icon)
{
	char * begin;
	char * pos;
	int    nw;

	message = strdup(Text);

	width   = Width;

	theight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	fheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height  = theight + fheight;

	caption = Caption;

	begin   = message;

	while (true)
	{
		height += fheight;

		line.push_back(begin);
		pos = strchr(begin, '\n');
		if (pos != NULL)
		{
			*pos = 0;
			begin = pos + 1;
		}
		else
			break;
	}

	nw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(g_Locale->getText(caption), true) + 20; // UTF-8

	if (Icon != NULL)
	{
		iconfile = Icon;
		nw += 30;
	}
	else
		iconfile = "";

	if (nw > width)
		width = nw;

	for (std::vector<char *>::const_iterator it = line.begin(); it != line.end(); it++)
	{
		nw = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(*it, true) + 20; // UTF-8
		if (nw > width )
			width = nw;
	}
	window = NULL;
}

CHintBox::~CHintBox(void)
{
	if (window != NULL)
	{
		delete window;
		window = NULL;
	}
	free(message);
}

void CHintBox::paint(void)
{
	int ypos;

	window = new CFBWindow((((g_settings.screen_EndX- g_settings.screen_StartX) - width ) >> 1) + g_settings.screen_StartX,
			       (((g_settings.screen_EndY- g_settings.screen_StartY) - height) >> 2) + g_settings.screen_StartY,
			       width + borderwidth,
			       height + borderwidth);

	if (window == NULL)
	{
		return; /* out of memory */
	}

	window->paintBoxRel(borderwidth, height, width, borderwidth, COL_BACKGROUND);
	window->paintBoxRel(width, borderwidth, borderwidth, height - borderwidth, COL_BACKGROUND);

	window->paintBoxRel(0, 0, width, theight, (CFBWindow::color_t)COL_MENUHEAD);
	if (!iconfile.empty())
	{
		window->paintIcon(iconfile.c_str(), 8, 5);
		window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE], 40, theight, width - 40, g_Locale->getText(caption), (CFBWindow::color_t)COL_MENUHEAD, 0, true); // UTF-8
	}
	else
		window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE], 10, theight, width - 10, g_Locale->getText(caption), (CFBWindow::color_t)COL_MENUHEAD, 0, true); // UTF-8

	window->paintBoxRel(0, theight, width, height - theight, (CFBWindow::color_t)COL_MENUCONTENT);

	ypos = theight + (fheight >> 1);

	for (std::vector<char *>::const_iterator it = line.begin(); it != line.end(); it++)
		window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_MENU], 10, (ypos += fheight), width, *it, (CFBWindow::color_t)COL_MENUCONTENT, 0, true); // UTF-8
}

void CHintBox::hide(void)
{
	if (window != NULL)
	{
		delete window;
		window = NULL;
	}
}

int ShowHintUTF(const char * const Caption, const char * const Text, const int Width, int timeout)
{
 	CHintBox * hintBox = new CHintBox(Caption, Text, Width);
	hintBox->paint();

	if ( timeout == -1 )
		timeout = g_settings.timing_infobar ;

	uint msg; uint data;
	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	int res = messages_return::none;

	while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
	{
    	g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if((msg==CRCInput::RC_timeout) || (msg==CRCInput::RC_home) || (msg==CRCInput::RC_ok))
		{
				res = messages_return::cancel_info;
		}
		else
		{
	        res = CNeutrinoApp::getInstance()->handleMsg( msg, data );
			if ( res & messages_return::unhandled )
			{

				// raus hier und darüber behandeln...
				g_RCInput->postMsg(  msg, data );
				res = messages_return::cancel_info;
			}
		}
	}

	hintBox->hide();
	delete hintBox;
	return 1;
}
