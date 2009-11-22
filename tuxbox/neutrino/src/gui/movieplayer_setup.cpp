/*
	$Id: movieplayer_setup.cpp,v 1.4 2009/11/22 15:36:52 rhabarber1848 Exp $

	movieplayer setup implementation - Neutrino-GUI

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


#include "gui/movieplayer_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/dirchooser.h>

#include "gui/filebrowser.h"

#include <driver/screen_max.h>

#include <system/debug.h>


CMoviePlayerSetup::CMoviePlayerSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CMoviePlayerSetup::~CMoviePlayerSetup()
{

}

int CMoviePlayerSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init movieplayer setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}
	
	if(actionKey == "movieplugin")
	{
		showMoviePlayerSelectPlugin();
 		return res;
	}
	showMoviePlayerSetup();
	
	return res;
}

void CMoviePlayerSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

/* for streaming settings menu */

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

#define STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC_OPTION_COUNT 2
const CMenuOptionChooser::keyval STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC_OPTIONS[STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC_OPTION_COUNT] =
{
	{ 0, LOCALE_STREAMINGMENU_MPEG1 },
	{ 1, LOCALE_STREAMINGMENU_MPEG2 }
};

#define STREAMINGMENU_STREAMING_RESOLUTION_OPTION_COUNT 4
const CMenuOptionChooser::keyval STREAMINGMENU_STREAMING_RESOLUTION_OPTIONS[STREAMINGMENU_STREAMING_RESOLUTION_OPTION_COUNT] =
{
	{ 0, LOCALE_STREAMINGMENU_352X288 },
	{ 1, LOCALE_STREAMINGMENU_352X576 },
	{ 2, LOCALE_STREAMINGMENU_480X576 },
	{ 3, LOCALE_STREAMINGMENU_704X576 }
};

#define STREAMINGMENU_STREAMING_TYPE_OPTION_COUNT 2
const CMenuOptionChooser::keyval STREAMINGMENU_STREAMING_TYPE_OPTIONS[STREAMINGMENU_STREAMING_TYPE_OPTION_COUNT] =
{
	{ 0, LOCALE_STREAMINGMENU_OFF },
	{ 1, LOCALE_STREAMINGMENU_ON  }
};

#define STREAMINGMENU_STOPSECTIONSD_OPTION_COUNT 3
const CMenuOptionChooser::keyval STREAMINGMENU_STOPSECTIONSD_OPTIONS[STREAMINGMENU_STOPSECTIONSD_OPTION_COUNT] =
{
	{ 0, LOCALE_RECORDINGMENU_SECTIONSD_RUN     },
	{ 1, LOCALE_RECORDINGMENU_SECTIONSD_STOP    },
	{ 2, LOCALE_RECORDINGMENU_SECTIONSD_RESTART }
};

