/*
	$Id: neutrino_menu.cpp,v 1.90 2009/10/27 20:28:42 dbt Exp $
	
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2008, 2009 Stefan Seyfried

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

#include <dirent.h>

#include <global.h>
#include <neutrino.h>

#include <system/debug.h>
#include <system/flashtool.h>

#include <driver/encoding.h>
#include <driver/vcrcontrol.h>

#include "gui/bedit/bouqueteditor_bouquets.h"
#include "gui/widget/colorchooser.h"
#include "gui/widget/dirchooser.h"
#include "gui/widget/icons.h"
#include "gui/widget/keychooser.h"
#include "gui/widget/lcdcontroler.h"
#include "gui/widget/rgbcsynccontroler.h"
#include "gui/widget/stringinput.h"
#include "gui/widget/stringinput_ext.h"

#include "gui/alphasetup.h"
#include "gui/audio_setup.h"
#include "gui/video_setup.h"
#include "gui/audio_select.h"
#ifdef ENABLE_AUDIOPLAYER
#include "gui/audioplayer.h"
#endif
#ifdef ENABLE_ESD
#include "gui/esound.h"
#endif
#include "gui/epgplus.h"
#include "gui/favorites.h"
#include "gui/imageinfo.h"
#include "gui/keyhelper.h"
#if defined(ENABLE_AUDIOPLAYER) || defined(ENABLE_INTERNETRADIO) || defined(ENABLE_ESD) || defined(ENABLE_PICTUREVIEWER) || defined (ENABLE_MOVIEPLAYER)
#include "gui/mediaplayer_setup.h"
#endif
#ifdef ENABLE_MOVIEPLAYER
#include "gui/movieplayer_setup.h"
#endif
#include "gui/motorcontrol.h"
#ifdef ENABLE_GUI_MOUNT
#include "gui/nfs.h"
#endif
#include "gui/personalize.h"
#ifdef ENABLE_PICTUREVIEWER
#include "gui/pictureviewer.h"
#endif
#include "gui/pluginlist.h"
#include "gui/scan.h"
#include "gui/scan_setup.h"
#include "gui/screensetup.h"
#include "gui/software_update.h"
#include "gui/streaminfo2.h"
#include "gui/sleeptimer.h"
#include "gui/update.h"
#include "gui/themes.h"

#if ENABLE_UPNP
#include "gui/upnpbrowser.h"
#endif

#ifdef _EXPERIMENTAL_SETTINGS_
#include "gui/experimental_menu.h"
#endif

static CTimingSettingsNotifier timingsettingsnotifier;

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  init main menu                                             *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::InitMainMenu(CMenuWidget &mainMenu,
								CMenuWidget &mainSettings,
								CMenuWidget &parentallockSettings,
								CMenuWidget &networkSettings,
								CMenuWidget &recordingSettings,
								CMenuWidget &colorSettings,
								CMenuWidget &lcdSettings,
								CMenuWidget &keySettings,
								CMenuWidget &languageSettings,
								CMenuWidget &miscSettings,
								CMenuWidget &driverSettings,
#ifdef ENABLE_MOVIEPLAYER
								CMenuWidget &moviePlayer,
#endif
								CMenuWidget &service)
{
	dprintf(DEBUG_DEBUG, "init mainmenue\n");

	// Dynamic renumbering
	int shortcut = 1;
	int shortcut2 = 1;

	CPersonalizeGui	*personalize;
	personalize = new CPersonalizeGui;
	
	// Main Menu
	mainMenu.addItem(GenericMenuSeparator);

	//tv-mode
 	personalize->addItem(mainMenu, LOCALE_MAINMENU_TVMODE, true, NULL, this, "tv", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED, (g_settings.startmode == STARTMODE_TV || firstchannel.mode != 'r' ), g_settings.personalize_tvmode);

	//radio mode
	personalize->addItem(mainMenu, LOCALE_MAINMENU_RADIOMODE, true, NULL, this, "radio", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN, (g_settings.startmode == STARTMODE_RADIO || (!(g_settings.startmode == STARTMODE_TV) && firstchannel.mode == 'r')),g_settings.personalize_radiomode);

	//scart
	personalize->addItem(mainMenu, LOCALE_MAINMENU_SCARTMODE, true, NULL, this, "scart", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, false, g_settings.personalize_scartmode);

	//games
	personalize->addItem(mainMenu, LOCALE_MAINMENU_GAMES, true, NULL, new CPluginList(LOCALE_MAINMENU_GAMES,CPlugins::P_TYPE_GAME), "", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE, false, g_settings.personalize_games);

	//separator	
	if (	g_settings.personalize_tvmode		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_radiomode	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_scartmode	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_games		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE)
		;// Stop seperator from appearing when menu entries have been hidden	
	else
		mainMenu.addItem(GenericMenuSeparatorLine); 
	
#ifdef ENABLE_AUDIOPLAYER
	// audioplayer
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_AUDIOPLAYER, true, NULL, new CAudioPlayerGui(), NULL, CRCInput::convertDigitToKey(shortcut), NULL, false,   g_settings.personalize_audioplayer);
	
#ifdef ENABLE_INTERNETRADIO
	// internet player
	shortcut += personalize->addItem(mainMenu, LOCALE_INETRADIO_NAME, true, NULL, new CAudioPlayerGui(true), NULL, CRCInput::convertDigitToKey(shortcut), NULL, false, g_settings.personalize_inetradio);
#endif
#endif	

#ifdef ENABLE_ESD
	// esound
	if (access("/bin/esd", X_OK) == 0 || access("/var/bin/esd", X_OK) == 0)
	{
		puts("[neutrino] found esound, adding personalized esound entry to mainmenue");
		shortcut += personalize->addItem(mainMenu, LOCALE_ESOUND_NAME, true, NULL, new CEsoundGui(), NULL, CRCInput::convertDigitToKey(shortcut), NULL, false, g_settings.personalize_esound);
	}
#endif

#ifdef ENABLE_MOVIEPLAYER
	// movieplayer
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_MOVIEPLAYER, true, NULL, &moviePlayer, NULL, CRCInput::convertDigitToKey(shortcut), NULL, false, g_settings.personalize_movieplayer);

	addMenueIntroItems(moviePlayer);

	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, moviePlayerGui, "tsplayback", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK_PC, true, NULL, moviePlayerGui, "tsplayback_pc", CRCInput::RC_1));
#ifdef ENABLE_MOVIEBROWSER
#ifndef ENABLE_MOVIEPLAYER2
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, moviePlayerGui, "tsmoviebrowser", CRCInput::RC_2));
#else
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, movieBrowser, "run", CRCInput::RC_2));
#endif /* ENABLE_MOVIEPLAYER2 */
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_BOOKMARK, true, NULL, moviePlayerGui, "bookmarkplayback", CRCInput::RC_3));
#else
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_BOOKMARK, true, NULL, moviePlayerGui, "bookmarkplayback", CRCInput::RC_2));
#endif /* ENABLE_MOVIEBROWSER */
	moviePlayer.addItem(GenericMenuSeparatorLine);
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_FILEPLAYBACK, true, NULL, moviePlayerGui, "fileplayback", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_DVDPLAYBACK, true, NULL, moviePlayerGui, "dvdplayback", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_VCDPLAYBACK, true, NULL, moviePlayerGui, "vcdplayback", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	moviePlayer.addItem(GenericMenuSeparatorLine);
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, new CMoviePlayerSetup(), NULL, CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP_SMALL));
#ifdef ENABLE_GUI_MOUNT
	moviePlayer.addItem(new CMenuForwarder(LOCALE_NETWORKMENU_MOUNT, true, NULL, new CNFSSmallMenu(), NULL, CRCInput::RC_setup, NEUTRINO_ICON_BUTTON_DBOX_SMALL));
#endif
#endif

#ifdef ENABLE_PICTUREVIEWER
	// pictureviewer
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_PICTUREVIEWER, true, NULL, new CPictureViewerGui(), NULL, CRCInput::convertDigitToKey(shortcut), NULL, false, g_settings.personalize_pictureviewer);
#endif

#if ENABLE_UPNP
	// upnpbrowser
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_UPNPBROWSER, true, NULL, new CUpnpBrowserGui(), NULL, CRCInput::convertDigitToKey(shortcut), NULL, false, g_settings.personalize_upnpbrowser);
#endif

	// scripts
	if (g_PluginList->hasPlugin(CPlugins::P_TYPE_SCRIPT))
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SCRIPTS, true, NULL, new CPluginList(LOCALE_MAINMENU_SCRIPTS,CPlugins::P_TYPE_SCRIPT), "", CRCInput::convertDigitToKey(shortcut++)));

#if defined(ENABLE_AUDIOPLAYER) || defined(ENABLE_INTERNETRADIO) || defined(ENABLE_ESD) || defined(ENABLE_MOVIEPLAYER) || defined(ENABLE_PICTUREVIEWER) || defined(ENABLE_UPNP)
	// separator
	if (	g_settings.personalize_audioplayer	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_inetradio	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_esound		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_movieplayer	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_pictureviewer	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_upnpbrowser	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE)
		;// Stop seperator from appearing when menu entries have been hidden
	else
		mainMenu.addItem(GenericMenuSeparatorLine); 
#endif
	// settings
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_SETTINGS, true, NULL, &mainSettings, NULL, CRCInput::convertDigitToKey(shortcut), NULL, false, CPersonalizeGui::PERSONALIZE_MODE_VISIBLE, g_settings.personalize_settings);

	// service
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_SERVICE, true, NULL, &service, NULL, CRCInput::convertDigitToKey(shortcut), NULL, false, CPersonalizeGui::PERSONALIZE_MODE_VISIBLE, g_settings.personalize_service);

	//separator
	if (	g_settings.personalize_sleeptimer	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_reboot		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_shutdown		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE)
		 ;// Stop seperator from appearing when menu entries have been hidden
	else
		mainMenu.addItem(GenericMenuSeparatorLine);

	//10. -- only 10 shortcuts (1-9, 0), the next could be the last also!(10. => 0)
	//sleeptimer
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_SLEEPTIMER, true, NULL, new CSleepTimerWidget, NULL, personalize->setShortcut(shortcut), NULL, false, g_settings.personalize_sleeptimer);

	// reboot
	shortcut += personalize->addItem(mainMenu, LOCALE_MAINMENU_REBOOT, true, NULL, this, "reboot", personalize->setShortcut(shortcut), NULL, false, g_settings.personalize_reboot);

	// shutdown
	personalize->addItem(mainMenu, LOCALE_MAINMENU_SHUTDOWN, true, NULL, this, "shutdown", CRCInput::RC_standby, NEUTRINO_ICON_BUTTON_POWER, false, g_settings.personalize_shutdown);

//----------------------
	//red Settings Menu
	addMenueIntroItems(mainSettings);
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	mainSettings.addItem(GenericMenuSeparatorLine);

	// video.
	shortcut2 += personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_VIDEO, true, NULL, new CVideoSetup(), NULL, CRCInput::convertDigitToKey(shortcut2), NULL, false, g_settings.personalize_video);	

	// audio
	shortcut2 += personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_AUDIO, true, NULL, new CAudioSetup(), NULL, CRCInput::convertDigitToKey(shortcut2), NULL, false, g_settings.personalize_audio);
	
	// parental lock
	if (g_settings.personalize_youth == CPersonalizeGui::PERSONALIZE_MODE_VISIBLE) {
		if(g_settings.parentallock_prompt)
			mainSettings.addItem(new CLockedMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, g_settings.parentallock_pincode, true, true, NULL, &parentallockSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
		else
			mainSettings.addItem(new CMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, true, NULL, &parentallockSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	}
	else if (g_settings.personalize_youth == CPersonalizeGui::PERSONALIZE_MODE_PIN)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, g_settings.personalize_pincode, true, true, NULL, &parentallockSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	// network
	shortcut2 += personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_NETWORK, true, NULL, &networkSettings, NULL, CRCInput::convertDigitToKey(shortcut2), NULL, false, g_settings.personalize_network);

	// record settings
	shortcut2 += personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_RECORDING, true, NULL, &recordingSettings, NULL, CRCInput::convertDigitToKey(shortcut2), NULL, false,g_settings.personalize_recording);

	// language
	shortcut2 += personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_LANGUAGE, true, NULL, &languageSettings , NULL, CRCInput::convertDigitToKey(shortcut2), NULL, false, g_settings.personalize_language);

	// color
	shortcut2 += personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_COLORS, true, NULL, &colorSettings, NULL, CRCInput::convertDigitToKey(shortcut2), NULL, false, g_settings.personalize_colors);
	
	// lcd.
	shortcut2 += personalize->addItem(mainSettings,LOCALE_MAINSETTINGS_LCD, true, NULL, &lcdSettings, NULL, CRCInput::convertDigitToKey(shortcut2), NULL, false, g_settings.personalize_lcd);
	
	//10. -- only 10 shortcuts (1-9, 0), the next could be the last also!(10. => 0)
	//keybindings
	shortcut2 += personalize->addItem(mainSettings,LOCALE_MAINSETTINGS_KEYBINDING, true, NULL, &keySettings, NULL, personalize->setShortcut(shortcut2), NULL, false, g_settings.personalize_keybinding);
	
	//blue (audioplayer, pictureviewer, esd, mediaplayer)
#if defined(ENABLE_AUDIOPLAYER) || defined(ENABLE_PICTUREVIEWER) || defined(ENABLE_ESD) || defined(ENABLE_MOVIEPLAYER)
	personalize->addItem(mainSettings, LOCALE_MEDIAPLAYERSETTINGS_GENERAL, true, NULL, new CMediaPlayerSetup, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE, false, g_settings.personalize_mediaplayer);
#endif

	//green (driver/boot settings)
	personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_DRIVER, true, NULL, &driverSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN, false, g_settings.personalize_driver);
	
	//yellow (miscSettings)
	personalize->addItem(mainSettings, LOCALE_MAINSETTINGS_MISC, true, NULL, &miscSettings, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, false, g_settings.personalize_misc);
	
	// personalize
	shortcut2 += personalize->addItem(mainSettings, LOCALE_PERSONALIZE_HEAD, true, NULL, new CPersonalizeGui(), NULL, personalize->setShortcut(shortcut2), NULL, false, CPersonalizeGui::PERSONALIZE_MODE_VISIBLE, g_settings.personalize_pinstatus);

	delete personalize ;
}

