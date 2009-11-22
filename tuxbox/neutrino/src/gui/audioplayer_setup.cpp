/*
	$Id: audioplayer_setup.cpp,v 1.2 2009/11/22 15:36:52 rhabarber1848 Exp $

	audioplayer setup implementation - Neutrino-GUI

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


#include "gui/audioplayer_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>

#include "gui/audioplayer.h"
#include "gui/filebrowser.h"

#include <driver/screen_max.h>

#include <system/debug.h>



CAudioPlayerSetup::CAudioPlayerSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CAudioPlayerSetup::~CAudioPlayerSetup()
{

}

int CAudioPlayerSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init audioplayer setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if(actionKey == "save") {
		CNeutrinoApp::getInstance()->saveSetup(); // Save the settings !
		return res;
	}

	if(actionKey == "audioplayerdir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.audioplayer_audioplayerdir))
			strncpy(g_settings.audioplayer_audioplayerdir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.audioplayer_audioplayerdir)-1);
		return res;
	}

	showAudioPlayerSetup();
	
	return res;
}

void CAudioPlayerSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};


#define AUDIOPLAYER_DISPLAY_ORDER_OPTION_COUNT 2
const CMenuOptionChooser::keyval AUDIOPLAYER_DISPLAY_ORDER_OPTIONS[AUDIOPLAYER_DISPLAY_ORDER_OPTION_COUNT] =
{
	{ CAudioPlayerGui::ARTIST_TITLE, LOCALE_AUDIOPLAYER_ARTIST_TITLE },
	{ CAudioPlayerGui::TITLE_ARTIST, LOCALE_AUDIOPLAYER_TITLE_ARTIST }
};


void CAudioPlayerSetup::showAudioPlayerSetup()
/*shows the audio setup menue*/
{

	CMenuWidget* audioplayerSetup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_SETTINGS, width);
	audioplayerSetup->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MAINMENU_AUDIOPLAYER));

	// intros
	audioplayerSetup->addItem(GenericMenuSeparator);
	audioplayerSetup->addItem(GenericMenuBack);
	audioplayerSetup->addItem(GenericMenuSeparatorLine);

	// display order
	audioplayerSetup->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_DISPLAY_ORDER, &g_settings.audioplayer_display , AUDIOPLAYER_DISPLAY_ORDER_OPTIONS, AUDIOPLAYER_DISPLAY_ORDER_OPTION_COUNT, true ));
	// auto select current
	audioplayerSetup->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_FOLLOW   , &g_settings.audioplayer_follow  , MESSAGEBOX_NO_YES_OPTIONS  , MESSAGEBOX_NO_YES_OPTION_COUNT  , true ));
	// search title by name/tilte
	audioplayerSetup->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_SELECT_TITLE_BY_NAME   , &g_settings.audioplayer_select_title_by_name  , MESSAGEBOX_NO_YES_OPTIONS  , MESSAGEBOX_NO_YES_OPTION_COUNT  , true ));
	// auto repeat y/n
	audioplayerSetup->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_REPEAT_ON   , &g_settings.audioplayer_repeat_on  , MESSAGEBOX_NO_YES_OPTIONS  , MESSAGEBOX_NO_YES_OPTION_COUNT  , true ));
	// show playlist y/n
	audioplayerSetup->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_SHOW_PLAYLIST, &g_settings.audioplayer_show_playlist, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true ));
	// screensaver timeout
	CStringInput * audio_screensaver= new CStringInput(LOCALE_AUDIOPLAYER_SCREENSAVER_TIMEOUT, g_settings.audioplayer_screensaver, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	audioplayerSetup->addItem(new CMenuForwarder(LOCALE_AUDIOPLAYER_SCREENSAVER_TIMEOUT, true, g_settings.audioplayer_screensaver, audio_screensaver));
	// high decode priority
	audioplayerSetup->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_HIGHPRIO , &g_settings.audioplayer_highprio    , MESSAGEBOX_NO_YES_OPTIONS  , MESSAGEBOX_NO_YES_OPTION_COUNT  , true ));
	// start directory
	audioplayerSetup->addItem(new CMenuForwarder(LOCALE_AUDIOPLAYER_DEFDIR, true, g_settings.audioplayer_audioplayerdir, this, "audioplayerdir"));
	// shoutcast metadata parsing y/n
	audioplayerSetup->addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_ENABLE_SC_METADATA, &g_settings.audioplayer_enable_sc_metadata, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true ));

	audioplayerSetup->exec (NULL, "");
	audioplayerSetup->hide ();
	delete audioplayerSetup;

}