void CMoviePlayerSetup::showMoviePlayerSetup()
{
	CMenuWidget* mp_setup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_SETTINGS, width);
	mp_setup->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MAINSETTINGS_STREAMING));

	// intros
	mp_setup->addItem(GenericMenuSeparator);
	mp_setup->addItem(GenericMenuBack);
	mp_setup->addItem(GenericMenuSeparatorLine);

	// server ip
	CIPInput * mp_setup_server_ip = new CIPInput(LOCALE_STREAMINGMENU_SERVER_IP, g_settings.streaming_server_ip, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CStringInput * mp_setup_server_port = new CStringInput(LOCALE_STREAMINGMENU_SERVER_PORT, g_settings.streaming_server_port, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2,"0123456789 ");
	CStringInputSMS * cddriveInput = new CStringInputSMS(LOCALE_STREAMINGMENU_STREAMING_SERVER_CDDRIVE, g_settings.streaming_server_cddrive, 20, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-:\\ ");
	CStringInput * mp_setup_videorate = new CStringInput(LOCALE_STREAMINGMENU_STREAMING_VIDEORATE, g_settings.streaming_videorate, 5, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2,"0123456789 ");
	CStringInput * mp_setup_audiorate = new CStringInput(LOCALE_STREAMINGMENU_STREAMING_AUDIORATE, g_settings.streaming_audiorate, 5, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2,"0123456789 ");
	CStringInputSMS * startdirInput = new CStringInputSMS(LOCALE_STREAMINGMENU_STREAMING_SERVER_STARTDIR, g_settings.streaming_server_startdir, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE,"abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-_:\\ ");

	CMenuForwarder* mf1 = new CMenuForwarder(LOCALE_STREAMINGMENU_SERVER_IP                , (g_settings.streaming_type==1), g_settings.streaming_server_ip      , mp_setup_server_ip);
	CMenuForwarder* mf2 = new CMenuForwarder(LOCALE_STREAMINGMENU_SERVER_PORT              , (g_settings.streaming_type==1), g_settings.streaming_server_port    , mp_setup_server_port);
	CMenuForwarder* mf3 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_SERVER_CDDRIVE , (g_settings.streaming_type==1), g_settings.streaming_server_cddrive , cddriveInput);
	CMenuForwarder* mf4 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_VIDEORATE      , (g_settings.streaming_type==1), g_settings.streaming_videorate      , mp_setup_videorate);
	CMenuForwarder* mf5 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_AUDIORATE      , (g_settings.streaming_type==1), g_settings.streaming_audiorate      , mp_setup_audiorate);
	CMenuForwarder* mf6 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_SERVER_STARTDIR, (g_settings.streaming_type==1), g_settings.streaming_server_startdir, startdirInput);

	// startdir
	CDirChooser* startdir = new CDirChooser(&g_settings.streaming_moviedir);	
	CMenuForwarder* mf7 = new CMenuForwarder(LOCALE_MOVIEPLAYER_DEFDIR, true, g_settings.streaming_moviedir, startdir);

	CMenuForwarder* mf8 = new CMenuForwarder(LOCALE_MOVIEPLAYER_DEFPLUGIN, true, g_settings.movieplayer_plugin,this,"movieplugin");
	CMenuOptionChooser* oj1 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_TRANSCODE_AUDIO      , &g_settings.streaming_transcode_audio      , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, g_settings.streaming_type);

	CMenuOptionChooser* oj2 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_FORCE_AVI_RAWAUDIO   , &g_settings.streaming_force_avi_rawaudio   , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, g_settings.streaming_type);

	CMenuOptionChooser* oj3 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_FORCE_TRANSCODE_VIDEO, &g_settings.streaming_force_transcode_video, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, g_settings.streaming_type);

// not yet supported by VLC
	CMenuOptionChooser* oj4 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC, &g_settings.streaming_transcode_video_codec, STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC_OPTIONS, STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC_OPTION_COUNT, g_settings.streaming_type);

	CMenuOptionChooser* oj5 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_RESOLUTION           , &g_settings.streaming_resolution           , STREAMINGMENU_STREAMING_RESOLUTION_OPTIONS, STREAMINGMENU_STREAMING_RESOLUTION_OPTION_COUNT, g_settings.streaming_type);

	CStreamingNotifier *StreamingNotifier = new CStreamingNotifier(mf1,mf2,mf3,mf4,mf5,mf6,oj1,oj2,oj3,oj4,oj5);

#ifndef ENABLE_MOVIEPLAYER2
	CIntInput * mp_setup_buffer_size = new CIntInput(LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE, (long&)g_settings.streaming_buffer_segment_size,3, LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE_HINT1, LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE_HINT2);
	CMenuForwarder* mf9 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE , g_settings.streaming_use_buffer, mp_setup_buffer_size->getValue()      , mp_setup_buffer_size);
	COnOffNotifier *bufferNotifier = new COnOffNotifier(mf9);
	CMenuOptionChooser* oj6 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_USE_BUFFER , &g_settings.streaming_use_buffer  , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true,bufferNotifier);