const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  }
};

/* service menu*/
void CNeutrinoApp::InitServiceSettings(CMenuWidget &service)
{
	dprintf(DEBUG_DEBUG, "init serviceSettings\n");

	CPersonalizeGui	*personalize;
	personalize = new CPersonalizeGui;

	// Dynamic renumbering
	int shortcut3 = 1;

	addMenueIntroItems(service);

	// bouquets
	personalize->addItem(service, LOCALE_BOUQUETEDITOR_NAME, true, NULL, new CBEBouquetWidget(), NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED, false, g_settings.personalize_bouqueteditor);

	// channel scan
	personalize->addItem(service, LOCALE_SERVICEMENU_SCANTS, true, NULL, new CScanSetup, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN, false, g_settings.personalize_scants);

	// separator
	if (	g_settings.personalize_bouqueteditor	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_scants		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE);
		// Stop seperator from appearing when menu entries have been hidden
	else
		service.addItem(GenericMenuSeparatorLine); 

	// reload channels
	shortcut3 += personalize->addItem(service, LOCALE_SERVICEMENU_RELOAD, true, NULL, this, "reloadchannels", CRCInput::convertDigitToKey(shortcut3), NULL, false, g_settings.personalize_reload);

	// reload plugins
	shortcut3 += personalize->addItem(service, LOCALE_SERVICEMENU_GETPLUGINS, true, NULL, this, "reloadplugins" , CRCInput::convertDigitToKey(shortcut3), NULL, false, g_settings.personalize_getplugins);
	
	// restart neutrino
	shortcut3 += personalize->addItem(service, LOCALE_SERVICEMENU_RESTART, true, NULL, this, "restart", CRCInput::convertDigitToKey(shortcut3), NULL, false, g_settings.personalize_restart);
	
	// epg restart
	shortcut3 += personalize->addItem(service, LOCALE_SERVICEMENU_EPGRESTART, true, NULL, this, "EPGrestart", CRCInput::convertDigitToKey(shortcut3), NULL, false, g_settings.personalize_epgrestart);
	
#ifdef HAVE_DBOX_HARDWARE
	// ucode check
	shortcut3 += personalize->addItem(service, LOCALE_SERVICEMENU_UCODECHECK, true, NULL, UCodeChecker, NULL, CRCInput::convertDigitToKey(shortcut3), NULL, false, g_settings.personalize_ucodecheck);
#endif

	// epg status
	shortcut3 += personalize->addItem(service, LOCALE_SERVICEMENU_CHAN_EPG_STAT, true, NULL, DVBInfo, NULL, CRCInput::convertDigitToKey(shortcut3), NULL, false, g_settings.personalize_chan_epg_stat);

	// separator
	if (	g_settings.personalize_reload		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_getplugins	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_restart		== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_epgrestart	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_ucodecheck	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE && 
		g_settings.personalize_chan_epg_stat	== CPersonalizeGui::PERSONALIZE_MODE_NOTVISIBLE);
		// Stop seperator from appearing when menu entries have been hidden
	else
		service.addItem(GenericMenuSeparatorLine); 

	//yellow imageinfo
	personalize->addItem(service, LOCALE_SERVICEMENU_IMAGEINFO, true, NULL, new CImageInfo(), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW, false, g_settings.personalize_imageinfo);

	//softupdate
	if(softupdate)
	{
		// blue software update
		personalize->addItem(service, LOCALE_SERVICEMENU_UPDATE, true, NULL, new CSoftwareUpdate(), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE, false, g_settings.personalize_update);
 	}

	delete personalize;
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

/* for misc settings menu */
#define OPTIONS_OFF1_ON0_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF1_ON0_OPTIONS[OPTIONS_OFF1_ON0_OPTION_COUNT] =
{
	{ 1, LOCALE_OPTIONS_OFF },
	{ 0, LOCALE_OPTIONS_ON  }
};

#define MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT 2
const CMenuOptionChooser::keyval MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS[MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT] =
{
	{ 0, LOCALE_FILESYSTEM_IS_UTF8_OPTION_ISO8859_1 },
	{ 1, LOCALE_FILESYSTEM_IS_UTF8_OPTION_UTF8      }
};

#define INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT 5
const CMenuOptionChooser::keyval  INFOBAR_SUBCHAN_DISP_POS_OPTIONS[INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_SETTINGS_POS_TOP_RIGHT },
	{ 1 , LOCALE_SETTINGS_POS_TOP_LEFT },
	{ 2 , LOCALE_SETTINGS_POS_BOTTOM_LEFT },
	{ 3 , LOCALE_SETTINGS_POS_BOTTOM_RIGHT },
	{ 4 , LOCALE_SETTINGS_POS_INFOBAR }
};

#define INFOBAR_SHOW_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  INFOBAR_SHOW_OPTIONS[INFOBAR_SHOW_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_OPTIONS_OFF },
	{ 1 , LOCALE_PICTUREVIEWER_RESIZE_SIMPLE },
	{ 2 , LOCALE_PICTUREVIEWER_RESIZE_COLOR_AVERAGE }
};

#define INFOBAR_EPG_SHOW_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  INFOBAR_EPG_SHOW_OPTIONS[INFOBAR_EPG_SHOW_OPTIONS_COUNT]=
{
   { 0 , LOCALE_OPTIONS_OFF },
   { 1 , LOCALE_INFOVIEWER_EPGINFO_SIMPLE_MESSAGE },
   { 2 , LOCALE_INFOVIEWER_EPGINFO_EXPENSIVE_MESSAGE }
};

#define VOLUMEBAR_DISP_POS_OPTIONS_COUNT 7
const CMenuOptionChooser::keyval  VOLUMEBAR_DISP_POS_OPTIONS[VOLUMEBAR_DISP_POS_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_SETTINGS_POS_TOP_RIGHT },
	{ 1 , LOCALE_SETTINGS_POS_TOP_LEFT },
	{ 2 , LOCALE_SETTINGS_POS_BOTTOM_LEFT },
	{ 3 , LOCALE_SETTINGS_POS_BOTTOM_RIGHT },
	{ 4 , LOCALE_SETTINGS_POS_DEFAULT_CENTER },
	{ 5 , LOCALE_SETTINGS_POS_HIGHER_CENTER },
	{ 6 , LOCALE_SETTINGS_POS_OFF }
};

#define SHOW_MUTE_ICON_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  SHOW_MUTE_ICON_OPTIONS[SHOW_MUTE_ICON_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_MISCSETTINGS_SHOW_MUTE_ICON_NO },
	{ 1 , LOCALE_MISCSETTINGS_SHOW_MUTE_ICON_YES },
	{ 2 , LOCALE_MISCSETTINGS_SHOW_MUTE_ICON_NOT_IN_AC3MODE }
};

#define CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS_COUNT 2
const CMenuOptionChooser::keyval  CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS[CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_CHANNELLIST_EPGTEXT_ALIGN_LEFT },
	{ 1 , LOCALE_CHANNELLIST_EPGTEXT_ALIGN_RIGHT }
};

#define INFOBAR_CHANNELLOGO_SHOW_OPTIONS_COUNT 4
const CMenuOptionChooser::keyval  INFOBAR_CHANNELLOGO_SHOW_OPTIONS[INFOBAR_CHANNELLOGO_SHOW_OPTIONS_COUNT]=
{
   { 0 , LOCALE_INFOVIEWER_CHANNELLOGO_OFF },
   { 1 , LOCALE_INFOVIEWER_CHANNELLOGO_SHOW_IN_NUMBERBOX },
   { 2 , LOCALE_INFOVIEWER_CHANNELLOGO_SHOW_AS_CHANNELNAME },
   { 3 , LOCALE_INFOVIEWER_CHANNELLOGO_SHOW_BESIDE_CHANNELNAME }
};

#define INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS[INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS_COUNT]=
{
   { 0 , LOCALE_INFOVIEWER_CHANNELLOGO_BACKGROUND_OFF },
   { 1 , LOCALE_INFOVIEWER_CHANNELLOGO_BACKGROUND_FRAMED },
   { 2 , LOCALE_INFOVIEWER_CHANNELLOGO_BACKGROUND_SHADED }
};

#define REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS_COUNT 4
const CMenuOptionChooser::keyval  REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS[REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS_COUNT]=
{
   { 0 , LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER },
   { 1 , LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER_OK },
   { 2 , LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER_HOME },
   { 3 , LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH_POWER_HOME_OK }
};

#if !defined(ENABLE_AUDIOPLAYER) && !defined(ENABLE_ESD)
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 5
#else
#if !defined(ENABLE_AUDIOPLAYER) && defined(ENABLE_ESD)
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 6
#else
#if defined(ENABLE_AUDIOPLAYER) && !defined(ENABLE_ESD)
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 7
#else
#define MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT 8
#endif
#endif
#endif
const CMenuOptionChooser::keyval  MISCSETTINGS_STARTMODE_WITH_OPTIONS[MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT]=
{
   { 0 , LOCALE_MISCSETTINGS_STARTMODE_RESTORE },
   { 1 , LOCALE_MAINMENU_TVMODE },
   { 2 , LOCALE_MAINMENU_RADIOMODE },
   { 3 , LOCALE_MAINMENU_SCARTMODE },
#ifdef ENABLE_AUDIOPLAYER
   { 4 , LOCALE_MAINMENU_AUDIOPLAYER },
   { 5 , LOCALE_INETRADIO_NAME },
#endif
#ifdef ENABLE_ESD
   { 6 , LOCALE_ESOUND_NAME },
#endif
   { 7 , LOCALE_TIMERLIST_TYPE_STANDBY },
};

/* misc settings menu */
void CNeutrinoApp::InitMiscSettings(CMenuWidget &miscSettings,
															CMenuWidget &miscSettingsGeneral,
															CMenuWidget &miscSettingsOSDExtras,
															CMenuWidget &miscSettingsInfobar,
															CMenuWidget &miscSettingsChannellist,
															CMenuWidget &miscSettingsEPGSettings,
															CMenuWidget &miscSettingsZapitSettings,
															CMenuWidget &miscSettingsRemoteControl,
															CMenuWidget &miscSettingsFilebrowser)
{
	dprintf(DEBUG_DEBUG, "init miscsettings\n");
	
	// auto numbering
	int shortcut = 1;
	
	// misc settings menue main view ///////////////////////
	addMenueIntroItems(miscSettings);
	// general
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_GENERAL, true, NULL, &miscSettingsGeneral, NULL,  CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));	
	// OSD-specials
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_OSD_SPECIALS, true, NULL, &miscSettingsOSDExtras, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	// infobar
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_INFOBAR, true, NULL, &miscSettingsInfobar, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	// channellist
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_CHANNELLIST, true, NULL, &miscSettingsChannellist, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	miscSettings.addItem(GenericMenuSeparatorLine);
	// epg settings
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_HEAD, true, NULL, &miscSettingsEPGSettings, NULL, CRCInput::convertDigitToKey(shortcut++)));
	// zapit settings
	miscSettings.addItem(new CMenuForwarder(LOCALE_ZAPITCONFIG_HEAD, true, NULL, &miscSettingsZapitSettings, NULL, CRCInput::convertDigitToKey(shortcut++)));
	// remote control
	miscSettings.addItem(new CMenuForwarder(LOCALE_KEYBINDINGMENU_RC, true, NULL, &miscSettingsRemoteControl, NULL, CRCInput::convertDigitToKey(shortcut++)));
	// filebrowser
	miscSettings.addItem(new CMenuForwarder(LOCALE_FILEBROWSER_HEAD, true, NULL, &miscSettingsFilebrowser, NULL, CRCInput::convertDigitToKey(shortcut++)));
	
	
	/* misc settings sub menues */
	// general
	miscSettingsGeneral.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_GENERAL));
	addMenueIntroItems(miscSettingsGeneral);
	CMenuOptionChooser *m6 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_STARTMODE, &g_settings.startmode, MISCSETTINGS_STARTMODE_WITH_OPTIONS, MISCSETTINGS_STARTMODE_WITH_OPTIONS_COUNT, true);
	CMenuOptionChooser *m1 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_SHUTDOWN_REAL_RCDELAY, &g_settings.shutdown_real_rcdelay, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, !g_settings.shutdown_real);
	CMenuOptionChooser *m5 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_RC_STANDBY_OFF_WITH, &g_settings.standby_off_with, REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS, REMOTE_CONTROL_STANDBY_OFF_WITH_OPTIONS_COUNT, !g_settings.shutdown_real);
	CShutdownCountNotifier *shutDownCountNotifier = new CShutdownCountNotifier;
	CStringInput * miscSettings_shutdown_count = new CStringInput(LOCALE_MISCSETTINGS_SHUTDOWN_COUNT, g_settings.shutdown_count, 3, LOCALE_MISCSETTINGS_SHUTDOWN_COUNT_HINT1, LOCALE_MISCSETTINGS_SHUTDOWN_COUNT_HINT2, "0123456789 ", shutDownCountNotifier);
	CMenuForwarder *m4 = new CMenuForwarder(LOCALE_MISCSETTINGS_SHUTDOWN_COUNT, !g_settings.shutdown_real, g_settings.shutdown_count, miscSettings_shutdown_count);
	CMenuOptionChooser *m3 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_STANDBY_SAVE_POWER, &g_settings.standby_save_power, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, !g_settings.shutdown_real);
	CMiscNotifier* miscNotifier = new CMiscNotifier(m1, m3, m4, m5);
	CMenuOptionChooser *m2 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_SHUTDOWN_REAL, &g_settings.shutdown_real, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, miscNotifier);
	miscSettingsGeneral.addItem(m6);
	miscSettingsGeneral.addItem(m2);
	miscSettingsGeneral.addItem(m3);
	miscSettingsGeneral.addItem(m4);
	miscSettingsGeneral.addItem(m1);
	miscSettingsGeneral.addItem(m5);

#ifndef TUXTXT_CFG_STANDALONE
	CTuxtxtCacheNotifier *tuxtxtcacheNotifier = new CTuxtxtCacheNotifier;
	miscSettingsGeneral.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_TUXTXT_CACHE, &g_settings.tuxtxt_cache, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, tuxtxtcacheNotifier));
