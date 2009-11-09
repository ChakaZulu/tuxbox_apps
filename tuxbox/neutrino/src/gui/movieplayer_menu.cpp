/*
	$Id: movieplayer_menu.cpp,v 1.1 2009/11/09 13:05:09 dbt Exp $

	Movieplayer menue - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

	$Log: movieplayer_menu.cpp,v $
	Revision 1.1  2009/11/09 13:05:09  dbt
	menue cleanup:
	parentallock, movieplayer_menue and network-setup for it's own modules
	
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "movieplayer_menu.h"
#include "movieplayer_setup.h"

#ifdef ENABLE_GUI_MOUNT
#include "nfs.h"
#endif

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <driver/screen_max.h>

#include <system/debug.h>



CMoviePlayerMenue::CMoviePlayerMenue()
{
	frameBuffer = CFrameBuffer::getInstance();

	moviePlayerGui = NULL;
#ifdef ENABLE_MOVIEBROWSER
	movieBrowser = NULL;
#endif

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

}

CMoviePlayerMenue::~CMoviePlayerMenue()
{
	delete moviePlayerGui;
#ifdef ENABLE_MOVIEBROWSER
	delete movieBrowser;
#endif
}

int CMoviePlayerMenue::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init movieplayer menu\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	showMoviePlayerMenue();

	
	return res;
}

void CMoviePlayerMenue::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

void CMoviePlayerMenue::showMoviePlayerMenue()
{
	moviePlayerGui = new CMoviePlayerGui();
#ifdef ENABLE_MOVIEBROWSER
	movieBrowser = new CMovieBrowser();
#endif
	//init
	CMenuWidget * mpmenue = new CMenuWidget(LOCALE_MAINMENU_MOVIEPLAYER, NEUTRINO_ICON_EPGINFO, width);

	mpmenue->addItem(GenericMenuSeparator);
	mpmenue->addItem(GenericMenuBack);
	mpmenue->addItem(GenericMenuSeparatorLine);

	//ts playback 
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, this->moviePlayerGui, "tsplayback", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	//ts playback pin 
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK_PC, true, NULL, this->moviePlayerGui, "tsplayback_pc", CRCInput::RC_1));

	neutrino_msg_t rc_msg;
#ifdef ENABLE_MOVIEBROWSER
#ifndef ENABLE_MOVIEPLAYER2
	//moviebrowser init via movieplayer 1
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, this->moviePlayerGui, "tsmoviebrowser", CRCInput::RC_2));
#else
	//moviebrowser init
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, this->movieBrowser, "run", CRCInput::RC_2));
#endif /* ENABLE_MOVIEPLAYER2 */
	rc_msg = CRCInput::RC_3;
#else
	rc_msg = CRCInput::RC_2;
#endif /* ENABLE_MOVIEBROWSER */

	//bookmark
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_BOOKMARK, true, NULL, this->moviePlayerGui, "bookmarkplayback", rc_msg));

	mpmenue->addItem(GenericMenuSeparatorLine);

	//vlc file play
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_FILEPLAYBACK, true, NULL, this->moviePlayerGui, "fileplayback", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	//vlc dvd play
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_DVDPLAYBACK, true, NULL, this->moviePlayerGui, "dvdplayback", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	//vlc vcd play
	mpmenue->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_VCDPLAYBACK, true, NULL, this->moviePlayerGui, "vcdplayback", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

	mpmenue->addItem(GenericMenuSeparatorLine);

	//help
	mpmenue->addItem(new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, new CMoviePlayerSetup(), NULL, CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP_SMALL));

#ifdef ENABLE_GUI_MOUNT
	//neutrino mount
	mpmenue->addItem(new CMenuForwarder(LOCALE_NETWORKMENU_MOUNT, true, NULL, new CNFSSmallMenu(), NULL, CRCInput::RC_setup, NEUTRINO_ICON_BUTTON_DBOX_SMALL));
#endif

	mpmenue->exec(NULL, "");
	mpmenue->hide();
	delete mpmenue;
}