#endif
	CMenuOptionChooser* oj7 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_SECTIONSD , &g_settings.streaming_stopsectionsd  , STREAMINGMENU_STOPSECTIONSD_OPTIONS, STREAMINGMENU_STOPSECTIONSD_OPTION_COUNT, true);
	CMenuOptionChooser* oj8 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_SHOW_TV_IN_BROWSER , &g_settings.streaming_show_tv_in_browser  , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);
	CMenuOptionChooser* oj9 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_FILEBROWSER_ALLOW_MULTISELECT , &g_settings.streaming_allow_multiselect  , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);


	// enable/disable streamingserver
	mp_setup->addItem(new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_TYPE                  , &g_settings.streaming_type                 , STREAMINGMENU_STREAMING_TYPE_OPTIONS, STREAMINGMENU_STREAMING_TYPE_OPTION_COUNT, true, StreamingNotifier));

	mp_setup->addItem(GenericMenuSeparatorLine);	//separator
	mp_setup->addItem( mf1);				//Server IP
	mp_setup->addItem( mf2);				//Server Port
	mp_setup->addItem( mf3);				//CD-Drive
	mp_setup->addItem( mf6);				//vlc Startdir
	mp_setup->addItem(GenericMenuSeparatorLine);	//separator	
	mp_setup->addItem( mf4);				//Video-Rate
	mp_setup->addItem( oj3);				//transcode
	mp_setup->addItem( oj4);				//codec
	mp_setup->addItem( oj5);				//definition
	mp_setup->addItem(GenericMenuSeparatorLine);	//separator
	mp_setup->addItem( mf5);				//Audiorate
	mp_setup->addItem( oj1);				//transcode audio
	mp_setup->addItem( oj2);				//ac3 on avi
	mp_setup->addItem(GenericMenuSeparatorLine);	//separator
	mp_setup->addItem( mf7);				//default startdir
	mp_setup->addItem( mf8);				//default movieplugin
#ifndef ENABLE_MOVIEPLAYER2
	mp_setup->addItem(GenericMenuSeparatorLine);	//separator
	mp_setup->addItem( oj6 );				//used buffer
	mp_setup->addItem( mf9 );				//buffer segment size
#endif
	mp_setup->addItem( oj7 );				//stop epg/sectionsd
	mp_setup->addItem( oj8 );				//tv in browser
	mp_setup->addItem( oj9 );				//mutltiselect in filebrowser

	mp_setup->exec (NULL, "");
	mp_setup->hide ();
	delete mp_setup;
}

void CMoviePlayerSetup::showMoviePlayerSelectPlugin()
{
	CMenuWidget * MoviePluginSelector = new CMenuWidget(LOCALE_MAINSETTINGS_STREAMING, NEUTRINO_ICON_FEATURES, width);
	MoviePluginSelector->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MOVIEPLAYER_DEFPLUGIN));

	MoviePluginSelector->addItem(GenericMenuSeparator);
	MoviePluginSelector->addItem(GenericMenuBack);
	MoviePluginSelector->addItem(GenericMenuSeparatorLine);

	CMoviePluginChangeExec *MoviePluginChanger = new CMoviePluginChangeExec;

	char id[5];
	int cnt = 0;
	int enabled_count = 0;
	for(unsigned int count=0;count < (unsigned int) g_PluginList->getNumberOfPlugins();count++)
	{
		if (g_PluginList->getType(count)== CPlugins::P_TYPE_TOOL && !g_PluginList->isHidden(count))
		{
			// zB vtxt-plugins
			sprintf(id, "%d", count);
			enabled_count++;
			MoviePluginSelector->addItem(new CMenuForwarderNonLocalized(g_PluginList->getName(count), true, NULL, MoviePluginChanger, id, CRCInput::convertDigitToKey(count)), (cnt == 0));
			cnt++;
		}
	}

	MoviePluginSelector->exec(NULL, "");
	MoviePluginSelector->hide ();
	delete MoviePluginSelector;
	delete MoviePluginChanger;
}