#endif
	
	// OSD - specials
	miscSettingsOSDExtras.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_OSD_SPECIALS));
	addMenueIntroItems(miscSettingsOSDExtras);
	miscSettingsOSDExtras.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_VOLUMEBAR_DISP_POS, &g_settings.volumebar_disp_pos, VOLUMEBAR_DISP_POS_OPTIONS, VOLUMEBAR_DISP_POS_OPTIONS_COUNT, true));
	miscSettingsOSDExtras.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_SHOW_MUTE_ICON, &g_settings.show_mute_icon, SHOW_MUTE_ICON_OPTIONS, SHOW_MUTE_ICON_OPTIONS_COUNT, true));

	//infobar settings
	miscSettingsInfobar.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_INFOBAR));
	addMenueIntroItems(miscSettingsInfobar);
	miscSettingsInfobar.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_INFOBAR_SAT_DISPLAY, &g_settings.infobar_sat_display, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	miscSettingsInfobar.addItem(new CMenuOptionChooser(LOCALE_INFOVIEWER_SUBCHAN_DISP_POS, &g_settings.infobar_subchan_disp_pos, INFOBAR_SUBCHAN_DISP_POS_OPTIONS, INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT, true));
	miscSettingsInfobar.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_VIRTUAL_ZAP_MODE, &g_settings.virtual_zap_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	miscSettingsInfobar.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_INFOBAR_SHOW, &g_settings.infobar_show, INFOBAR_EPG_SHOW_OPTIONS, INFOBAR_EPG_SHOW_OPTIONS_COUNT, true));

#ifdef ENABLE_RADIOTEXT
	CRadiotextNotifier *radiotextNotifier = new CRadiotextNotifier;
	miscSettingsInfobar.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_RADIOTEXT, &g_settings.radiotext_enable, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, radiotextNotifier));
#endif
	miscSettingsInfobar.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_CHANNELLOGO));
	
	miscSettingsInfobar.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_CHANNELLOGO_SHOW, &g_settings.infobar_show_channellogo, INFOBAR_CHANNELLOGO_SHOW_OPTIONS, INFOBAR_CHANNELLOGO_SHOW_OPTIONS_COUNT, true));
	miscSettingsInfobar.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_CHANNELLOGO_LOGODIR, true, g_settings.infobar_channel_logodir, this, "channel_logodir"));
 	miscSettingsInfobar.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_CHANNELLOGO_BACKGROUND, &g_settings.infobar_channellogo_background, INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS, INFOBAR_CHANNELLOGO_BACKGROUND_SHOW_OPTIONS_COUNT, true));

	//channellist
	miscSettingsChannellist.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_CHANNELLIST));
	addMenueIntroItems(miscSettingsChannellist);
	miscSettingsChannellist.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_CHANNELLIST_EPGTEXT_ALIGN, &g_settings.channellist_epgtext_align_right, CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS, CHANNELLIST_EPGTEXT_ALIGN_RIGHT_OPTIONS_COUNT, true));
	miscSettingsChannellist.addItem(new CMenuOptionChooser(LOCALE_CHANNELLIST_EXTENDED, &g_settings.channellist_extended, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	
	//epg settings
	miscSettingsEPGSettings.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_EPG_HEAD));
	addMenueIntroItems(miscSettingsEPGSettings);
	CSectionsdConfigNotifier* sectionsdConfigNotifier = new CSectionsdConfigNotifier;
	CStringInput * miscSettings_epg_cache = new CStringInput(LOCALE_MISCSETTINGS_EPG_CACHE, &g_settings.epg_cache, 2,LOCALE_MISCSETTINGS_EPG_CACHE_HINT1, LOCALE_MISCSETTINGS_EPG_CACHE_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettingsEPGSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_CACHE, true, g_settings.epg_cache, miscSettings_epg_cache));
	CStringInput * miscSettings_epg_extendedcache = new CStringInput(LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE, &g_settings.epg_extendedcache, 2,LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE_HINT1, LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettingsEPGSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE, true, g_settings.epg_extendedcache, miscSettings_epg_extendedcache));
	CStringInput * miscSettings_epg_old_events = new CStringInput(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, &g_settings.epg_old_events, 2,LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettingsEPGSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, true, g_settings.epg_old_events, miscSettings_epg_old_events));
	CStringInput * miscSettings_epg_max_events = new CStringInput(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, &g_settings.epg_max_events, 5,LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettingsEPGSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, true, g_settings.epg_max_events, miscSettings_epg_max_events));
	miscSettingsEPGSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_DIR, true, g_settings.epg_dir, this, "epgdir"));

	//zapit settings
	miscSettingsZapitSettings.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_ZAPITCONFIG_HEAD));

	// remote control
	miscSettingsRemoteControl.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_RC));
	addMenueIntroItems(miscSettingsRemoteControl);
	keySetupNotifier = new CKeySetupNotifier;
	CStringInput * keySettings_repeat_genericblocker = new CStringInput(LOCALE_KEYBINDINGMENU_REPEATBLOCKGENERIC, g_settings.repeat_genericblocker, 3, LOCALE_REPEATBLOCKER_HINT_1, LOCALE_REPEATBLOCKER_HINT_2, "0123456789 ", keySetupNotifier);
	CStringInput * keySettings_repeatBlocker = new CStringInput(LOCALE_KEYBINDINGMENU_REPEATBLOCK, g_settings.repeat_blocker, 3, LOCALE_REPEATBLOCKER_HINT_1, LOCALE_REPEATBLOCKER_HINT_2, "0123456789 ", keySetupNotifier);
	keySetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	miscSettingsRemoteControl.addItem(new CMenuForwarder(LOCALE_KEYBINDINGMENU_REPEATBLOCK, true, g_settings.repeat_blocker, keySettings_repeatBlocker));
	miscSettingsRemoteControl.addItem(new CMenuForwarder(LOCALE_KEYBINDINGMENU_REPEATBLOCKGENERIC, true, g_settings.repeat_genericblocker, keySettings_repeat_genericblocker));
		
	// filebrowser
	miscSettingsFilebrowser.addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_FILEBROWSER_HEAD));
	addMenueIntroItems(miscSettingsFilebrowser);
	miscSettingsFilebrowser.addItem(new CMenuOptionChooser(LOCALE_FILESYSTEM_IS_UTF8            , &g_settings.filesystem_is_utf8            , MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS, MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT, true ));
	miscSettingsFilebrowser.addItem(new CMenuOptionChooser(LOCALE_FILEBROWSER_SHOWRIGHTS        , &g_settings.filebrowser_showrights        , MESSAGEBOX_NO_YES_OPTIONS              , MESSAGEBOX_NO_YES_OPTION_COUNT              , true ));
	miscSettingsFilebrowser.addItem(new CMenuOptionChooser(LOCALE_FILEBROWSER_DENYDIRECTORYLEAVE, &g_settings.filebrowser_denydirectoryleave, MESSAGEBOX_NO_YES_OPTIONS              , MESSAGEBOX_NO_YES_OPTION_COUNT              , true ));
}

/* These variables need to be defined outside InitZapitSettings,
   otherwise locale "INTERNAL ERROR - PLEASE REPORT" is displayed
   instead of the option values */
int remainingChannelsBouquet = 0;
int saveaudiopids = 0;
int savelastchannel = 0;
char CstartChannelRadio[30]; /* zapitclient.h, responseChannels, char name[30]; */
char CstartChannelTV[30];

void CNeutrinoApp::InitZapitSettings(CMenuWidget &miscSettingsZapitSettings)
{
	addMenueIntroItems(miscSettingsZapitSettings);
	CZapitSetupNotifier *zapitSetupNotifier = new CZapitSetupNotifier(NULL, NULL);

	savelastchannel = g_Zapit->getSaveLastChannel() ? 1 : 0;
	strcpy(CstartChannelRadio,g_Zapit->getChannelNrName(g_Zapit->getStartChannelRadio(), CZapitClient::MODE_RADIO).c_str());
	strcpy(CstartChannelTV,g_Zapit->getChannelNrName(g_Zapit->getStartChannelTV(), CZapitClient::MODE_TV).c_str());
	CMenuForwarder *c1 = new CMenuForwarder(LOCALE_ZAPITCONFIG_START_TV, !savelastchannel, CstartChannelTV, this, "zapit_starttv");
	CMenuForwarder *c2 = new CMenuForwarder(LOCALE_ZAPITCONFIG_START_RADIO, !savelastchannel, CstartChannelRadio, this, "zapit_startradio");
	CZapitSetupNotifier *zapitSaveLastNotifier = new CZapitSetupNotifier(c1, c2);
	miscSettingsZapitSettings.addItem(new CMenuOptionChooser(LOCALE_ZAPITCONFIG_SAVE_LAST_CHANNEL, &savelastchannel, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, zapitSaveLastNotifier));
	miscSettingsZapitSettings.addItem(c1);
	miscSettingsZapitSettings.addItem(c2);

	saveaudiopids = g_Zapit->getSaveAudioPIDs() ? 1 : 0;
	miscSettingsZapitSettings.addItem(new CMenuOptionChooser(LOCALE_ZAPITCONFIG_SAVE_AUDIO_PID, &saveaudiopids, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, zapitSetupNotifier));

	remainingChannelsBouquet = g_Zapit->getRemainingChannelsBouquet() ? 1 : 0;
	miscSettingsZapitSettings.addItem(new CMenuOptionChooser(LOCALE_ZAPITCONFIG_REMAINING_BOUQUET, &remainingChannelsBouquet, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, zapitSetupNotifier));
}

/* for driver and boot settings menu */
#define DRIVERSETTINGS_FB_DESTINATION_OPTION_COUNT 3
const CMenuOptionChooser::keyval DRIVERSETTINGS_FB_DESTINATION_OPTIONS[DRIVERSETTINGS_FB_DESTINATION_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_NULL   },
	{ 1, LOCALE_OPTIONS_SERIAL },
	{ 2, LOCALE_OPTIONS_FB     }
};

#define DRIVERSETTINGS_FDX_OPTION_COUNT 3
const CMenuOptionChooser::keyval DRIVERSETTINGS_FDX_OPTIONS[DRIVERSETTINGS_FDX_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF					},
	{ 1, LOCALE_OPTIONS_ON					},
	{ 2, LOCALE_DRIVERSETTINGS_FDX_FORCE	},
};

typedef struct driver_setting_files_t
{
	const neutrino_locale_t                  name;
	const char * const                       filename;
	const CMenuOptionChooser::keyval * const options;
} driver_setting_files_struct_t;

const driver_setting_files_struct_t driver_setting_files[DRIVER_SETTING_FILES_COUNT] =
{
	{LOCALE_DRIVERSETTINGS_BOOTINFO      , "/var/etc/.boot_info"     , OPTIONS_OFF0_ON1_OPTIONS },
#ifdef HAVE_DBOX_HARDWARE
#if HAVE_DVB_API_VERSION == 1
	{LOCALE_DRIVERSETTINGS_STARTBHDRIVER , "/var/etc/.bh"            , OPTIONS_OFF0_ON1_OPTIONS },
#endif
	{LOCALE_DRIVERSETTINGS_HWSECTIONS    , "/var/etc/.hw_sections"   , OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_NOAVIAWATCHDOG, "/var/etc/.no_watchdog"   , OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_NOENXWATCHDOG , "/var/etc/.no_enxwatchdog", OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_PHILIPSRCPATCH, "/var/etc/.philips_rc_patch", OPTIONS_OFF0_ON1_OPTIONS },
	{LOCALE_DRIVERSETTINGS_SPTSFIX       , "/var/etc/.sptsfix"       , OPTIONS_OFF0_ON1_OPTIONS },
#endif
	{LOCALE_DRIVERSETTINGS_PMTUPDATE     , "/var/etc/.no_pmt_update" , OPTIONS_OFF1_ON0_OPTIONS }
};

/* driver settings menu */
void CNeutrinoApp::InitDriverSettings(CMenuWidget &driverSettings)
{
	bool item_enabled[DRIVER_SETTING_FILES_COUNT];
	int boxtype = g_Controld->getBoxType();
	
	dprintf(DEBUG_DEBUG, "init driversettings\n");
	driverSettings.addItem(GenericMenuSeparator);
	driverSettings.addItem(GenericMenuBack);
	driverSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVERSETTINGS_DRIVER_BOOT));

#ifdef HAVE_DBOX_HARDWARE
	CSPTSNotifier *sptsNotifier = new CSPTSNotifier;
	driverSettings.addItem(new CMenuOptionChooser(LOCALE_DRIVERSETTINGS_SPTSMODE, &g_settings.misc_spts, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, sptsNotifier));
#endif

	for (int i = 0; i < DRIVER_SETTING_FILES_COUNT; i++)
	{
		FILE * fd = fopen(driver_setting_files[i].filename, "r");
		if (fd)
		{
			fclose(fd);
			g_settings.misc_option[i] = 1;
		}
		else
			g_settings.misc_option[i] = 0;
		
		if (!strcmp(driver_setting_files[i].filename, "/var/etc/.philips_rc_patch") && (boxtype == 1)) // usefully for Philips RC and sometimes for Sagem RC
			item_enabled[i] = false;
		else if (!strcmp(driver_setting_files[i].filename, "/var/etc/.no_enxwatchdog") && (boxtype == 1)) // not for Nokia
			item_enabled[i] = false;
		else if (!strcmp(driver_setting_files[i].filename, "/var/etc/.sptsfix") && (boxtype != 1)) // only Nokia has Avia500
			item_enabled[i] = false;
		else
			item_enabled[i] = true;

		driverSettings.addItem(new CMenuOptionChooser(driver_setting_files[i].name, &(g_settings.misc_option[i]), driver_setting_files[i].options, 2, item_enabled[i], new CTouchFileNotifier(driver_setting_files[i].filename)));
	}

#ifdef HAVE_DBOX_HARDWARE
	driverSettings.addItem(new CMenuOptionChooser(LOCALE_DRIVERSETTINGS_FB_DESTINATION, &g_settings.uboot_console, DRIVERSETTINGS_FB_DESTINATION_OPTIONS, DRIVERSETTINGS_FB_DESTINATION_OPTION_COUNT, true, ConsoleDestinationChanger));
	driverSettings.addItem(new CMenuOptionChooser(LOCALE_DRIVERSETTINGS_FDX_LOAD, &g_settings.uboot_dbox_duplex, DRIVERSETTINGS_FDX_OPTIONS, DRIVERSETTINGS_FDX_OPTION_COUNT, true, FdxSettingsChanger));
