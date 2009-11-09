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

#include <gui/widget/hintbox.h>

#include <global.h>
#include <neutrino.h>

#define HINTBOX_MAX_HEIGHT 420


CHintBox::CHintBox(const neutrino_locale_t Caption, const char * const Text, const int Width, const char * const Icon)
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
		if (height > HINTBOX_MAX_HEIGHT)
			height -= fheight;

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
	entries_per_page = ((height - theight) / fheight) - 1;
	current_page = 0;

	unsigned int additional_width;

	if (entries_per_page < line.size())
		additional_width = 20 + 15;
	else
		additional_width = 20 +  0;

	if (Icon != NULL)
	{
		iconfile = Icon;
		additional_width += 30;
	}
	else
		iconfile = "";

	nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getRenderWidth(g_Locale->getText(caption), true); // UTF-8

	if (nw > width)
		width = nw;

	for (std::vector<char *>::const_iterator it = line.begin(); it != line.end(); it++)
	{
		nw = additional_width + g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getRenderWidth(*it, true); // UTF-8
		if (nw > width)
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
	if (window != NULL)
	{
		/*
		 * do not paint stuff twice:
		 * => thread safety needed by movieplayer.cpp:
		 *    one thread calls our paint method, the other one our hide method
		 * => no memory leaks
		 */
		return;
	}

	window = new CFBWindow((((g_settings.screen_EndX- g_settings.screen_StartX) - width ) >> 1) + g_settings.screen_StartX,
			       (((g_settings.screen_EndY- g_settings.screen_StartY) - height) >> 2) + g_settings.screen_StartY,
			       width + SHADOW_OFFSET,
			       height + SHADOW_OFFSET);
	refresh();
}

void CHintBox::refresh(void)
{
	if (window == NULL)
	{
		return;
	}

	int c_rad_mid = RADIUS_MID;

	window->paintBoxRel(SHADOW_OFFSET, SHADOW_OFFSET, width, (entries_per_page + 1) * fheight + theight, (CFBWindow::color_t)COL_INFOBAR_SHADOW_PLUS_0, c_rad_mid);
	window->paintBoxRel(0, 0, width, theight, (CFBWindow::color_t)COL_MENUHEAD_PLUS_0, c_rad_mid , CORNER_TOP);

	if (!iconfile.empty())
	{
		window->paintIcon(iconfile.c_str(), 8, 5);
		window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE], 40, theight, width - 40, g_Locale->getText(caption), (CFBWindow::color_t)COL_MENUHEAD, 0, true); // UTF-8
	}
	else
		window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE], 10, theight, width - 10, g_Locale->getText(caption), (CFBWindow::color_t)COL_MENUHEAD, 0, true); // UTF-8
		window->paintBoxRel(0, theight, width, (entries_per_page + 1) * fheight, (CFBWindow::color_t)COL_MENUCONTENT_PLUS_0, c_rad_mid , CORNER_BOTTOM);

	int count = entries_per_page;
	int ypos  = theight + (fheight >> 1);

	for (std::vector<char *>::const_iterator it = line.begin() + (entries_per_page * current_page); ((it != line.end()) && (count > 0)); it++, count--)
		window->RenderString(g_Font[SNeutrinoSettings::FONT_TYPE_MENU], 10, (ypos += fheight), width, *it, (CFBWindow::color_t)COL_MENUCONTENT, 0, true); // UTF-8

	if (entries_per_page < line.size())
	{
		ypos = theight + (fheight >> 1);
		window->paintBoxRel(width - 15, ypos, 15, (entries_per_page * fheight) + 16, COL_MENUCONTENT_PLUS_1);
		unsigned int marker_size = ((entries_per_page * fheight) + 16) / ((line.size() + entries_per_page - 1) / entries_per_page);
		window->paintBoxRel(width - 13, ypos + current_page * marker_size, 11, marker_size               , COL_MENUCONTENT_PLUS_3);
	}
}

bool CHintBox::has_scrollbar(void)
{
	return (entries_per_page < line.size());
}

void CHintBox::scroll_up(void)
{
	if (current_page > 0)
	{
		current_page--;
		refresh();
	}
}

void CHintBox::scroll_down(void)
{
	if ((entries_per_page * (current_page + 1)) <= line.size())
	{
		current_page++;
		refresh();
	}
}

void CHintBox::hide(void)
{
	if (window != NULL)
	{
		delete window;
		window = NULL;
	}
}

int ShowHintUTF(const neutrino_locale_t Caption, const char * const Text, const int Width, int timeout, const char * const Icon)
{
	neutrino_msg_t msg;
	neutrino_msg_data_t data;

 	CHintBox * hintBox = new CHintBox(Caption, Text, Width, Icon);
	hintBox->paint();

	if ( timeout == -1 )
		timeout = g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR];

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( timeout );

	int res = messages_return::none;

	while ( ! ( res & ( messages_return::cancel_info | messages_return::cancel_all ) ) )
	{
		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		neutrino_msg_t msg_repeatok = msg & ~CRCInput::RC_Repeat;

		if ((msg == CRCInput::RC_timeout) ||
		    (msg == CRCInput::RC_home   ) ||
		    (msg == CRCInput::RC_ok     ))
		{
				res = messages_return::cancel_info;
		}
		else if (hintBox->has_scrollbar() && (msg_repeatok == CRCInput::RC_up || msg_repeatok == CRCInput::RC_down))
		{
			if (msg_repeatok == CRCInput::RC_up)
				hintBox->scroll_up();
			else
				hintBox->scroll_down();
		}
		else
		{
			res = CNeutrinoApp::getInstance()->handleMsg(msg, data);
			if (res & messages_return::unhandled)
			{

				// raus hier und darüber behandeln...
				g_RCInput->postMsg(msg, data);
				res = messages_return::cancel_info;
			}
		}
	}

	hintBox->hide();
	delete hintBox;
	return 1;
}

int ShowLocalizedHint(const neutrino_locale_t Caption, const neutrino_locale_t Text, const int Width, int timeout, const char * const Icon)
{
	return ShowHintUTF(Caption, g_Locale->getText(Text),Width,timeout,Icon);
}
