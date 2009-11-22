/*
	$Id: mediaplayer_setup.cpp,v 1.2 2009/11/22 15:36:52 rhabarber1848 Exp $

	Neutrino-GUI  -   DBoxII-Project

	media player setup implementation - Neutrino-GUI

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

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/mediaplayer_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include "gui/widget/stringinput.h"

#ifdef ENABLE_AUDIOPLAYER
#include "gui/audioplayer_setup.h"
#endif

#ifdef ENABLE_PICTUREVIEWER
#include "gui/pictureviewer_setup.h"
#endif

#ifdef ENABLE_ESD
#include "gui/esd_setup.h"
#endif

#ifdef ENABLE_MOVIEPLAYER
#include "gui/movieplayer_setup.h"
#endif


#include <driver/screen_max.h>

#include <system/debug.h>



CMediaPlayerSetup::CMediaPlayerSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CMediaPlayerSetup::~CMediaPlayerSetup()
{

}

int CMediaPlayerSetup::exec(CMenuTarget* parent, const std::string & /*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init mediaplayer setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}


	showMediaPlayerSetup();
	
	return res;
}

void CMediaPlayerSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}



void CMediaPlayerSetup::showMediaPlayerSetup()
/*shows media setup menue entries*/
{

	CMenuWidget* mediaSetup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_SETTINGS, width);
	mediaSetup->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MEDIAPLAYERSETTINGS_GENERAL));

	// intros
	mediaSetup->addItem(GenericMenuSeparator);
	mediaSetup->addItem(GenericMenuBack);
	mediaSetup->addItem(GenericMenuSeparatorLine);

	// entries
#ifdef ENABLE_AUDIOPLAYER
	// audioplayer
	mediaSetup->addItem(new CMenuForwarder(LOCALE_MAINMENU_AUDIOPLAYER, true, NULL, new CAudioPlayerSetup, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
#endif
#ifdef ENABLE_ESD
	// esound
	mediaSetup->addItem(new CMenuForwarder(LOCALE_ESOUND_NAME, true, NULL, new CEsdSetup, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
#endif
#ifdef ENABLE_MOVIEPLAYER
	// movieplayer
	mediaSetup->addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_STREAMING, true, NULL, new CMoviePlayerSetup, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
#endif
#ifdef ENABLE_PICTUREVIEWER
	// pictureviewer
	mediaSetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_HEAD, true, NULL, new CPictureViewerSetup, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
#endif

	mediaSetup->exec (NULL, "");
	mediaSetup->hide ();
	delete mediaSetup;

}