#endif
}

/* language settings menu */
void CNeutrinoApp::InitLanguageSettings(CMenuWidget &languageSettings)
{
	addMenueIntroItems(languageSettings);

	//search available languages....

	struct dirent **namelist;
	int n;
	//		printf("scanning locale dir now....(perhaps)\n");

	const char *pfad[] = {DATADIR "/neutrino/locale","/var/tuxbox/config/locale"};

	for(int p = 0;p < 2;p++)
	{
		n = scandir(pfad[p], &namelist, 0, alphasort);
		if(n < 0)
		{
			perror("loading locales: scandir");
		}
		else
		{
			for(int count=0;count<n;count++)
			{
				char * locale = strdup(namelist[count]->d_name);
				char * pos = strstr(locale, ".locale");
				if(pos != NULL)
				{
					*pos = '\0';
					CMenuOptionLanguageChooser* oj = new CMenuOptionLanguageChooser((char*)locale, this);
					oj->addOption(locale);
					languageSettings.addItem( oj );
				}
				else
					free(locale);
				free(namelist[count]);
			}
			free(namelist);
		}
	}
}


/* for parental lock settings menu */
#if 1
#define PARENTALLOCK_PROMPT_OPTION_COUNT 3
#else
#define PARENTALLOCK_PROMPT_OPTION_COUNT 4
#endif
const CMenuOptionChooser::keyval PARENTALLOCK_PROMPT_OPTIONS[PARENTALLOCK_PROMPT_OPTION_COUNT] =
{
	{ PARENTALLOCK_PROMPT_NEVER         , LOCALE_PARENTALLOCK_NEVER          },
#if 0
	{ PARENTALLOCK_PROMPT_ONSTART       , LOCALE_PARENTALLOCK_ONSTART        },
#endif
	{ PARENTALLOCK_PROMPT_CHANGETOLOCKED, LOCALE_PARENTALLOCK_CHANGETOLOCKED },
	{ PARENTALLOCK_PROMPT_ONSIGNAL      , LOCALE_PARENTALLOCK_ONSIGNAL       }
};

#define PARENTALLOCK_LOCKAGE_OPTION_COUNT 3
const CMenuOptionChooser::keyval PARENTALLOCK_LOCKAGE_OPTIONS[PARENTALLOCK_LOCKAGE_OPTION_COUNT] =
{
	{ 12, LOCALE_PARENTALLOCK_LOCKAGE12 },
	{ 16, LOCALE_PARENTALLOCK_LOCKAGE16 },
	{ 18, LOCALE_PARENTALLOCK_LOCKAGE18 }
};

/* parental lock settings menu */
void CNeutrinoApp::InitParentalLockSettings(CMenuWidget &parentallockSettings)
{
	addMenueIntroItems(parentallockSettings);

	parentallockSettings.addItem(new CMenuOptionChooser(LOCALE_PARENTALLOCK_PROMPT , &g_settings.parentallock_prompt , PARENTALLOCK_PROMPT_OPTIONS , PARENTALLOCK_PROMPT_OPTION_COUNT , !parentallocked));

	parentallockSettings.addItem(new CMenuOptionChooser(LOCALE_PARENTALLOCK_LOCKAGE, &g_settings.parentallock_lockage, PARENTALLOCK_LOCKAGE_OPTIONS, PARENTALLOCK_LOCKAGE_OPTION_COUNT, !parentallocked));

	CPINChangeWidget * pinChangeWidget = new CPINChangeWidget(LOCALE_PARENTALLOCK_CHANGEPIN, g_settings.parentallock_pincode, 4, LOCALE_PARENTALLOCK_CHANGEPIN_HINT1);
	parentallockSettings.addItem( new CMenuForwarder(LOCALE_PARENTALLOCK_CHANGEPIN, true, g_settings.parentallock_pincode, pinChangeWidget));
}

/* for network settings menu */
#define OPTIONS_NTPENABLE_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_NTPENABLE_OPTIONS[OPTIONS_NTPENABLE_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_NTP_OFF },
	{ 1, LOCALE_OPTIONS_NTP_ON }
};

/* network settings menu */
void CNeutrinoApp::InitNetworkSettings(CMenuWidget &networkSettings)
{
	CIPInput * networkSettings_NetworkIP  = new CIPInput(LOCALE_NETWORKMENU_IPADDRESS , networkConfig.address   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, MyIPChanger);
	CIPInput * networkSettings_NetMask    = new CIPInput(LOCALE_NETWORKMENU_NETMASK   , networkConfig.netmask   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CIPInput * networkSettings_Broadcast  = new CIPInput(LOCALE_NETWORKMENU_BROADCAST , networkConfig.broadcast , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CIPInput * networkSettings_Gateway    = new CIPInput(LOCALE_NETWORKMENU_GATEWAY   , networkConfig.gateway   , LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CIPInput * networkSettings_NameServer = new CIPInput(LOCALE_NETWORKMENU_NAMESERVER, networkConfig.nameserver, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);

	CSectionsdConfigNotifier* sectionsdConfigNotifier = new CSectionsdConfigNotifier;
	CStringInputSMS * networkSettings_NtpServer = new CStringInputSMS(LOCALE_NETWORKMENU_NTPSERVER, &g_settings.network_ntpserver, 30, LOCALE_NETWORKMENU_NTPSERVER_HINT1, LOCALE_NETWORKMENU_NTPSERVER_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-. ", sectionsdConfigNotifier);
	CStringInput * networkSettings_NtpRefresh = new CStringInput(LOCALE_NETWORKMENU_NTPREFRESH, &g_settings.network_ntprefresh, 3,LOCALE_NETWORKMENU_NTPREFRESH_HINT1, LOCALE_NETWORKMENU_NTPREFRESH_HINT2 , "0123456789 ", sectionsdConfigNotifier);

	CMenuForwarder *m0 = new CMenuForwarder(LOCALE_NETWORKMENU_SETUPNOW, true, NULL, this, "network", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);
	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_NETWORKMENU_IPADDRESS , networkConfig.inet_static, networkConfig.address   , networkSettings_NetworkIP );
	CMenuForwarder *m2 = new CMenuForwarder(LOCALE_NETWORKMENU_NETMASK   , networkConfig.inet_static, networkConfig.netmask   , networkSettings_NetMask   );
	CMenuForwarder *m3 = new CMenuForwarder(LOCALE_NETWORKMENU_BROADCAST , networkConfig.inet_static, networkConfig.broadcast , networkSettings_Broadcast );
	CMenuForwarder *m4 = new CMenuForwarder(LOCALE_NETWORKMENU_GATEWAY   , networkConfig.inet_static, networkConfig.gateway   , networkSettings_Gateway   );
	CMenuForwarder *m5 = new CMenuForwarder(LOCALE_NETWORKMENU_NAMESERVER, networkConfig.inet_static, networkConfig.nameserver, networkSettings_NameServer);
	CMenuForwarder *m6 = new CMenuForwarder( LOCALE_NETWORKMENU_NTPSERVER, true , g_settings.network_ntpserver, networkSettings_NtpServer );
	CMenuForwarder *m7 = new CMenuForwarder( LOCALE_NETWORKMENU_NTPREFRESH, true , g_settings.network_ntprefresh, networkSettings_NtpRefresh );

	CDHCPNotifier* dhcpNotifier = new CDHCPNotifier(m1,m2,m3,m4,m5);

	network_automatic_start = networkConfig.automatic_start ? 1 : 0;
	CMenuOptionChooser* o1 = new CMenuOptionChooser(LOCALE_NETWORKMENU_SETUPONSTARTUP, &network_automatic_start, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	network_dhcp = networkConfig.inet_static ? 0 : 1;
	CMenuOptionChooser* o2 = new CMenuOptionChooser(LOCALE_NETWORKMENU_DHCP, &network_dhcp, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, dhcpNotifier);

	/* add menu items */
	addMenueIntroItems(networkSettings);
	networkSettings.addItem( m0 );

	networkSettings.addItem(new CMenuForwarder(LOCALE_NETWORKMENU_TEST, true, NULL, this, "networktest", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	networkSettings.addItem(new CMenuForwarder(LOCALE_NETWORKMENU_SHOW, true, NULL, this, "networkshow", CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP_SMALL));
	networkSettings.addItem(GenericMenuSeparatorLine);

	networkSettings.addItem(o1);
	networkSettings.addItem(GenericMenuSeparatorLine);
	networkSettings.addItem(o2);
	networkSettings.addItem(GenericMenuSeparatorLine);

	networkSettings.addItem( m1);
	networkSettings.addItem( m2);
	networkSettings.addItem( m3);

	networkSettings.addItem(GenericMenuSeparatorLine);
	networkSettings.addItem( m4);
	networkSettings.addItem( m5);
	networkSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_NETWORKMENU_NTPTITLE));
	networkSettings.addItem(new CMenuOptionChooser(LOCALE_NETWORKMENU_NTPENABLE, &g_settings.network_ntpenable, OPTIONS_NTPENABLE_OPTIONS, OPTIONS_NTPENABLE_OPTION_COUNT, true, sectionsdConfigNotifier));
	networkSettings.addItem( m6);
	networkSettings.addItem( m7);
#ifdef ENABLE_GUI_MOUNT
	networkSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_NETWORKMENU_MOUNT));
	networkSettings.addItem(new CMenuForwarder(LOCALE_NFS_MOUNT , true, NULL, new CNFSMountGui(), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	networkSettings.addItem(new CMenuForwarder(LOCALE_NFS_UMOUNT, true, NULL, new CNFSUmountGui(), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
#endif
}

/* for record settings menu */
#define RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT 4
const CMenuOptionChooser::keyval RECORDINGMENU_RECORDING_TYPE_OPTIONS[RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT] =
{
	{ CNeutrinoApp::RECORDING_OFF   , LOCALE_RECORDINGMENU_OFF    },
	{ CNeutrinoApp::RECORDING_SERVER, LOCALE_RECORDINGMENU_SERVER },
	{ CNeutrinoApp::RECORDING_VCR   , LOCALE_RECORDINGMENU_VCR    },
	{ CNeutrinoApp::RECORDING_FILE  , LOCALE_RECORDINGMENU_FILE   }
};

#define RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT 3
const CMenuOptionChooser::keyval RECORDINGMENU_STOPSECTIONSD_OPTIONS[RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT] =
{
	{ 0, LOCALE_RECORDINGMENU_SECTIONSD_RUN     },
	{ 1, LOCALE_RECORDINGMENU_SECTIONSD_STOP    },
	{ 2, LOCALE_RECORDINGMENU_SECTIONSD_RESTART }
};

#define RECORDINGMENU_RINGBUFFER_SIZE_COUNT 5
const CMenuOptionChooser::keyval RECORDINGMENU_RINGBUFFER_SIZES[RECORDINGMENU_RINGBUFFER_SIZE_COUNT] =
{
	{ 0, LOCALE_RECORDINGMENU_RINGBUFFERS_05M },
	{ 1, LOCALE_RECORDINGMENU_RINGBUFFERS_1M },
	{ 2, LOCALE_RECORDINGMENU_RINGBUFFERS_2M },
	{ 3, LOCALE_RECORDINGMENU_RINGBUFFERS_4M },
	{ 4, LOCALE_RECORDINGMENU_RINGBUFFERS_8M }
};

/* record settings menu */
void CNeutrinoApp::InitRecordingSettings(CMenuWidget &recordingSettings)
{
	CIPInput * recordingSettings_server_ip = new CIPInput(LOCALE_RECORDINGMENU_SERVER_IP, g_settings.recording_server_ip, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CStringInput * recordingSettings_server_port = new CStringInput(LOCALE_RECORDINGMENU_SERVER_PORT, g_settings.recording_server_port, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");

	CMenuForwarder * mf1 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_IP, (g_settings.recording_type == RECORDING_SERVER), g_settings.recording_server_ip, recordingSettings_server_ip);
	CMenuForwarder * mf2 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_PORT, (g_settings.recording_type == RECORDING_SERVER), g_settings.recording_server_port, recordingSettings_server_port);

	CMACInput * recordingSettings_server_mac = new CMACInput(LOCALE_RECORDINGMENU_SERVER_MAC,  g_settings.recording_server_mac, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CMenuForwarder * mf3 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_MAC, ((g_settings.recording_type == RECORDING_SERVER) && g_settings.recording_server_wakeup==1), g_settings.recording_server_mac, recordingSettings_server_mac);

	CRecordingNotifier2 * RecordingNotifier2 = new CRecordingNotifier2(mf3);

	CMenuOptionChooser * oj2 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_SERVER_WAKEUP, &g_settings.recording_server_wakeup, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER), RecordingNotifier2);

	CMenuOptionChooser* oj3 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STOPPLAYBACK, &g_settings.recording_stopplayback, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE));

	CMenuOptionChooser* oj4 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_SECTIONSD, &g_settings.recording_stopsectionsd, RECORDINGMENU_STOPSECTIONSD_OPTIONS, RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE));

	CMenuOptionChooser* oj4b = new CMenuOptionChooser(LOCALE_RECORDINGMENU_ZAP_ON_ANNOUNCE, &g_settings.recording_zap_on_announce, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj5 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_NO_SCART, &g_settings.recording_vcr_no_scart, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_VCR));

	CMenuOptionChooser* oj12 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORD_IN_SPTS_MODE, &g_settings.recording_in_spts_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT,(g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE) );

	int rec_pre,rec_post;
	g_Timerd->getRecordingSafety(rec_pre,rec_post);
	sprintf(g_settings.record_safety_time_before, "%02d", rec_pre/60);
	sprintf(g_settings.record_safety_time_after, "%02d", rec_post/60);
	CRecordingSafetyNotifier *RecordingSafetyNotifier = new CRecordingSafetyNotifier;
	CStringInput * timerSettings_record_safety_time_before = new CStringInput(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, g_settings.record_safety_time_before, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_2,"0123456789 ", RecordingSafetyNotifier);
	CMenuForwarder *mf5 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, true, g_settings.record_safety_time_before, timerSettings_record_safety_time_before );

	CStringInput * timerSettings_record_safety_time_after = new CStringInput(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, g_settings.record_safety_time_after, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_2,"0123456789 ", RecordingSafetyNotifier);
	CMenuForwarder *mf6 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, true, g_settings.record_safety_time_after, timerSettings_record_safety_time_after );

	int zapto_pre;
	g_Timerd->getZaptoSafety(zapto_pre);
	sprintf(g_settings.zapto_safety_time_before, "%02d", zapto_pre/60);
	CZaptoSafetyNotifier *ZaptoSafetyNotifier = new CZaptoSafetyNotifier;
	CStringInput * timerSettings_zapto_safety_time_before = new CStringInput(LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE, g_settings.zapto_safety_time_before, 2, LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE_HINT_1, LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE_HINT_2,"0123456789 ", ZaptoSafetyNotifier);
	CMenuForwarder *mf14 = new CMenuForwarder(LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE, true, g_settings.zapto_safety_time_before, timerSettings_zapto_safety_time_before );

	// default recording audio pids
	CMenuWidget *apidRecordingSettings = new CMenuWidget(LOCALE_RECORDINGMENU_APIDS, "audio.raw");
	CMenuForwarder* mf13 = new CMenuForwarder(LOCALE_RECORDINGMENU_APIDS ,true, NULL, apidRecordingSettings);
	g_settings.recording_audio_pids_std = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_STD ) ? 1 : 0 ;
	g_settings.recording_audio_pids_alt = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_ALT ) ? 1 : 0 ;
	g_settings.recording_audio_pids_ac3 = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_AC3 ) ? 1 : 0 ;

	CRecAPIDSettingsNotifier * an = new CRecAPIDSettingsNotifier;
	CMenuOptionChooser* aoj1 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_STD, &g_settings.recording_audio_pids_std, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);
	CMenuOptionChooser* aoj2 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_ALT, &g_settings.recording_audio_pids_alt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);
	CMenuOptionChooser* aoj3 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_AC3, &g_settings.recording_audio_pids_ac3, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);
	apidRecordingSettings->addItem(GenericMenuSeparator);
	apidRecordingSettings->addItem(GenericMenuBack);
	apidRecordingSettings->addItem(GenericMenuSeparatorLine);
	apidRecordingSettings->addItem(aoj1);
	apidRecordingSettings->addItem(aoj2);
	apidRecordingSettings->addItem(aoj3);

	// Directory menu for direct recording
	CMenuWidget *dirMenu = new CMenuWidget(LOCALE_RECORDINGMENU_DEFDIR, NEUTRINO_ICON_RECORDING);
	dirMenu->addItem(GenericMenuSeparator);
	CDirChooser* fc1[MAX_RECORDING_DIR];
	CMenuForwarder* mffc[MAX_RECORDING_DIR];
	char temp[10];
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		fc1[i] = new CDirChooser(&g_settings.recording_dir[i]);
		snprintf(temp,10,"%d:",i);
		temp[9]=0;// terminate for sure
		mffc[i] = new CMenuForwarderNonLocalized(temp, true, g_settings.recording_dir[i],fc1[i]);
	}
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		dirMenu->addItem(mffc[i]);
	}
	dirMenu->addItem(GenericMenuSeparator);
	// for direct recording
	CMenuWidget *directRecordingSettings = new CMenuWidget(LOCALE_RECORDINGMENU_FILESETTINGS, NEUTRINO_ICON_RECORDING);
	
	CMenuForwarder* mf7 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILESETTINGS,(g_settings.recording_type == RECORDING_FILE),NULL,directRecordingSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);

	CStringInput * recordingSettings_splitsize = new CStringInput(LOCALE_RECORDINGMENU_SPLITSIZE, g_settings.recording_splitsize, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
	CMenuForwarder* mf9 = new CMenuForwarder(LOCALE_RECORDINGMENU_SPLITSIZE, true, g_settings.recording_splitsize,recordingSettings_splitsize);

	CMenuOptionChooser* oj6 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_O_SYNC, &g_settings.recording_use_o_sync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj7 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_FDATASYNC, &g_settings.recording_use_fdatasync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj8 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_VTXT_PID, &g_settings.recording_stream_vtxt_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj9 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_SUBTITLE_PID, &g_settings.recording_stream_subtitle_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj13 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RINGBUFFERS, &g_settings.recording_ringbuffers, RECORDINGMENU_RINGBUFFER_SIZES, RECORDINGMENU_RINGBUFFER_SIZE_COUNT, true);
	CMenuOptionChooser* oj10 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_CHOOSE_DIRECT_REC_DIR, &g_settings.recording_choose_direct_rec_dir, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj11 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_EPG_FOR_FILENAME, &g_settings.recording_epg_for_filename, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CStringInput * recordingSettings_filenameTemplate = new CStringInput(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, &g_settings.recording_filename_template[0], 21, LOCALE_RECORDINGMENU_FILENAME_TEMPLATE_HINT, LOCALE_IPSETUP_HINT_2, "%/-_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
	CMenuForwarder* mf11 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, true, g_settings.recording_filename_template[0],recordingSettings_filenameTemplate);

	CStringInput * recordingSettings_dirPermissions = new CStringInput(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, g_settings.recording_dir_permissions[0], 3, LOCALE_RECORDINGMENU_DIR_PERMISSIONS_HINT, LOCALE_IPSETUP_HINT_2, "01234567");
	CMenuForwarder* mf12 = new CMenuForwarder(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, true, g_settings.recording_dir_permissions[0],recordingSettings_dirPermissions);

	CRecordingNotifier *RecordingNotifier = new CRecordingNotifier(mf1,mf2,oj2,mf3,oj3,oj4,oj5,mf7,oj12);

	CMenuOptionChooser* oj1 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORDING_TYPE, &g_settings.recording_type, RECORDINGMENU_RECORDING_TYPE_OPTIONS, RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT, true, RecordingNotifier);

	addMenueIntroItems(recordingSettings);
	recordingSettings.addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_SETUPNOW, true, NULL, this, "recording", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	recordingSettings.addItem(new CMenuForwarder(LOCALE_SETTINGS_HELP, true, NULL, this, "help_recording", CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP_SMALL));
	recordingSettings.addItem(GenericMenuSeparatorLine);
	recordingSettings.addItem( oj1);
	recordingSettings.addItem(GenericMenuSeparatorLine);
	recordingSettings.addItem( mf1);
	recordingSettings.addItem( mf2);
	recordingSettings.addItem( oj2);
	recordingSettings.addItem( mf3);
	recordingSettings.addItem( oj3);
	recordingSettings.addItem( oj4);
	recordingSettings.addItem( oj4b);
	recordingSettings.addItem(GenericMenuSeparatorLine);
	recordingSettings.addItem( oj5);
	recordingSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_TIMERSETTINGS_SEPARATOR));
	recordingSettings.addItem( mf5);
	recordingSettings.addItem( mf6);
	recordingSettings.addItem( mf14);
	recordingSettings.addItem(GenericMenuSeparatorLine);
	recordingSettings.addItem(oj12);
	recordingSettings.addItem( mf13);
	recordingSettings.addItem( mf7);

	directRecordingSettings->addItem(GenericMenuSeparator);
	directRecordingSettings->addItem(GenericMenuBack);
	directRecordingSettings->addItem(GenericMenuSeparatorLine);
	directRecordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_DEFDIR, true, NULL, dirMenu));
	directRecordingSettings->addItem(GenericMenuSeparatorLine);
	directRecordingSettings->addItem(mf9);
	directRecordingSettings->addItem(oj13);
	directRecordingSettings->addItem(oj6);
	directRecordingSettings->addItem(oj7);
	directRecordingSettings->addItem(oj8);
	directRecordingSettings->addItem(oj9);
	directRecordingSettings->addItem(oj10);
	directRecordingSettings->addItem(oj11);
	directRecordingSettings->addItem(mf11);
	directRecordingSettings->addItem(mf12);

	recordingstatus = 0;
}


/* for font settings menu */
class CMenuNumberInput : public CMenuForwarder, CMenuTarget, CChangeObserver
{
private:
	CChangeObserver * observer;
	CConfigFile     * configfile;
	int32_t           defaultvalue;
	char              value[11];

protected:

	virtual const char * getOption(void)
		{
			sprintf(value, "%u", configfile->getInt32(locale_real_names[text], defaultvalue));
			return value;
		}

	virtual bool changeNotify(const neutrino_locale_t OptionName, void * Data)
		{
			configfile->setInt32(locale_real_names[text], atoi(value));
			return observer->changeNotify(OptionName, Data);
		}


public:
	CMenuNumberInput(const neutrino_locale_t Text, const int32_t DefaultValue, CChangeObserver * const _observer, CConfigFile * const _configfile) : CMenuForwarder(Text, true, NULL, this)
		{
			observer     = _observer;
			configfile   = _configfile;
			defaultvalue = DefaultValue;
		}

	int exec(CMenuTarget * parent, const std::string & action_Key)
		{
			CStringInput input(text, (char *)getOption(), 3, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ", this);
			return input.exec(parent, action_Key);
		}
};

void CNeutrinoApp::AddFontSettingItem(CMenuWidget &fontSettings, const SNeutrinoSettings::FONT_TYPES number_of_fontsize_entry)
{
	fontSettings.addItem(new CMenuNumberInput(neutrino_font[number_of_fontsize_entry].name, neutrino_font[number_of_fontsize_entry].defaultsize, fontsizenotifier, &configfile));
}


const SNeutrinoSettings::FONT_TYPES channellist_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_CHANNELLIST,
	SNeutrinoSettings::FONT_TYPE_CHANNELLIST_DESCR,
	SNeutrinoSettings::FONT_TYPE_CHANNELLIST_NUMBER,
	SNeutrinoSettings::FONT_TYPE_CHANNEL_NUM_ZAP
};

const SNeutrinoSettings::FONT_TYPES eventlist_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_TITLE,
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMLARGE,
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_ITEMSMALL,
	SNeutrinoSettings::FONT_TYPE_EVENTLIST_DATETIME,
};

const SNeutrinoSettings::FONT_TYPES infobar_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_INFOBAR_NUMBER,
	SNeutrinoSettings::FONT_TYPE_INFOBAR_CHANNAME,
	SNeutrinoSettings::FONT_TYPE_INFOBAR_INFO,
	SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL
};

const SNeutrinoSettings::FONT_TYPES epg_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_EPG_TITLE,
	SNeutrinoSettings::FONT_TYPE_EPG_INFO1,
	SNeutrinoSettings::FONT_TYPE_EPG_INFO2,
	SNeutrinoSettings::FONT_TYPE_EPG_DATE
};

const SNeutrinoSettings::FONT_TYPES gamelist_font_sizes[2] =
{
	SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMLARGE,
	SNeutrinoSettings::FONT_TYPE_GAMELIST_ITEMSMALL
};

const SNeutrinoSettings::FONT_TYPES other_font_sizes[4] =
{
	SNeutrinoSettings::FONT_TYPE_MENU,
	SNeutrinoSettings::FONT_TYPE_MENU_TITLE,
	SNeutrinoSettings::FONT_TYPE_MENU_INFO,
	SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM
};

font_sizes_groups font_sizes_groups[6] =
{
	{LOCALE_FONTMENU_CHANNELLIST, 4, channellist_font_sizes, "fontsize.dcha"},
	{LOCALE_FONTMENU_EVENTLIST  , 4, eventlist_font_sizes  , "fontsize.deve"},
	{LOCALE_FONTMENU_EPG        , 4, epg_font_sizes        , "fontsize.depg"},
	{LOCALE_FONTMENU_INFOBAR    , 4, infobar_font_sizes    , "fontsize.dinf"},
	{LOCALE_FONTMENU_GAMELIST   , 2, gamelist_font_sizes   , "fontsize.dgam"},
	{NONEXISTANT_LOCALE         , 4, other_font_sizes      , "fontsize.doth"}
};

/* font settings menu */
void CNeutrinoApp::InitFontSettings(CMenuWidget &fontSettings)
{
	addMenueIntroItems(fontSettings);
	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_MENU);
	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_MENU_TITLE);
	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_MENU_INFO);
	fontSettings.addItem(GenericMenuSeparatorLine);

	for (int i = 0; i < 5; i++)
	{
		CMenuWidget * fontSettingsSubMenu = new CMenuWidget(font_sizes_groups[i].groupname, "colors.raw");
		fontSettingsSubMenu->addItem(GenericMenuSeparator);
		fontSettingsSubMenu->addItem(GenericMenuBack);
		fontSettingsSubMenu->addItem(GenericMenuSeparatorLine);
		for (unsigned int j = 0; j < font_sizes_groups[i].count; j++)
		{
			AddFontSettingItem(*fontSettingsSubMenu, font_sizes_groups[i].content[j]);
		}
		fontSettingsSubMenu->addItem(GenericMenuSeparatorLine);
		fontSettingsSubMenu->addItem(new CMenuForwarder(LOCALE_OPTIONS_DEFAULT, true, NULL, this, font_sizes_groups[i].actionkey));

		fontSettings.addItem(new CMenuForwarder(font_sizes_groups[i].groupname, true, NULL, fontSettingsSubMenu));
	}

	AddFontSettingItem(fontSettings, SNeutrinoSettings::FONT_TYPE_FILEBROWSER_ITEM);
	fontSettings.addItem(GenericMenuSeparatorLine);
	fontSettings.addItem(new CMenuForwarder(LOCALE_OPTIONS_DEFAULT, true, NULL, this, font_sizes_groups[5].actionkey));
}

/* for color settings menu */
#define COLORMENU_CORNERSETTINGS_TYPE_OPTION_COUNT 2
const CMenuOptionChooser::keyval COLORMENU_CORNERSETTINGS_TYPE_OPTIONS[COLORMENU_CORNERSETTINGS_TYPE_OPTION_COUNT] =
{
	{ 0, LOCALE_COLORMENU_ROUNDED_CORNERS_OFF },
	{ 1, LOCALE_COLORMENU_ROUNDED_CORNERS_ON  }
};

/* color settings menu */
void CNeutrinoApp::InitColorSettings(CMenuWidget &colorSettings, CMenuWidget &fontSettings)
{
	addMenueIntroItems(colorSettings);

	CMenuWidget *colorSettings_Themes = new CMenuWidget(LOCALE_COLORTHEMEMENU_HEAD, NEUTRINO_ICON_SETTINGS);
	InitColorThemesSettings(*colorSettings_Themes);

	colorSettings.addItem( new CMenuForwarder(LOCALE_COLORMENU_THEMESELECT, true, NULL, colorSettings_Themes, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED) );
	CMenuWidget *colorSettings_menuColors = new CMenuWidget(LOCALE_COLORMENUSETUP_HEAD, NEUTRINO_ICON_SETTINGS, 400, 400);
	InitColorSettingsMenuColors(*colorSettings_menuColors);
	colorSettings.addItem( new CMenuForwarder(LOCALE_COLORMENU_MENUCOLORS, true, NULL, colorSettings_menuColors, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN) );

	CMenuWidget *colorSettings_statusbarColors = new CMenuWidget(LOCALE_COLORMENU_STATUSBAR, NEUTRINO_ICON_SETTINGS);
	InitColorSettingsStatusBarColors(*colorSettings_statusbarColors);
	colorSettings.addItem( new CMenuForwarder(LOCALE_COLORSTATUSBAR_HEAD, true, NULL, colorSettings_statusbarColors, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW) );

	colorSettings.addItem(GenericMenuSeparatorLine);
	colorSettings.addItem( new CMenuForwarder(LOCALE_COLORMENU_FONT, true, NULL, &fontSettings, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE) );
	CMenuWidget *colorSettings_timing = new CMenuWidget(LOCALE_COLORMENU_TIMING, NEUTRINO_ICON_SETTINGS);
	InitColorSettingsTiming(*colorSettings_timing);
	colorSettings.addItem(new CMenuForwarder(LOCALE_TIMING_HEAD, true, NULL, colorSettings_timing, NULL, CRCInput::RC_1));

	colorSettings.addItem(GenericMenuSeparatorLine);
#ifdef HAVE_DBOX_HARDWARE
	if ((g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) || (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM)) // eNX
	{
		CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_COLORMENU_FADE, &g_settings.widget_fade, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true );
		colorSettings.addItem(oj);
	}
	else // GTX, ...
	{
		CAlphaSetup* chAlphaSetup = new CAlphaSetup(LOCALE_COLORMENU_GTX_ALPHA, &g_settings.gtx_alpha1, &g_settings.gtx_alpha2);
		colorSettings.addItem( new CMenuForwarder(LOCALE_COLORMENU_GTX_ALPHA, true, NULL, chAlphaSetup, NULL, CRCInput::RC_2));
	}
#else // dream and TD
	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_COLORMENU_FADE, &g_settings.widget_fade, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true );
	colorSettings.addItem(oj);
#endif
	CMenuOptionChooser* ojc = new CMenuOptionChooser(LOCALE_COLORMENU_ROUNDED_CORNERS, &g_settings.rounded_corners, COLORMENU_CORNERSETTINGS_TYPE_OPTIONS, COLORMENU_CORNERSETTINGS_TYPE_OPTION_COUNT, true );
	colorSettings.addItem(ojc);
}

/* theme settings menu */
void CNeutrinoApp::InitColorThemesSettings(CMenuWidget &colorSettings_Themes)
{
	addMenueIntroItems(colorSettings_Themes);
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_NEUTRINO_THEME, true, NULL, this, "theme_neutrino", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

	CThemes* cthemes = new CThemes();
	colorSettings_Themes.addItem( new CMenuForwarder(LOCALE_COLORTHEMEMENU_HEAD2, true, NULL, cthemes, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));

}

/* color chooser menu */
/* menu colors */
void CNeutrinoApp::InitColorSettingsMenuColors(CMenuWidget &colorSettings_menuColors)
{
	colorSettings_menuColors.addItem(GenericMenuSeparator);
	colorSettings_menuColors.addItem(GenericMenuBack);

	CColorChooser* chHeadcolor = new CColorChooser(LOCALE_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue,
																  &g_settings.menu_Head_alpha, colorSetupNotifier);
	CColorChooser* chHeadTextcolor = new CColorChooser(LOCALE_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Head_Text_red, &g_settings.menu_Head_Text_green, &g_settings.menu_Head_Text_blue,
																		NULL, colorSetupNotifier);
	CColorChooser* chContentcolor = new CColorChooser(LOCALE_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Content_red, &g_settings.menu_Content_green, &g_settings.menu_Content_blue,
																	  &g_settings.menu_Content_alpha, colorSetupNotifier);
	CColorChooser* chContentTextcolor = new CColorChooser(LOCALE_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Content_Text_red, &g_settings.menu_Content_Text_green, &g_settings.menu_Content_Text_blue,
																			NULL, colorSetupNotifier);
	CColorChooser* chContentSelectedcolor = new CColorChooser(LOCALE_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Content_Selected_red, &g_settings.menu_Content_Selected_green, &g_settings.menu_Content_Selected_blue,
																				 &g_settings.menu_Content_Selected_alpha, colorSetupNotifier);
	CColorChooser* chContentSelectedTextcolor = new CColorChooser(LOCALE_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Content_Selected_Text_red, &g_settings.menu_Content_Selected_Text_green, &g_settings.menu_Content_Selected_Text_blue,
																					  NULL, colorSetupNotifier);
	CColorChooser* chContentInactivecolor = new CColorChooser(LOCALE_COLORMENU_BACKGROUND_HEAD, &g_settings.menu_Content_inactive_red, &g_settings.menu_Content_inactive_green, &g_settings.menu_Content_inactive_blue,
																				 &g_settings.menu_Content_inactive_alpha, colorSetupNotifier);
	CColorChooser* chContentInactiveTextcolor = new CColorChooser(LOCALE_COLORMENU_TEXTCOLOR_HEAD, &g_settings.menu_Content_inactive_Text_red, &g_settings.menu_Content_inactive_Text_green, &g_settings.menu_Content_inactive_Text_blue,
																					  NULL, colorSetupNotifier);
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORMENUSETUP_MENUHEAD));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_BACKGROUND, true, NULL, chHeadcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_TEXTCOLOR, true, NULL, chHeadTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORMENUSETUP_MENUCONTENT));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_BACKGROUND, true, NULL, chContentcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_TEXTCOLOR, true, NULL, chContentTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORMENUSETUP_MENUCONTENT_INACTIVE));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_BACKGROUND, true, NULL, chContentInactivecolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_TEXTCOLOR, true, NULL, chContentInactiveTextcolor));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORMENUSETUP_MENUCONTENT_SELECTED));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_BACKGROUND, true, NULL, chContentSelectedcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_TEXTCOLOR, true, NULL, chContentSelectedTextcolor ));
}

/* infobar colors */
void CNeutrinoApp::InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_statusbarColors)
{
	colorSettings_statusbarColors.addItem(GenericMenuSeparator);

	colorSettings_statusbarColors.addItem(GenericMenuBack);

	CColorChooser* chInfobarcolor = new CColorChooser(LOCALE_COLORMENU_BACKGROUND_HEAD, &g_settings.infobar_red, &g_settings.infobar_green, &g_settings.infobar_blue,
																	  &g_settings.infobar_alpha, colorSetupNotifier);
	CColorChooser* chInfobarTextcolor = new CColorChooser(LOCALE_COLORMENU_TEXTCOLOR_HEAD, &g_settings.infobar_Text_red, &g_settings.infobar_Text_green, &g_settings.infobar_Text_blue,
																			NULL, colorSetupNotifier);

	colorSettings_statusbarColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_COLORSTATUSBAR_TEXT));
	colorSettings_statusbarColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_BACKGROUND, true, NULL, chInfobarcolor ));
	colorSettings_statusbarColors.addItem( new CMenuForwarder(LOCALE_COLORMENU_TEXTCOLOR, true, NULL, chInfobarTextcolor ));
}

/* OSD timeouts */
void CNeutrinoApp::InitColorSettingsTiming(CMenuWidget &colorSettings_timing)
{
	/* note: SetupTiming() is already called in CNeutrinoApp::run */

	addMenueIntroItems(colorSettings_timing);

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		CStringInput * colorSettings_timing_item = new CStringInput(timing_setting_name[i], g_settings.timing_string[i], 3, LOCALE_TIMING_HINT_1, LOCALE_TIMING_HINT_2, "0123456789 ", &timingsettingsnotifier);
		colorSettings_timing.addItem(new CMenuForwarder(timing_setting_name[i], true, g_settings.timing_string[i], colorSettings_timing_item));
	}

	colorSettings_timing.addItem(GenericMenuSeparatorLine);
	colorSettings_timing.addItem(new CMenuForwarder(LOCALE_OPTIONS_DEFAULT, true, NULL, this, "osd.def"));
}

/* for lcd settings menu*/
#define LCDMENU_STATUSLINE_OPTION_COUNT 4
const CMenuOptionChooser::keyval LCDMENU_STATUSLINE_OPTIONS[LCDMENU_STATUSLINE_OPTION_COUNT] =
{
	{ 0, LOCALE_LCDMENU_STATUSLINE_PLAYTIME   },
	{ 1, LOCALE_LCDMENU_STATUSLINE_VOLUME     },
	{ 2, LOCALE_LCDMENU_STATUSLINE_BOTH       },
	{ 3, LOCALE_LCDMENU_STATUSLINE_BOTH_AUDIO }
};

/* for lcd EPG menu*/
#define LCDMENU_EPG_OPTION_COUNT 6
const CMenuOptionChooser::keyval LCDMENU_EPG_OPTIONS[LCDMENU_EPG_OPTION_COUNT] =
{
	{ 1, LOCALE_LCDMENU_EPG_NAME		},
	{ 2, LOCALE_LCDMENU_EPG_TITLE		},
	{ 3, LOCALE_LCDMENU_EPG_NAME_TITLE	},
	{ 7, LOCALE_LCDMENU_EPG_NAME_SEPLINE_TITLE },
	{ 11, LOCALE_LCDMENU_EPG_NAMESHORT_TITLE },
	{ 15, LOCALE_LCDMENU_EPG_NAMESHORT_SEPLINE_TITLE }
};
  
/* lcd settings menu*/
void CNeutrinoApp::InitLcdSettings(CMenuWidget &lcdSettings)
{
	addMenueIntroItems(lcdSettings);

	CLcdControler* lcdsliders = new CLcdControler(LOCALE_LCDMENU_HEAD, NULL);

	CLcdNotifier* lcdnotifier = new CLcdNotifier();

	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_LCDMENU_INVERSE, &g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier);
	lcdSettings.addItem(oj);

	if (g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) {
		oj = new CMenuOptionChooser(LOCALE_LCDMENU_BIAS, &g_settings.lcd_setting[SNeutrinoSettings::LCD_BIAS], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier);
		lcdSettings.addItem(oj);
	}

	oj = new CMenuOptionChooser(LOCALE_LCDMENU_POWER, &g_settings.lcd_setting[SNeutrinoSettings::LCD_POWER], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier);
	lcdSettings.addItem(oj);

	if ((g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) || (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM))
	{
		// Autodimm available on Sagem/Philips only
		oj = new CMenuOptionChooser(LOCALE_LCDMENU_AUTODIMM, &g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier);
		lcdSettings.addItem( oj );
	}

	CStringInput * dim_time = new CStringInput(LOCALE_LCDMENU_DIM_TIME, g_settings.lcd_setting_dim_time, 3,
							NONEXISTANT_LOCALE, NONEXISTANT_LOCALE,"0123456789 ");
	lcdSettings.addItem(new CMenuForwarder(LOCALE_LCDMENU_DIM_TIME,true, g_settings.lcd_setting_dim_time,dim_time));

	CStringInput * dim_brightness = new CStringInput(LOCALE_LCDMENU_DIM_BRIGHTNESS, g_settings.lcd_setting_dim_brightness, 3,
							NONEXISTANT_LOCALE, NONEXISTANT_LOCALE,"0123456789 ");
	lcdSettings.addItem(new CMenuForwarder(LOCALE_LCDMENU_DIM_BRIGHTNESS,true, g_settings.lcd_setting_dim_brightness,dim_brightness));

	lcdSettings.addItem(GenericMenuSeparatorLine);
	lcdSettings.addItem(new CMenuForwarder(LOCALE_LCDMENU_LCDCONTROLER, true, NULL, lcdsliders));

	lcdSettings.addItem(GenericMenuSeparatorLine);
	oj = new CMenuOptionChooser(LOCALE_LCDMENU_STATUSLINE, &g_settings.lcd_setting[SNeutrinoSettings::LCD_SHOW_VOLUME], LCDMENU_STATUSLINE_OPTIONS, LCDMENU_STATUSLINE_OPTION_COUNT, true);
	lcdSettings.addItem(oj);
	
	//lcd_epg
	lcdSettings.addItem(GenericMenuSeparatorLine);
	oj = new CMenuOptionChooser(LOCALE_LCDMENU_EPG, &g_settings.lcd_setting[SNeutrinoSettings::LCD_EPGMODE], LCDMENU_EPG_OPTIONS, LCDMENU_EPG_OPTION_COUNT, true);
	lcdSettings.addItem(oj);
}
 
/* for keybindings settings menu*/
#define KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT 3
const CMenuOptionChooser::keyval KEYBINDINGMENU_BOUQUETHANDLING_OPTIONS[KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT] =
{
	{ 0, LOCALE_KEYBINDINGMENU_BOUQUETCHANNELS_ON_OK },
	{ 1, LOCALE_KEYBINDINGMENU_BOUQUETLIST_ON_OK     },
	{ 2, LOCALE_KEYBINDINGMENU_ALLCHANNELS_ON_OK     }
};

enum keynames {
	VIRTUALKEY_TV_RADIO_MODE,
	VIRTUALKEY_PAGE_UP,
	VIRTUALKEY_PAGE_DOWN,
	VIRTUALKEY_CANCEL_ACTION,
	VIRTUALKEY_SORT,
	VIRTUALKEY_SEARCH,
	VIRTUALKEY_ADD_RECORD,
	VIRTUALKEY_ADD_REMIND,
	VIRTUALKEY_RELOAD,
	VIRTUALKEY_CHANNEL_UP,
	VIRTUALKEY_CHANNEL_DOWN,
	VIRTUALKEY_BOUQUET_UP,
	VIRTUALKEY_BOUQUET_DOWN,
	VIRTUALKEY_SUBCHANNEL_UP,
	VIRTUALKEY_SUBCHANNEL_DOWN,
	VIRTUALKEY_SUBCHANNEL_TOGGLE,
	VIRTUALKEY_ZAP_HISTORY,
	VIRTUALKEY_LASTCHANNEL,

	MAX_NUM_KEYNAMES
};

const neutrino_locale_t keydescription_head[] =
{
	LOCALE_KEYBINDINGMENU_TVRADIOMODE_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEUP_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_CANCEL_HEAD,
	LOCALE_KEYBINDINGMENU_SORT_HEAD,
	LOCALE_EVENTFINDER_HEAD,
	LOCALE_KEYBINDINGMENU_ADDRECORD_HEAD,
	LOCALE_KEYBINDINGMENU_ADDREMIND_HEAD,
	LOCALE_KEYBINDINGMENU_RELOAD_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETUP_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELTOGGLE_HEAD,
	LOCALE_KEYBINDINGMENU_ZAPHISTORY_HEAD,
	LOCALE_KEYBINDINGMENU_LASTCHANNEL_HEAD
};

const neutrino_locale_t keydescription[] =
{
	LOCALE_KEYBINDINGMENU_TVRADIOMODE,
	LOCALE_KEYBINDINGMENU_PAGEUP,
	LOCALE_KEYBINDINGMENU_PAGEDOWN,
	LOCALE_KEYBINDINGMENU_CANCEL,
	LOCALE_KEYBINDINGMENU_SORT,
	LOCALE_EVENTFINDER_HEAD,
	LOCALE_KEYBINDINGMENU_ADDRECORD,
	LOCALE_KEYBINDINGMENU_ADDREMIND,
	LOCALE_KEYBINDINGMENU_RELOAD,
	LOCALE_KEYBINDINGMENU_CHANNELUP,
	LOCALE_KEYBINDINGMENU_CHANNELDOWN,
	LOCALE_KEYBINDINGMENU_BOUQUETUP,
	LOCALE_KEYBINDINGMENU_BOUQUETDOWN,
	LOCALE_KEYBINDINGMENU_SUBCHANNELUP,
	LOCALE_KEYBINDINGMENU_SUBCHANNELDOWN,
	LOCALE_KEYBINDINGMENU_SUBCHANNELTOGGLE,
	LOCALE_KEYBINDINGMENU_ZAPHISTORY,
	LOCALE_KEYBINDINGMENU_LASTCHANNEL
};

/* keybindings settings menu*/
void CNeutrinoApp::InitKeySettings(CMenuWidget &keySettings)
{
	keySettings.addItem(GenericMenuSeparator);
	keySettings.addItem(GenericMenuBack);
	// USERMENU
	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_USERMENU_HEAD));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_RED, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_RED,0), NULL, CRCInput::RC_red,NEUTRINO_ICON_BUTTON_RED));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_GREEN, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_GREEN,1), NULL, CRCInput::RC_green,NEUTRINO_ICON_BUTTON_GREEN));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_YELLOW, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_YELLOW,2), NULL, CRCInput::RC_yellow,NEUTRINO_ICON_BUTTON_YELLOW));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_BLUE, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_BLUE,3), NULL, CRCInput::RC_blue,NEUTRINO_ICON_BUTTON_BLUE));

	neutrino_msg_t * keyvalue_p[] =
		{
			&g_settings.key_tvradio_mode,
			&g_settings.key_channelList_pageup,
			&g_settings.key_channelList_pagedown,
			&g_settings.key_channelList_cancel,
			&g_settings.key_channelList_sort,
			&g_settings.key_channelList_search,
			&g_settings.key_channelList_addrecord,
			&g_settings.key_channelList_addremind,
			&g_settings.key_channelList_reload,
			&g_settings.key_quickzap_up,
			&g_settings.key_quickzap_down,
			&g_settings.key_bouquet_up,
			&g_settings.key_bouquet_down,
			&g_settings.key_subchannel_up,
			&g_settings.key_subchannel_down,
			&g_settings.key_subchannel_toggle,
			&g_settings.key_zaphistory,
			&g_settings.key_lastchannel
		};

	CKeyChooser * keychooser[MAX_NUM_KEYNAMES];

	for (int i = 0; i < MAX_NUM_KEYNAMES; i++)
		keychooser[i] = new CKeyChooser(keyvalue_p[i], keydescription_head[i], NEUTRINO_ICON_SETTINGS);

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_MODECHANGE));
	keySettings.addItem(new CMenuForwarder(keydescription[VIRTUALKEY_TV_RADIO_MODE], true, NULL, keychooser[VIRTUALKEY_TV_RADIO_MODE]));

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_CHANNELLIST));

	CMenuOptionChooser *oj = new CMenuOptionChooser(LOCALE_KEYBINDINGMENU_BOUQUETHANDLING, &g_settings.bouquetlist_mode, KEYBINDINGMENU_BOUQUETHANDLING_OPTIONS, KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT, true );
	keySettings.addItem(oj);

	for (int i = VIRTUALKEY_PAGE_UP; i <= VIRTUALKEY_RELOAD; i++)
		keySettings.addItem(new CMenuForwarder(keydescription[i], true, NULL, keychooser[i]));

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_QUICKZAP));

	for (int i = VIRTUALKEY_CHANNEL_UP; i <= VIRTUALKEY_SUBCHANNEL_DOWN; i++)
		keySettings.addItem(new CMenuForwarder(keydescription[i], true, NULL, keychooser[i]));

	keySettings.addItem(new CMenuForwarder(keydescription[VIRTUALKEY_SUBCHANNEL_TOGGLE], true, NULL, keychooser[VIRTUALKEY_SUBCHANNEL_TOGGLE]));

	keySettings.addItem(new CMenuForwarder(keydescription[VIRTUALKEY_ZAP_HISTORY], true, NULL, keychooser[VIRTUALKEY_ZAP_HISTORY]));

	keySettings.addItem(new CMenuForwarder(keydescription[VIRTUALKEY_LASTCHANNEL], true, NULL, keychooser[VIRTUALKEY_LASTCHANNEL]));
}

#define MAINMENU_RECORDING_OPTION_COUNT 2
const CMenuOptionChooser::keyval MAINMENU_RECORDING_OPTIONS[MAINMENU_RECORDING_OPTION_COUNT] =
{
	{ 0, LOCALE_MAINMENU_RECORDING_START },
	{ 1, LOCALE_MAINMENU_RECORDING_STOP  }
};


// USERMENU
bool CNeutrinoApp::showUserMenu(int button)
{
	if(button < 0 || button >= SNeutrinoSettings::BUTTON_MAX) 
		return false;
		
	CMenuItem* menu_item = NULL;
	CKeyHelper keyhelper;
	neutrino_msg_t key = CRCInput::RC_nokey;
	const char * icon = NULL; 
	
	char id[5];
	int menu_items = 0;
	int menu_prev = -1;
	int cnt = 0;

	// define classes
	CFavorites* tmpFavorites				= NULL;
	CPauseSectionsdNotifier* tmpPauseSectionsdNotifier 	= NULL;
	CAudioSelectMenuHandler* tmpAudioSelectMenuHandler 	= NULL;
	CMenuWidget* tmpNVODSelector				= NULL;
	CStreamInfo2Handler*	tmpStreamInfo2Handler 		= NULL;
	CEventListHandler* tmpEventListHandler			= NULL;
	CEPGplusHandler* tmpEPGplusHandler			= NULL;
	CEPGDataHandler* tmpEPGDataHandler			= NULL;
	
	std::string txt = g_settings.usermenu_text[button];
	if (button == SNeutrinoSettings::BUTTON_RED)
	{
		if( txt.empty() )
			txt = g_Locale->getText(LOCALE_INFOVIEWER_EVENTLIST);
	}
	else if( button == SNeutrinoSettings::BUTTON_GREEN)
	{
		if( txt.empty() )
			txt = g_Locale->getText(LOCALE_INFOVIEWER_LANGUAGES);
	}
	else if( button == SNeutrinoSettings::BUTTON_YELLOW)
	{
		if( txt.empty() ) 
			txt = g_Locale->getText((g_RemoteControl->are_subchannels) ? LOCALE_INFOVIEWER_SUBSERVICE : LOCALE_INFOVIEWER_SELECTTIME);
			//txt = g_Locale->getText(LOCALE_NVODSELECTOR_DIRECTORMODE);
	}
	else if( button == SNeutrinoSettings::BUTTON_BLUE)
	{	
		if( txt.empty() )
			txt = g_Locale->getText(LOCALE_INFOVIEWER_STREAMINFO);
	}
	CMenuWidget *menu = new CMenuWidget(txt.c_str() , "features.raw", 400);
	if (menu == NULL) 
		return 0;
	menu->addItem(GenericMenuSeparator);
	
	// go through any postition number
	for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX ; pos++)
	{
		int dummy;
		// now compare pos with the position of any item. Add this item if position is the same 
		switch(g_settings.usermenu[button][pos])
		{
			case SNeutrinoSettings::ITEM_NONE: 
				// do nothing 
				break;

			case SNeutrinoSettings::ITEM_BAR: 
				if(menu_prev == -1 && menu_prev == SNeutrinoSettings::ITEM_BAR )
					break;
					
				menu->addItem(GenericMenuSeparatorLine);
				menu_prev = SNeutrinoSettings::ITEM_BAR;
				break;

			case SNeutrinoSettings::ITEM_VTXT:
				for(unsigned int count = 0; count < (unsigned int) g_PluginList->getNumberOfPlugins(); count++)
				{
					std::string tmp = g_PluginList->getName(count);
					if (g_PluginList->getType(count)== CPlugins::P_TYPE_TOOL && !g_PluginList->isHidden(count) && tmp.find("Teletext") != std::string::npos)
					{
						sprintf(id, "%d", count);
						menu_items++;
						menu_prev = SNeutrinoSettings::ITEM_VTXT;
						keyhelper.get(&key, &icon, cnt == 0 ? CRCInput::RC_blue : CRCInput::RC_nokey);
						menu_item = new CMenuForwarderNonLocalized(g_PluginList->getName(count), true, NULL, StreamFeaturesChanger, id, key, icon);
						menu->addItem(menu_item, (cnt == 0));
						cnt++;
					}
				}
				break;

			case SNeutrinoSettings::ITEM_PLUGIN:
				for(unsigned int count = 0; count < (unsigned int) g_PluginList->getNumberOfPlugins(); count++)
				{
					std::string tmp = g_PluginList->getName(count);
					if (g_PluginList->getType(count)== CPlugins::P_TYPE_TOOL && !g_PluginList->isHidden(count) && tmp.find("Teletext") == std::string::npos)
					{
						sprintf(id, "%d", count);
						menu_items++;
						menu_prev = SNeutrinoSettings::ITEM_PLUGIN;
						keyhelper.get(&key, &icon, cnt == 0 ? CRCInput::RC_blue : CRCInput::RC_nokey);
						menu_item = new CMenuForwarderNonLocalized(g_PluginList->getName(count), true, NULL, StreamFeaturesChanger, id, key, icon);
						menu->addItem(menu_item, (cnt == 0));
						cnt++;
					}
				}
				break;
				
			case SNeutrinoSettings::ITEM_FAVORITS: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_FAVORITS;
				tmpFavorites = new CFavorites;
				keyhelper.get(&key,&icon);
				menu_item = new CMenuForwarder(LOCALE_FAVORITES_MENUEADD, true, NULL, tmpFavorites, "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_RECORD: 
				if(g_settings.recording_type == RECORDING_OFF) 
					break;
				
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_RECORD;
				keyhelper.get(&key,&icon,CRCInput::RC_red);
				menu_item = new CMenuOptionChooser(LOCALE_MAINMENU_RECORDING, &recordingstatus, MAINMENU_RECORDING_OPTIONS, MAINMENU_RECORDING_OPTION_COUNT, true, this, key, icon);
				menu->addItem(menu_item, false);
				break;

#ifdef ENABLE_MOVIEPLAYER
			case SNeutrinoSettings::ITEM_MOVIEPLAYER_TS: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_MOVIEPLAYER_TS;
				keyhelper.get(&key,&icon,CRCInput::RC_green);
				menu_item = new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, this->moviePlayerGui, "tsplayback", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_MOVIEPLAYER_MB: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_MOVIEPLAYER_MB;
				keyhelper.get(&key,&icon,CRCInput::RC_green);
#ifndef ENABLE_MOVIEPLAYER2
				menu_item = new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, this->moviePlayerGui, "tsmoviebrowser", key, icon);
#else
				menu_item = new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, movieBrowser, "run", key, icon);
#endif
				menu->addItem(menu_item, false);
				break;
#endif

			case SNeutrinoSettings::ITEM_TIMERLIST: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_TIMERLIST;
				keyhelper.get(&key,&icon,CRCInput::RC_yellow);
				menu_item = new CMenuForwarder(LOCALE_TIMERLIST_NAME, true, NULL, Timerlist, "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_REMOTE: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_REMOTE;
				keyhelper.get(&key,&icon);
				menu_item = new CMenuForwarder(LOCALE_RCLOCK_MENUEADD, true, NULL, this->rcLock, "-1" , key, icon );
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_EPG_SUPER: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_SUPER;
				tmpEPGplusHandler = new CEPGplusHandler();
				keyhelper.get(&key,&icon,CRCInput::RC_green);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_EPGPLUS   , true, NULL, tmpEPGplusHandler  ,  "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_EPG_LIST: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_LIST;
				tmpEventListHandler = new CEventListHandler();
				keyhelper.get(&key,&icon,CRCInput::RC_red);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_EVENTLIST , true, NULL, tmpEventListHandler,  "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_EPG_INFO: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_INFO;
				tmpEPGDataHandler = new CEPGDataHandler();
				keyhelper.get(&key,&icon,CRCInput::RC_yellow);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_EVENTINFO , true, NULL, tmpEPGDataHandler ,  "-1", key, icon);
				menu->addItem(menu_item, false);
				break;

			case SNeutrinoSettings::ITEM_EPG_MISC: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_EPG_MISC;
				dummy = g_Sectionsd->getIsScanningActive();
				tmpPauseSectionsdNotifier = new CPauseSectionsdNotifier;
				keyhelper.get(&key,&icon);
				menu_item = new CMenuOptionChooser(LOCALE_MAINMENU_PAUSESECTIONSD, &dummy, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, tmpPauseSectionsdNotifier , key, icon );
				menu->addItem(menu_item, false);
				menu_items++;
				keyhelper.get(&key,&icon);
				menu_item = new CMenuForwarder(LOCALE_MAINMENU_CLEARSECTIONSD, true, NULL, this, "clearSectionsd", key,icon);
				menu->addItem(menu_item, false);
				break;
				
			case SNeutrinoSettings::ITEM_AUDIO_SELECT: 
				// -- new Audio Selector Menu (rasc 2005-08-30)
				if (g_settings.audio_left_right_selectable ||
				    g_RemoteControl->current_PIDs.APIDs.size() > 1)
				{
					menu_items++;
					menu_prev = SNeutrinoSettings::ITEM_AUDIO_SELECT;
					tmpAudioSelectMenuHandler = new CAudioSelectMenuHandler;
					keyhelper.get(&key,&icon);
					menu_item = new CMenuForwarder(LOCALE_AUDIOSELECTMENUE_HEAD, true, NULL, tmpAudioSelectMenuHandler, "-1", key,icon);
					menu->addItem(menu_item, false);
				}
				break;
				
			case SNeutrinoSettings::ITEM_SUBCHANNEL: 
				if (!(g_RemoteControl->subChannels.empty()))
				{
					// NVOD/SubService- Kanal!
					tmpNVODSelector = new CMenuWidget(g_RemoteControl->are_subchannels ? LOCALE_NVODSELECTOR_SUBSERVICE : LOCALE_NVODSELECTOR_HEAD, "video.raw", 350);
					if(getNVODMenu(tmpNVODSelector))
					{
						menu_items++;
						menu_prev = SNeutrinoSettings::ITEM_SUBCHANNEL;
						keyhelper.get(&key,&icon);
						menu_item = new CMenuForwarder(g_RemoteControl->are_subchannels ? LOCALE_NVODSELECTOR_SUBSERVICE : LOCALE_NVODSELECTOR_HEAD, true, NULL, tmpNVODSelector, "-1", key,icon);
						menu->addItem(menu_item, false);
					}
				}
				break;
				
			case SNeutrinoSettings::ITEM_TECHINFO: 
				menu_items++;
				menu_prev = SNeutrinoSettings::ITEM_TECHINFO;
				tmpStreamInfo2Handler = new CStreamInfo2Handler();
				keyhelper.get(&key,&icon,CRCInput::RC_blue);
				menu_item = new CMenuForwarder(LOCALE_EPGMENU_STREAMINFO, true, NULL, tmpStreamInfo2Handler    , "-1", key, icon );
				menu->addItem(menu_item, false);
				break;

			default: 
				printf("[neutrino] WARNING! menu wrong item!!\n");
				break;
		}
	} 
	
	// Allow some tailoring for privat image bakers ;)
	if (button == SNeutrinoSettings::BUTTON_RED)
	{
	}
	else if( button == SNeutrinoSettings::BUTTON_GREEN)
	{
	}
	else if( button == SNeutrinoSettings::BUTTON_YELLOW)
	{
	}
	else if( button == SNeutrinoSettings::BUTTON_BLUE)
	{	
#ifdef _EXPERIMENTAL_SETTINGS_
		//Experimental Settings
		if(menu_prev != -1)
			menu->addItem(GenericMenuSeparatorLine);
		menu_items ++;
		menu_key++;
		// FYI: there is a memory leak with 'new CExperimentalSettingsMenuHandler()
		menu_item = new CMenuForwarder(LOCALE_EXPERIMENTALSETTINGS, true, NULL, new CExperimentalSettingsMenuHandler(), "-1", CRCInput::convertDigitToKey(menu_key), "");
		menu->addItem(menu_item, false);
#endif
	}
	
	if(menu_items >	1 ) 				// show menu if there are more than 2 items only
	{
		menu->exec(NULL,"");
	}
	else if (menu_item != NULL)			// otherwise, we start the item directly (must be the last one)
	{
		menu_item->exec( NULL );
	}									// neither nor, we do nothing

	// restore mute symbol
	//AudioMute(current_muted, true);
	
	// clear the heap
	if(menu)			delete menu;
	if(tmpFavorites)		delete tmpFavorites;
	if(tmpPauseSectionsdNotifier)	delete tmpPauseSectionsdNotifier;
	if(tmpAudioSelectMenuHandler)	delete tmpAudioSelectMenuHandler;
	if(tmpNVODSelector)		delete tmpNVODSelector;
	if(tmpStreamInfo2Handler)	delete tmpStreamInfo2Handler;
	if(tmpEventListHandler)		delete tmpEventListHandler;
	if(tmpEPGplusHandler)		delete tmpEPGplusHandler;
	if(tmpEPGDataHandler)		delete tmpEPGDataHandler;
	return true;
}

// should be remove, now just for reference
void CNeutrinoApp::ShowStreamFeatures()
{
	char id[5];
	int cnt = 0;
	int enabled_count = 0;
	CMenuWidget *StreamFeatureSelector = new CMenuWidget(LOCALE_STREAMFEATURES_HEAD, "features.raw", 350);
	if (StreamFeatureSelector == NULL) return;

	StreamFeatureSelector->addItem(GenericMenuSeparator);

	for(unsigned int count=0;count < (unsigned int) g_PluginList->getNumberOfPlugins();count++)
	{
		if (g_PluginList->getType(count)== CPlugins::P_TYPE_TOOL && !g_PluginList->isHidden(count))
		{
			// zB vtxt-plugins

			sprintf(id, "%d", count);

			enabled_count++;

			StreamFeatureSelector->addItem(new CMenuForwarderNonLocalized(g_PluginList->getName(count), true, NULL, StreamFeaturesChanger, id, (cnt== 0) ? CRCInput::RC_blue : CRCInput::convertDigitToKey(enabled_count-1), (cnt == 0) ? NEUTRINO_ICON_BUTTON_BLUE : ""), (cnt == 0));
			cnt++;
		}
	}

	if(cnt>0)
	{
		StreamFeatureSelector->addItem(GenericMenuSeparatorLine);
	}

	sprintf(id, "%d", -1);

	// -- Add Channel to favorites
//	StreamFeatureSelector.addItem(new CMenuForwarder(LOCALE_FAVORITES_MENUEADD, true, NULL, new CFavorites, id, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), false);
	CFavorites *tmpFavorites = new CFavorites;
	StreamFeatureSelector->addItem(new CMenuForwarder(LOCALE_FAVORITES_MENUEADD, true, NULL, tmpFavorites, id, CRCInput::convertDigitToKey(enabled_count), ""), false);

	StreamFeatureSelector->addItem(GenericMenuSeparatorLine);

	// start/stop recording
	if (g_settings.recording_type != RECORDING_OFF)
	{
		StreamFeatureSelector->addItem(new CMenuOptionChooser(LOCALE_MAINMENU_RECORDING, &recordingstatus, MAINMENU_RECORDING_OPTIONS, MAINMENU_RECORDING_OPTION_COUNT, true, this, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	}

#ifdef ENABLE_MOVIEPLAYER
	// -- Add TS Playback to blue button
	StreamFeatureSelector->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, this->moviePlayerGui, "tsplayback", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), false);
#endif

	// -- Timer-Liste
	CTimerList *tmpTimerlist = new CTimerList;
	StreamFeatureSelector->addItem(new CMenuForwarder(LOCALE_TIMERLIST_NAME, true, NULL, tmpTimerlist, id, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);

	StreamFeatureSelector->addItem(GenericMenuSeparatorLine);

	// --  Lock remote control
	StreamFeatureSelector->addItem(new CMenuForwarder(LOCALE_RCLOCK_MENUEADD, true, NULL, this->rcLock, id, CRCInput::RC_nokey, ""), false);

	// -- Sectionsd pause
	int dummy = g_Sectionsd->getIsScanningActive();
	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_MAINMENU_PAUSESECTIONSD, &dummy, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, new CPauseSectionsdNotifier );
	StreamFeatureSelector->addItem(oj);

#ifdef _EXPERIMENTAL_SETTINGS_
	//Experimental Settings
	StreamFeatureSelector->addItem(new CMenuForwarder(LOCALE_EXPERIMENTALSETTINGS, true, NULL, new CExperimentalSettingsMenuHandler(), id, CRCInput::RC_nokey, ""), false);
#endif

	StreamFeatureSelector->exec(NULL,"");

	// restore mute symbol
	delete StreamFeatureSelector;
	delete tmpFavorites;
	delete tmpTimerlist;
}


// USERMENU
// leave this functions, somebody might want to use it in the future again
void CNeutrinoApp::SelectNVOD()
{
	if (!(g_RemoteControl->subChannels.empty()))
	{
		// NVOD/SubService- Kanal!
		CMenuWidget NVODSelector(g_RemoteControl->are_subchannels ? LOCALE_NVODSELECTOR_SUBSERVICE : LOCALE_NVODSELECTOR_HEAD, "video.raw", 350);
		if(getNVODMenu(&NVODSelector))
			NVODSelector.exec(NULL, "");
	}
}


bool CNeutrinoApp::getNVODMenu(CMenuWidget* menu)
{
	if(menu == NULL) 
		return false;
	if (g_RemoteControl->subChannels.empty())
		return false;
		
	menu->addItem(GenericMenuSeparator);

	int count = 0;
	char nvod_id[5];

	for( CSubServiceListSorted::iterator e=g_RemoteControl->subChannels.begin(); e!=g_RemoteControl->subChannels.end(); ++e)
	{
		sprintf(nvod_id, "%d", count);

		if( !g_RemoteControl->are_subchannels )
		{
			char nvod_time_a[50], nvod_time_e[50], nvod_time_x[50];
			char nvod_s[100];
			struct  tm *tmZeit;

			tmZeit= localtime(&e->startzeit);
			sprintf(nvod_time_a, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

			time_t endtime = e->startzeit+ e->dauer;
			tmZeit= localtime(&endtime);
			sprintf(nvod_time_e, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

			time_t jetzt=time(NULL);
			if(e->startzeit > jetzt)
			{
				int mins=(e->startzeit- jetzt)/ 60;
				sprintf(nvod_time_x, g_Locale->getText(LOCALE_NVOD_STARTING), mins);
			}
			else
				if( (e->startzeit<= jetzt) && (jetzt < endtime) )
			{
				int proz=(jetzt- e->startzeit)*100/ e->dauer;
				sprintf(nvod_time_x, g_Locale->getText(LOCALE_NVOD_PERCENTAGE), proz);
			}
			else
				nvod_time_x[0]= 0;

			sprintf(nvod_s, "%s - %s %s", nvod_time_a, nvod_time_e, nvod_time_x);
			menu->addItem(new CMenuForwarderNonLocalized(nvod_s, true, NULL, NVODChanger, nvod_id), (count == g_RemoteControl->selected_subchannel));
		}
		else
		{
			if (count == 0)
				menu->addItem(new CMenuForwarderNonLocalized( (Latin1_to_UTF8(e->subservice_name)).c_str(), true, NULL, NVODChanger, nvod_id, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
			else
				menu->addItem(new CMenuForwarderNonLocalized( (Latin1_to_UTF8(e->subservice_name)).c_str(), true, NULL, NVODChanger, nvod_id, CRCInput::convertDigitToKey(count)), (count == g_RemoteControl->selected_subchannel));
		}

		count++;
	}

	if( g_RemoteControl->are_subchannels )
	{
		menu->addItem(GenericMenuSeparatorLine);
		CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_NVODSELECTOR_DIRECTORMODE, &g_RemoteControl->director_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
		menu->addItem(oj);
	}

	return true;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (Neutrino)                               *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_neutrino()
{
	g_settings.menu_Head_alpha = 0x00;
	g_settings.menu_Head_red   = 0x00;
	g_settings.menu_Head_green = 0x0A;
	g_settings.menu_Head_blue  = 0x19;

	g_settings.menu_Head_Text_alpha = 0x00;
	g_settings.menu_Head_Text_red   = 0x5f;
	g_settings.menu_Head_Text_green = 0x46;
	g_settings.menu_Head_Text_blue  = 0x00;

	g_settings.menu_Content_alpha = 0x14;
	g_settings.menu_Content_red   = 0x00;
	g_settings.menu_Content_green = 0x0f;
	g_settings.menu_Content_blue  = 0x23;

	g_settings.menu_Content_Text_alpha = 0x00;
	g_settings.menu_Content_Text_red   = 0x64;
	g_settings.menu_Content_Text_green = 0x64;
	g_settings.menu_Content_Text_blue  = 0x64;

	g_settings.menu_Content_Selected_alpha = 0x14;
	g_settings.menu_Content_Selected_red   = 0x19;
	g_settings.menu_Content_Selected_green = 0x37;
	g_settings.menu_Content_Selected_blue  = 0x64;

	g_settings.menu_Content_Selected_Text_alpha  = 0x00;
	g_settings.menu_Content_Selected_Text_red    = 0x00;
	g_settings.menu_Content_Selected_Text_green  = 0x00;
	g_settings.menu_Content_Selected_Text_blue   = 0x00;

	g_settings.menu_Content_inactive_alpha = 0x14;
	g_settings.menu_Content_inactive_red   = 0x00;
	g_settings.menu_Content_inactive_green = 0x0f;
	g_settings.menu_Content_inactive_blue  = 0x23;

	g_settings.menu_Content_inactive_Text_alpha  = 0x00;
	g_settings.menu_Content_inactive_Text_red    = 55;
	g_settings.menu_Content_inactive_Text_green  = 70;
	g_settings.menu_Content_inactive_Text_blue   = 85;

	g_settings.infobar_alpha = 0x14;
	g_settings.infobar_red   = 0x00;
	g_settings.infobar_green = 0x0e;
	g_settings.infobar_blue  = 0x23;

	g_settings.infobar_Text_alpha = 0x00;
	g_settings.infobar_Text_red   = 0x64;
	g_settings.infobar_Text_green = 0x64;
	g_settings.infobar_Text_blue  = 0x64;
}


void CNeutrinoApp::addMenueIntroItems(CMenuWidget &item)
/*adds the typical menu intro with separator, back button and separatorline to menu*/
{
	item.addItem(GenericMenuSeparator);
	item.addItem(GenericMenuBack);
	item.addItem(GenericMenuSeparatorLine);
}

