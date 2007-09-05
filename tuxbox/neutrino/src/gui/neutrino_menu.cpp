/*
	$Id: neutrino_menu.cpp,v 1.5 2007/09/05 22:28:14 dbt Exp $
	
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
#include "gui/audio_select.h"
#include "gui/audioplayer.h"
#include "gui/epgplus.h"
#include "gui/favorites.h"
#include "gui/imageinfo.h"
#include "gui/keyhelper.h"
#include "gui/motorcontrol.h"
#include "gui/nfs.h"
#include "gui/personalize.h"
#include "gui/pictureviewer.h"
#include "gui/pluginlist.h"
#include "gui/scan.h"
#include "gui/screensetup.h"
#include "gui/streaminfo2.h"
#include "gui/sleeptimer.h"
#include "gui/update.h"
#if ENABLE_UPNP
#include "gui/upnpbrowser.h"
#endif

#ifdef _EXPERIMENTAL_SETTINGS_
#include "gui/experimental_menu.h"
#endif

CZapitClient::SatelliteList satList;
static CTimingSettingsNotifier timingsettingsnotifier;

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  init main menu                                             *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::InitMainMenu(CMenuWidget &mainMenu,
								CMenuWidget &mainSettings,
								CMenuWidget &audioSettings,
								CMenuWidget &parentallockSettings,
								CMenuWidget &networkSettings,
								CMenuWidget &recordingSettings,
								CMenuWidget &colorSettings,
								CMenuWidget &lcdSettings,
								CMenuWidget &keySettings,
								CMenuWidget &videoSettings,
								CMenuWidget &languageSettings,
								CMenuWidget &miscSettings,
								CMenuWidget &driverSettings,
								CMenuWidget &service,
								CMenuWidget &fontSettings,
								CMenuWidget &audiopl_picSettings,
								CMenuWidget &streamingSettings,
								CMenuWidget &moviePlayer)
{
	dprintf(DEBUG_DEBUG, "init mainmenue\n");

	// Dynamic renumbering
	int shortcut = 1;
	int shortcut2 = 1;

	// Main Menu
	mainMenu.addItem(GenericMenuSeparator);

	if (g_settings.personalize_tvmode == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_TVMODE, true, NULL, this, "tv", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED), firstchannel.mode != 'r');
	if (g_settings.personalize_tvmode == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_TVMODE, g_settings.personalize_pincode, true, true, NULL, this, "tv", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED), firstchannel.mode != 'r');

	if (g_settings.personalize_radiomode == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_RADIOMODE, true, NULL, this, "radio", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), firstchannel.mode == 'r');
	if (g_settings.personalize_radiomode == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_RADIOMODE, g_settings.personalize_pincode, true, true, NULL, this, "radio", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), firstchannel.mode == 'r');

	if (g_settings.personalize_scartmode == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SCARTMODE, true, NULL, this, "scart", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	if (g_settings.personalize_scartmode == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_SCARTMODE, g_settings.personalize_pincode, true, true, NULL, this, "scart", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

	if (g_settings.personalize_games == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_GAMES, true, NULL, new CPluginList(LOCALE_MAINMENU_GAMES,CPlugins::P_TYPE_GAME), "", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	if (g_settings.personalize_games == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_GAMES, g_settings.personalize_pincode, true, true, NULL, new CPluginList(LOCALE_MAINMENU_GAMES,CPlugins::P_TYPE_GAME), "", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

	if (g_settings.personalize_tvmode==0 && g_settings.personalize_radiomode==0 && g_settings.personalize_scartmode==0 && g_settings.personalize_games==0) {
		// Stop seperator from appearing when menu entries have been hidden	
	} else {
		mainMenu.addItem(GenericMenuSeparatorLine); }

	if (g_settings.personalize_audioplayer == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_AUDIOPLAYER, true, NULL, new CAudioPlayerGui(), NULL, CRCInput::convertDigitToKey(shortcut++)));
	if (g_settings.personalize_audioplayer == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_AUDIOPLAYER, g_settings.personalize_pincode, true, true, NULL, new CAudioPlayerGui(), NULL, CRCInput::convertDigitToKey(shortcut++)));

	if (g_settings.personalize_movieplayer == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_MOVIEPLAYER, true, NULL, &moviePlayer, NULL, CRCInput::convertDigitToKey(shortcut++)));
	if (g_settings.personalize_movieplayer == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_MOVIEPLAYER, g_settings.personalize_pincode, true, true, NULL, &moviePlayer, NULL, CRCInput::convertDigitToKey(shortcut++)));


	moviePlayer.addItem(GenericMenuSeparator);
	moviePlayer.addItem(GenericMenuBack);
	moviePlayer.addItem(GenericMenuSeparatorLine);
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, moviePlayerGui, "tsplayback", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK_PC, true, NULL, moviePlayerGui, "tsplayback_pc", CRCInput::RC_1));
#ifdef MOVIEBROWSER
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, moviePlayerGui, "tsmoviebrowser", CRCInput::RC_2));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_BOOKMARK, true, NULL, moviePlayerGui, "bookmarkplayback", CRCInput::RC_3));
#else
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_BOOKMARK, true, NULL, moviePlayerGui, "bookmarkplayback", CRCInput::RC_2));
#endif /* MOVIEBROWSER */
	moviePlayer.addItem(GenericMenuSeparatorLine);
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_FILEPLAYBACK, true, NULL, moviePlayerGui, "fileplayback", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_DVDPLAYBACK, true, NULL, moviePlayerGui, "dvdplayback", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_VCDPLAYBACK, true, NULL, moviePlayerGui, "vcdplayback", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	moviePlayer.addItem(GenericMenuSeparatorLine);
	moviePlayer.addItem(new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, &streamingSettings, NULL, CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP_SMALL));
	moviePlayer.addItem(new CMenuForwarder(LOCALE_NFSMENU_HEAD, true, NULL, new CNFSSmallMenu(), NULL, CRCInput::RC_setup, NEUTRINO_ICON_BUTTON_DBOX_SMALL));

	if (g_settings.personalize_pictureviewer == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_PICTUREVIEWER, true, NULL, new CPictureViewerGui(), NULL, CRCInput::convertDigitToKey(shortcut++)));
	if (g_settings.personalize_pictureviewer == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_PICTUREVIEWER, g_settings.personalize_pincode, true, true, NULL, new CPictureViewerGui(), NULL, CRCInput::convertDigitToKey(shortcut++)));

#if ENABLE_UPNP
	if (g_settings.personalize_upnpbrowser == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_UPNPBROWSER, true, NULL, new CUpnpBrowserGui(), NULL, CRCInput::convertDigitToKey(shortcut++)));
	if (g_settings.personalize_upnpbrowser == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_UPNPBROWSER, g_settings.personalize_pincode, true, true, NULL, new CUpnpBrowserGui(), NULL, CRCInput::convertDigitToKey(shortcut++)));
#endif

	if (g_PluginList->hasPlugin(CPlugins::P_TYPE_SCRIPT))
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SCRIPTS, true, NULL, new CPluginList(LOCALE_MAINMENU_SCRIPTS,CPlugins::P_TYPE_SCRIPT), "",
										CRCInput::convertDigitToKey(shortcut++)));

	if (g_settings.personalize_audioplayer==0 && g_settings.personalize_movieplayer==0 && g_settings.personalize_pictureviewer==0 && g_settings.personalize_upnpbrowser==0) {
		// Stop seperator from appearing when menu entries have been hidden
	} else {
		mainMenu.addItem(GenericMenuSeparatorLine); }

	if (g_settings.personalize_settings == 0)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, &mainSettings, NULL, CRCInput::convertDigitToKey(shortcut++)));
	else
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_SETTINGS, g_settings.personalize_pincode, true, true, NULL, &mainSettings, NULL, CRCInput::convertDigitToKey(shortcut++)));

	if (g_settings.personalize_service == 0)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SERVICE, true, NULL, &service, NULL, CRCInput::convertDigitToKey(shortcut++)));
	else
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_SERVICE, g_settings.personalize_pincode, true, true, NULL, &service, NULL, CRCInput::convertDigitToKey(shortcut++)));

	if (g_settings.personalize_sleeptimer==0 && g_settings.personalize_reboot==0 && g_settings.personalize_shutdown==0) {
		 // Stop seperator from appearing when menu entries have been hidden
	} else {
	mainMenu.addItem(GenericMenuSeparatorLine);
	 }

	if (g_settings.personalize_sleeptimer == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SLEEPTIMER, true, NULL, new CSleepTimerWidget, NULL, CRCInput::convertDigitToKey(shortcut++)));
	if (g_settings.personalize_sleeptimer == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_SLEEPTIMER, g_settings.personalize_pincode, true, true, NULL, new CSleepTimerWidget, NULL, CRCInput::convertDigitToKey(shortcut++)));

	if (g_settings.personalize_reboot == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_REBOOT, true, NULL, this, "reboot", CRCInput::convertDigitToKey(shortcut++)));
	if (g_settings.personalize_reboot == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_REBOOT, g_settings.personalize_pincode, true, true, NULL, this, "reboot", CRCInput::convertDigitToKey(shortcut++)));

	if (g_settings.personalize_shutdown == 1)
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SHUTDOWN, true, NULL, this, "shutdown", CRCInput::RC_standby, "power.raw"));
	if (g_settings.personalize_shutdown == 2)
		mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_SHUTDOWN, g_settings.personalize_pincode, true, true, NULL, this, "shutdown", CRCInput::RC_standby, "power.raw"));

	// Settings Menu
	mainSettings.addItem(GenericMenuSeparator);
	mainSettings.addItem(GenericMenuBack);
	mainSettings.addItem(GenericMenuSeparatorLine);
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	mainSettings.addItem(GenericMenuSeparatorLine);

	if (g_settings.personalize_video == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_VIDEO, true, NULL, &videoSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_video == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_VIDEO, g_settings.personalize_pincode, true, true, NULL, &videoSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_audio == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_AUDIO, true, NULL, &audioSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_audio == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_AUDIO, g_settings.personalize_pincode, true, true, NULL, &audioSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_youth == 1)
		if(g_settings.parentallock_prompt)
			mainSettings.addItem(new CLockedMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, g_settings.parentallock_pincode, true, true, NULL, &parentallockSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
		else
			mainSettings.addItem(new CMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, true, NULL, &parentallockSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_youth == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, g_settings.personalize_pincode, true, true, NULL, &parentallockSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_network == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_NETWORK, true, NULL, &networkSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_network == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_NETWORK, g_settings.personalize_pincode, true, true, NULL, &networkSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_recording == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_RECORDING, true, NULL, &recordingSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_recording == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_RECORDING, g_settings.personalize_pincode, true, true, NULL, &recordingSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_streaming == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_STREAMING, true, NULL, &streamingSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_streaming == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_STREAMING, g_settings.personalize_pincode, true, true, NULL, &streamingSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_language == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_LANGUAGE, true, NULL, &languageSettings , NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_language == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_LANGUAGE, g_settings.personalize_pincode, true, true, NULL, &languageSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_colors == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_COLORS, true, NULL, &colorSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_colors == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_COLORS, g_settings.personalize_pincode, true, true, NULL, &colorSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_lcd == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_LCD, true, NULL, &lcdSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));
	if (g_settings.personalize_lcd == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_LCD, g_settings.personalize_pincode, true, true, NULL, &lcdSettings, NULL, CRCInput::convertDigitToKey(shortcut2++)));

	if (g_settings.personalize_keybinding == 1){
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_KEYBINDING, true, NULL, &keySettings, NULL, CRCInput::convertDigitToKey((shortcut2 == 10) ? 0 : shortcut2)));
		if (shortcut==10) shortcut2++;
		}
	if (g_settings.personalize_keybinding == 2){
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_KEYBINDING, g_settings.personalize_pincode, true, true, NULL, &keySettings, NULL, CRCInput::convertDigitToKey((shortcut2 == 10) ? 0 : shortcut2)));
		if (shortcut==10) shortcut2++;
		}
	
	if (g_settings.personalize_audpic == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_AUDIOPLAYERPICSETTINGS_GENERAL, true, NULL, &audiopl_picSettings, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	if (g_settings.personalize_audpic == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_AUDIOPLAYERPICSETTINGS_GENERAL, g_settings.personalize_pincode, true, true, NULL, &audiopl_picSettings, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

	if (g_settings.personalize_driver == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_DRIVER, true, NULL, &driverSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	if (g_settings.personalize_driver == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_DRIVER, g_settings.personalize_pincode, true, true, NULL, &driverSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));

	if (g_settings.personalize_misc == 1)
		mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_MISC, true, NULL, &miscSettings, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW ));
	if (g_settings.personalize_misc == 2)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_MAINSETTINGS_MISC, g_settings.personalize_pincode, true, true, NULL, &miscSettings, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));

	if (g_settings.personalize_pinstatus == 0){
		shortcut++;
		mainSettings.addItem(new CMenuForwarder(LOCALE_PERSONALIZE_HEAD, true, NULL, new CPersonalizeGui(), NULL, CRCInput::convertDigitToKey((shortcut2 == 10) ? 0 : shortcut2)));
		}
	if (g_settings.personalize_pinstatus == 1){
		shortcut++;
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_PERSONALIZE_HEAD, g_settings.personalize_pincode, true, true, NULL, new CPersonalizeGui(), NULL, CRCInput::convertDigitToKey((shortcut2 == 10) ? 0 : shortcut2)));
		}
}

#define SCANTS_BOUQUET_OPTION_COUNT 5
const CMenuOptionChooser::keyval SCANTS_BOUQUET_OPTIONS[SCANTS_BOUQUET_OPTION_COUNT] =
{
	{ CZapitClient::BM_DELETEBOUQUETS        , LOCALE_SCANTS_BOUQUET_ERASE     },
	{ CZapitClient::BM_CREATEBOUQUETS        , LOCALE_SCANTS_BOUQUET_CREATE    },
	{ CZapitClient::BM_DONTTOUCHBOUQUETS     , LOCALE_SCANTS_BOUQUET_LEAVE     },
	{ CZapitClient::BM_UPDATEBOUQUETS        , LOCALE_SCANTS_BOUQUET_UPDATE    },
	{ CZapitClient::BM_CREATESATELLITEBOUQUET, LOCALE_SCANTS_BOUQUET_SATELLITE }
};

#define SCANTS_ZAPIT_SCANTYPE_COUNT 4
const CMenuOptionChooser::keyval SCANTS_ZAPIT_SCANTYPE[SCANTS_ZAPIT_SCANTYPE_COUNT] =
{
	{  CZapitClient::ST_TVRADIO	, LOCALE_ZAPIT_SCANTYPE_TVRADIO     },
	{  CZapitClient::ST_TV		, LOCALE_ZAPIT_SCANTYPE_TV    },
	{  CZapitClient::ST_RADIO	, LOCALE_ZAPIT_SCANTYPE_RADIO     },
	{  CZapitClient::ST_ALL		, LOCALE_ZAPIT_SCANTYPE_ALL }
};

#define SATSETUP_DISEQC_OPTION_COUNT 6
const CMenuOptionChooser::keyval SATSETUP_DISEQC_OPTIONS[SATSETUP_DISEQC_OPTION_COUNT] =
{
	{ NO_DISEQC          , LOCALE_SATSETUP_NODISEQC    },
	{ MINI_DISEQC        , LOCALE_SATSETUP_MINIDISEQC  },
	{ DISEQC_1_0         , LOCALE_SATSETUP_DISEQC10    },
	{ DISEQC_1_1         , LOCALE_SATSETUP_DISEQC11    },
	{ DISEQC_1_2         , LOCALE_SATSETUP_DISEQC12    },
	{ SMATV_REMOTE_TUNING, LOCALE_SATSETUP_SMATVREMOTE }

};

#define SATSETUP_SCANTP_FEC_COUNT 5
#if HAVE_DVB_API_VERSION < 3
const CMenuOptionChooser::keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
	{ 1, LOCALE_SCANTP_FEC_1_2 },
	{ 2, LOCALE_SCANTP_FEC_2_3 },
	{ 3, LOCALE_SCANTP_FEC_3_4 },
	{ 4, LOCALE_SCANTP_FEC_5_6 },
	{ 5, LOCALE_SCANTP_FEC_7_8 }
};
#else
const CMenuOptionChooser::keyval SATSETUP_SCANTP_FEC[SATSETUP_SCANTP_FEC_COUNT] =
{
        { 1, LOCALE_SCANTP_FEC_1_2 },
        { 2, LOCALE_SCANTP_FEC_2_3 },
        { 3, LOCALE_SCANTP_FEC_3_4 },
        { 5, LOCALE_SCANTP_FEC_5_6 },
        { 7, LOCALE_SCANTP_FEC_7_8 }
};
#endif

#define SATSETUP_SCANTP_POL_COUNT 2
const CMenuOptionChooser::keyval SATSETUP_SCANTP_POL[SATSETUP_SCANTP_POL_COUNT] =
{
	{ 0, LOCALE_SCANTP_POL_H },
	{ 1, LOCALE_SCANTP_POL_V }
};

#define OSD_STATUS_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval OSD_STATUS_OPTIONS[OSD_STATUS_OPTIONS_COUNT] =
{
    { 0, LOCALE_COLORMENU_OSD_NORMAL },
    { 1, LOCALE_COLORMENU_OSD_BOTTOM },
    { 2, LOCALE_COLORMENU_OSD_OFF    }
};

#define SECTIONSD_SCAN_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval SECTIONSD_SCAN_OPTIONS[SECTIONSD_SCAN_OPTIONS_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  },
	{ 2, LOCALE_OPTIONS_ON_WITHOUT_MESSAGES  }
};

const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  }
};

void CNeutrinoApp::InitScanSettings(CMenuWidget &settings)
{
	dprintf(DEBUG_DEBUG, "init scansettings\n");

	CMenuOptionChooser* ojScantype = new CMenuOptionChooser(LOCALE_ZAPIT_SCANTYPE, (int *)&scanSettings.scanType, SCANTS_ZAPIT_SCANTYPE, SCANTS_ZAPIT_SCANTYPE_COUNT, true, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);
	CMenuOptionChooser* ojBouquets = new CMenuOptionChooser(LOCALE_SCANTS_BOUQUET, (int *)&scanSettings.bouquetMode, SCANTS_BOUQUET_OPTIONS, SCANTS_BOUQUET_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);

	settings.addItem(GenericMenuSeparator);
	settings.addItem(GenericMenuBack);
	settings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	settings.addItem(GenericMenuSeparatorLine);

	//sat-lnb-settings
	if(g_info.delivery_system == DVB_S)
	{

		satList.clear();
		g_Zapit->getScanSatelliteList(satList);

		printf("[neutrino] received %d sats\n", satList.size());
		t_satellite_position currentSatellitePosition = g_Zapit->getCurrentSatellitePosition();

		if (1/*scanSettings.diseqcMode == DISEQC_1_2*/)
		{
			for (uint i = 0; i < satList.size(); i++)
			{
				//printf("[neutrino] received %d: %s, %d\n", i, satList[i].satName, satList[i].satPosition);
				scanSettings.satPosition[i] = satList[i].satPosition;
				scanSettings.satMotorPos[i] = satList[i].motorPosition;
				strcpy(scanSettings.satName[i], satList[i].satName);
				//scanSettings.satDiseqc[i] = satList[i].satDiseqc;
				if (satList[i].satPosition == currentSatellitePosition)
					strcpy(scanSettings.satNameNoDiseqc, satList[i].satName);
			}
			for (uint i = satList.size(); i < MAX_SATELLITES; i++)
			{
				scanSettings.satName[i][0] = 0;
				scanSettings.satPosition[i] = 0;
				scanSettings.satDiseqc[i] = -1;
			}
		}

		CMenuOptionStringChooser* ojSat = new CMenuOptionStringChooser(LOCALE_SATSETUP_SATELLITE, scanSettings.satNameNoDiseqc, ((scanSettings.diseqcMode == DISEQC_1_2) || (scanSettings.diseqcMode == NO_DISEQC)));
		for (uint i=0; i < satList.size(); i++)
		{
			ojSat->addOption(satList[i].satName);
			dprintf(DEBUG_DEBUG, "got scanprovider (sat): %s\n", satList[i].satName );
		}

		CMenuOptionNumberChooser * ojDiseqcRepeats = new CMenuOptionNumberChooser(LOCALE_SATSETUP_DISEQCREPEAT, (int *)&scanSettings.diseqcRepeat, (scanSettings.diseqcMode != NO_DISEQC) && (scanSettings.diseqcMode != DISEQC_1_0), 0, 2);

		CMenuWidget* extSatSettings = new CMenuWidget(LOCALE_SATSETUP_EXTENDED, NEUTRINO_ICON_SETTINGS);
		extSatSettings->addItem(GenericMenuSeparator);
		extSatSettings->addItem(GenericMenuBack);
		extSatSettings->addItem(GenericMenuSeparatorLine);

		CMenuForwarder* ojExtSatSettings = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED, (scanSettings.diseqcMode != NO_DISEQC), NULL, extSatSettings);
		for( uint i=0; i < satList.size(); i++)
		{
			CMenuOptionNumberChooser * oj = new CMenuOptionNumberChooser(NONEXISTANT_LOCALE, scanSettings.diseqscOfSat(satList[i].satName), true, -1, satList.size() - 1, 1, -1, LOCALE_OPTIONS_OFF, satList[i].satName);

			extSatSettings->addItem(oj);
		}

		CMenuWidget* extMotorSettings = new CMenuWidget(LOCALE_SATSETUP_EXTENDED_MOTOR, NEUTRINO_ICON_SETTINGS);
		extMotorSettings->addItem(GenericMenuSeparator);
		extMotorSettings->addItem(GenericMenuBack);
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_SAVESETTINGSNOW, true, NULL, this, "savesettings"));
		extMotorSettings->addItem(new CMenuForwarder(LOCALE_SATSETUP_MOTORCONTROL   , true, NULL, new CMotorControl()));
		extMotorSettings->addItem(GenericMenuSeparatorLine);

		CMenuForwarder* ojExtMotorSettings = new CMenuForwarder(LOCALE_SATSETUP_EXTENDED_MOTOR, (scanSettings.diseqcMode == DISEQC_1_2), NULL, extMotorSettings);

		for( uint i=0; i < satList.size(); i++)
		{
			CMenuOptionNumberChooser * oj = new CMenuOptionNumberChooser(NONEXISTANT_LOCALE, scanSettings.motorPosOfSat(satList[i].satName), true, 0, 64/*satList.size()*/, 0, 0, LOCALE_OPTIONS_OFF, satList[i].satName);

			extMotorSettings->addItem(oj);
		}

		CMenuOptionChooser* ojDiseqc = new CMenuOptionChooser(LOCALE_SATSETUP_DISEQC, (int *)&scanSettings.diseqcMode, SATSETUP_DISEQC_OPTIONS, SATSETUP_DISEQC_OPTION_COUNT, true, new CSatDiseqcNotifier(ojSat, ojExtSatSettings, ojExtMotorSettings, ojDiseqcRepeats));

		settings.addItem( ojScantype );
		settings.addItem( ojBouquets );
		settings.addItem( ojDiseqc );
		settings.addItem( ojSat );
		settings.addItem( ojDiseqcRepeats );

		settings.addItem( ojExtSatSettings );
		settings.addItem( ojExtMotorSettings );
	}
	else
	{//kabel

		CZapitClient::SatelliteList providerList;
		g_Zapit->getScanSatelliteList(providerList);

		CMenuOptionStringChooser* oj = new CMenuOptionStringChooser(LOCALE_CABLESETUP_PROVIDER, (char*)&scanSettings.satNameNoDiseqc, true);

		for( uint i=0; i< providerList.size(); i++)
		{
			oj->addOption(providerList[i].satName);
			dprintf(DEBUG_DEBUG, "got scanprovider (cable): %s\n", providerList[i].satName );
		}
		settings.addItem( ojScantype );
		settings.addItem( ojBouquets );
		settings.addItem( oj);
	}
	CMenuOptionChooser* onoff_mode = ( new CMenuOptionChooser(LOCALE_SCANTP_SCANMODE, (int *)&scanSettings.scan_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	settings.addItem(onoff_mode);
	if(scanSettings.TP_fec == 0) {
		scanSettings.TP_fec = 1;
	}
	settings.addItem(GenericMenuSeparatorLine);

	CStringInput* freq = new CStringInput(LOCALE_SCANTP_FREQ, (char *) scanSettings.TP_freq, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	CStringInput* rate = new CStringInput(LOCALE_SCANTP_RATE, (char *) scanSettings.TP_rate, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	CMenuOptionChooser* fec = new CMenuOptionChooser(LOCALE_SCANTP_FEC, (int *)&scanSettings.TP_fec, SATSETUP_SCANTP_FEC, SATSETUP_SCANTP_FEC_COUNT, scanSettings.TP_scan);
	CMenuOptionChooser* pol = new CMenuOptionChooser(LOCALE_SCANTP_POL, (int *)&scanSettings.TP_pol, SATSETUP_SCANTP_POL, SATSETUP_SCANTP_POL_COUNT, scanSettings.TP_scan);
	CMenuOptionChooser* onoffscanSectionsd = ( new CMenuOptionChooser(LOCALE_SECTIONSD_SCANMODE, (int *)&scanSettings.scanSectionsd, SECTIONSD_SCAN_OPTIONS, SECTIONSD_SCAN_OPTIONS_COUNT, true, new CSectionsdConfigNotifier));
	CMenuForwarder *Rate =new CMenuForwarder(LOCALE_SCANTP_RATE, scanSettings.TP_scan, scanSettings.TP_rate, rate);
	CMenuForwarder *Freq = new CMenuForwarder(LOCALE_SCANTP_FREQ, scanSettings.TP_scan, scanSettings.TP_freq, freq);

	scanSettings.TP_SatSelectMenu = new CMenuOptionStringChooser(LOCALE_SATSETUP_SATELLITE, scanSettings.TP_satname, ((scanSettings.diseqcMode != NO_DISEQC) && scanSettings.TP_scan), new CScanSettingsSatManNotifier);
	//sat-lnb-settings
	if(g_info.delivery_system == DVB_S)
	{
		std::vector<std::string> tmpsatNameList;
		tmpsatNameList.clear();
		for (uint i = 0; i < satList.size(); i++)
		{
			if (0 <= (*scanSettings.diseqscOfSat(satList[i].satName)))
			{
				tmpsatNameList.push_back(satList[i].satName);
				dprintf(DEBUG_DEBUG, "satName = %s, diseqscOfSat(%d) = %d\n", satList[i].satName, i, *scanSettings.diseqscOfSat(satList[i].satName));
			}
		}

		for (uint i=0; i < tmpsatNameList.size(); i++)
		{
			scanSettings.TP_SatSelectMenu->addOption(tmpsatNameList[i].c_str());
			dprintf(DEBUG_DEBUG, "got scanprovider (sat): %s\n", tmpsatNameList[i].c_str());
		}
	}

	CTP_scanNotifier *TP_scanNotifier = new CTP_scanNotifier(fec,pol,Freq,Rate,scanSettings.TP_SatSelectMenu);
	CMenuOptionChooser* onoff = ( new CMenuOptionChooser(LOCALE_SCANTP_SCAN, (int *)&scanSettings.TP_scan, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_info.delivery_system == DVB_S), TP_scanNotifier));

	settings.addItem(onoff);
	settings.addItem(Freq);
	settings.addItem(Rate);
	settings.addItem(fec);
	settings.addItem(pol);
	settings.addItem(scanSettings.TP_SatSelectMenu);
	settings.addItem(GenericMenuSeparatorLine);
	settings.addItem(onoffscanSectionsd);
	settings.addItem(GenericMenuSeparatorLine);

	settings.addItem(new CMenuForwarder(LOCALE_SCANTS_STARTNOW, true, NULL, new CScanTs(), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
}


#define FLASHUPDATE_UPDATEMODE_OPTION_COUNT 2
const CMenuOptionChooser::keyval FLASHUPDATE_UPDATEMODE_OPTIONS[FLASHUPDATE_UPDATEMODE_OPTION_COUNT] =
{
	{ 0, LOCALE_FLASHUPDATE_UPDATEMODE_MANUAL   },
	{ 1, LOCALE_FLASHUPDATE_UPDATEMODE_INTERNET }
};

void CNeutrinoApp::InitServiceSettings(CMenuWidget &service, CMenuWidget &scanSettings)
{
	dprintf(DEBUG_DEBUG, "init serviceSettings\n");

	// Dynamic renumbering
	int shortcut3 = 1;
	service.addItem(GenericMenuSeparator);
	service.addItem(GenericMenuBack);
	service.addItem(GenericMenuSeparatorLine);
	if (g_settings.personalize_bouqueteditor == 1)
		service.addItem(new CMenuForwarder(LOCALE_BOUQUETEDITOR_NAME, true, NULL, new CBEBouquetWidget(), NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	if (g_settings.personalize_bouqueteditor == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_BOUQUETEDITOR_NAME, g_settings.personalize_pincode, true, true, NULL, new CBEBouquetWidget(), NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

	if (g_settings.personalize_scants == 1)
		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_SCANTS, true, NULL, &scanSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	if (g_settings.personalize_scants == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_SERVICEMENU_SCANTS, g_settings.personalize_pincode, true, true, NULL, &scanSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));

	if (g_settings.personalize_bouqueteditor==0 && g_settings.personalize_scants==0) {
		// Stop seperator from appearing when menu entries have been hidden
	} else {
		service.addItem(GenericMenuSeparatorLine); }

	if (g_settings.personalize_reload == 1)
		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_RELOAD, true, NULL, this, "reloadchannels", CRCInput::convertDigitToKey(shortcut3++)));
	if (g_settings.personalize_reload == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_SERVICEMENU_RELOAD, g_settings.personalize_pincode, true, true, NULL, this, "reloadchannels", CRCInput::convertDigitToKey(shortcut3++)));

	if (g_settings.personalize_getplugins == 1)
		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_GETPLUGINS, true, NULL, this, "reloadplugins" , CRCInput::convertDigitToKey(shortcut3++)));
	if (g_settings.personalize_getplugins == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_SERVICEMENU_GETPLUGINS, g_settings.personalize_pincode, true, true, NULL, this, "reloadplugins", CRCInput::convertDigitToKey(shortcut3++)));

	if (g_settings.personalize_restart == 1)
		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_RESTART, true, NULL, this, "restart", CRCInput::convertDigitToKey(shortcut3++)));
	if (g_settings.personalize_restart == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_SERVICEMENU_RESTART, g_settings.personalize_pincode, true, true, NULL, this, "restart", CRCInput::convertDigitToKey(shortcut3++)));

#ifndef HAVE_DREAMBOX_HARDWARE
	if (g_settings.personalize_ucodecheck == 1)
		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_UCODECHECK, true, NULL, UCodeChecker, NULL, CRCInput::convertDigitToKey(shortcut3++)));
	if (g_settings.personalize_ucodecheck == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_SERVICEMENU_UCODECHECK, g_settings.personalize_pincode, true, true, NULL, UCodeChecker, NULL, CRCInput::convertDigitToKey(shortcut3++)));
#endif
	if (g_settings.personalize_reload==0 && g_settings.personalize_getplugins==0 && g_settings.personalize_restart==0 && g_settings.personalize_ucodecheck==0) {
		// Stop seperator from appearing when menu entries have been hidden
	} else {
		service.addItem(GenericMenuSeparatorLine); }

	if (g_settings.personalize_imageinfo == 1)
		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_IMAGEINFO, true, NULL, new CImageInfo(), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW  ), false);
	if (g_settings.personalize_imageinfo == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_SERVICEMENU_IMAGEINFO, g_settings.personalize_pincode, true, true, NULL, new CImageInfo(), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);

	//softupdate
	if(softupdate)
	{
		dprintf(DEBUG_DEBUG, "init soft-update-stuff\n");
		CMenuWidget* updateSettings = new CMenuWidget(LOCALE_SERVICEMENU_UPDATE, "softupdate.raw", 550);
		updateSettings->addItem(GenericMenuSeparator);
		updateSettings->addItem(GenericMenuBack);
		updateSettings->addItem(GenericMenuSeparatorLine);


		// experts-functions to read/write to the mtd
		CMenuWidget* mtdexpert = new CMenuWidget(LOCALE_FLASHUPDATE_EXPERTFUNCTIONS, "softupdate.raw");
		mtdexpert->addItem(GenericMenuSeparator);
		mtdexpert->addItem(GenericMenuBack);
		mtdexpert->addItem(GenericMenuSeparatorLine);
		CFlashExpert* fe = new CFlashExpert();
		mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_READFLASH    , true, NULL, fe, "readflash"       , CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
		mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_WRITEFLASH   , true, NULL, fe, "writeflash"      , CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
		mtdexpert->addItem(GenericMenuSeparatorLine);
		mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_READFLASHMTD , true, NULL, fe, "readflashmtd"    , CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
		mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_WRITEFLASHMTD, true, NULL, fe, "writeflashmtd"   , CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
		mtdexpert->addItem(GenericMenuSeparatorLine);

		CStringInputSMS * updateSettings_url_file = new CStringInputSMS(LOCALE_FLASHUPDATE_URL_FILE, g_settings.softupdate_url_file, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789!""$%&/()=?-. ");
		mtdexpert->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_URL_FILE, true, g_settings.softupdate_url_file, updateSettings_url_file));

		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_EXPERTFUNCTIONS, true, NULL, mtdexpert, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));

#ifndef HAVE_DREAMBOX_HARDWARE
		updateSettings->addItem(GenericMenuSeparatorLine);
		CMenuOptionChooser *oj = new CMenuOptionChooser(LOCALE_FLASHUPDATE_UPDATEMODE, &g_settings.softupdate_mode, FLASHUPDATE_UPDATEMODE_OPTIONS, FLASHUPDATE_UPDATEMODE_OPTION_COUNT, true);
		updateSettings->addItem( oj );
#endif

		/* show current version */
		updateSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_FLASHUPDATE_CURRENTVERSION_SEP));

		/* get current version SBBBYYYYMMTTHHMM -- formatsting */
		CConfigFile configfile('\t');

		const char * versionString = (configfile.loadConfig("/.version")) ? (configfile.getString( "version", "????????????????").c_str()) : "????????????????";

		dprintf(DEBUG_INFO, "current flash-version: %s\n", versionString);

		static CFlashVersionInfo versionInfo(versionString);

		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTVERSIONDATE    , false, versionInfo.getDate()));
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTVERSIONTIME    , false, versionInfo.getTime()));
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTRELEASECYCLE   , false, versionInfo.getReleaseCycle()));
		/* versionInfo.getType() returns const char * which is never deallocated */
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CURRENTVERSIONSNAPSHOT, false, versionInfo.getType()));

#ifndef HAVE_DREAMBOX_HARDWARE
		updateSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_FLASHUPDATE_PROXYSERVER_SEP));

		CStringInputSMS * updateSettings_proxy = new CStringInputSMS(LOCALE_FLASHUPDATE_PROXYSERVER, g_settings.softupdate_proxyserver, 23, LOCALE_FLASHUPDATE_PROXYSERVER_HINT1, LOCALE_FLASHUPDATE_PROXYSERVER_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYSERVER, true, g_settings.softupdate_proxyserver, updateSettings_proxy));

		CStringInputSMS * updateSettings_proxyuser = new CStringInputSMS(LOCALE_FLASHUPDATE_PROXYUSERNAME, g_settings.softupdate_proxyusername, 23, LOCALE_FLASHUPDATE_PROXYUSERNAME_HINT1, LOCALE_FLASHUPDATE_PROXYUSERNAME_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYUSERNAME, true, g_settings.softupdate_proxyusername, updateSettings_proxyuser));

		CStringInputSMS * updateSettings_proxypass = new CStringInputSMS(LOCALE_FLASHUPDATE_PROXYPASSWORD, g_settings.softupdate_proxypassword, 20, LOCALE_FLASHUPDATE_PROXYPASSWORD_HINT1, LOCALE_FLASHUPDATE_PROXYPASSWORD_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYPASSWORD, true, g_settings.softupdate_proxypassword, updateSettings_proxypass));
#endif

		updateSettings->addItem(GenericMenuSeparatorLine);
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CHECKUPDATE, true, NULL, new CFlashUpdate(), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));


	if (g_settings.personalize_update == 1)
		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_UPDATE, true, NULL, updateSettings, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	if (g_settings.personalize_update == 2)
		service.addItem(new CLockedMenuForwarder(LOCALE_SERVICEMENU_UPDATE, g_settings.personalize_pincode, true, true, NULL, updateSettings, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

	}
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

#define PICTUREVIEWER_SCALING_OPTION_COUNT 3
const CMenuOptionChooser::keyval PICTUREVIEWER_SCALING_OPTIONS[PICTUREVIEWER_SCALING_OPTION_COUNT] =
{
	{ CPictureViewer::SIMPLE, LOCALE_PICTUREVIEWER_RESIZE_SIMPLE        },
	{ CPictureViewer::COLOR , LOCALE_PICTUREVIEWER_RESIZE_COLOR_AVERAGE },
	{ CPictureViewer::NONE  , LOCALE_PICTUREVIEWER_RESIZE_NONE          }
};

#define AUDIOPLAYER_DISPLAY_ORDER_OPTION_COUNT 2
const CMenuOptionChooser::keyval AUDIOPLAYER_DISPLAY_ORDER_OPTIONS[AUDIOPLAYER_DISPLAY_ORDER_OPTION_COUNT] =
{
	{ CAudioPlayerGui::ARTIST_TITLE, LOCALE_AUDIOPLAYER_ARTIST_TITLE },
	{ CAudioPlayerGui::TITLE_ARTIST, LOCALE_AUDIOPLAYER_TITLE_ARTIST }
};

void CNeutrinoApp::InitAudioplPicSettings(CMenuWidget &audioplPicSettings)
{
	audioplPicSettings.addItem(GenericMenuSeparator);
	audioplPicSettings.addItem(GenericMenuBack);

	audioplPicSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_PICTUREVIEWER_HEAD));
	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_PICTUREVIEWER_SCALING  , &g_settings.picviewer_scaling     , PICTUREVIEWER_SCALING_OPTIONS  , PICTUREVIEWER_SCALING_OPTION_COUNT  , true ));
	CStringInput * pic_timeout= new CStringInput(LOCALE_PICTUREVIEWER_SLIDE_TIME, g_settings.picviewer_slide_time, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	audioplPicSettings.addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_SLIDE_TIME, true, g_settings.picviewer_slide_time, pic_timeout));
	audioplPicSettings.addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DEFDIR, true, g_settings.network_nfs_picturedir, this, "picturedir"));
	CIPInput * audioplPicSettings_DecServerIP = new CIPInput( LOCALE_PICTUREVIEWER_DECODE_SERVER_IP, g_settings.picviewer_decode_server_ip, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	audioplPicSettings.addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DECODE_SERVER_IP, true, g_settings.picviewer_decode_server_ip, audioplPicSettings_DecServerIP));
	CStringInput * audioplPicSettings_DecServerPort= new CStringInput(LOCALE_PICTUREVIEWER_DECODE_SERVER_PORT, g_settings.picviewer_decode_server_port, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	audioplPicSettings.addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DECODE_SERVER_PORT, true, g_settings.picviewer_decode_server_port, audioplPicSettings_DecServerPort));

	audioplPicSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_AUDIOPLAYER_NAME));
	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_DISPLAY_ORDER, &g_settings.audioplayer_display     , AUDIOPLAYER_DISPLAY_ORDER_OPTIONS, AUDIOPLAYER_DISPLAY_ORDER_OPTION_COUNT, true ));
	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_FOLLOW       , &g_settings.audioplayer_follow      , MESSAGEBOX_NO_YES_OPTIONS      , MESSAGEBOX_NO_YES_OPTION_COUNT      , true ));
	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_SELECT_TITLE_BY_NAME       , &g_settings.audioplayer_select_title_by_name      , MESSAGEBOX_NO_YES_OPTIONS      , MESSAGEBOX_NO_YES_OPTION_COUNT      , true ));
	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_REPEAT_ON       , &g_settings.audioplayer_repeat_on      , MESSAGEBOX_NO_YES_OPTIONS      , MESSAGEBOX_NO_YES_OPTION_COUNT      , true ));
	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_SHOW_PLAYLIST, &g_settings.audioplayer_show_playlist, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true ));

	CStringInput * audio_screensaver= new CStringInput(LOCALE_AUDIOPLAYER_SCREENSAVER_TIMEOUT, g_settings.audioplayer_screensaver, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	audioplPicSettings.addItem(new CMenuForwarder(LOCALE_AUDIOPLAYER_SCREENSAVER_TIMEOUT, true, g_settings.audioplayer_screensaver, audio_screensaver));

	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_HIGHPRIO     , &g_settings.audioplayer_highprio    , MESSAGEBOX_NO_YES_OPTIONS      , MESSAGEBOX_NO_YES_OPTION_COUNT      , true ));
	audioplPicSettings.addItem(new CMenuForwarder(LOCALE_AUDIOPLAYER_DEFDIR, true, g_settings.network_nfs_audioplayerdir, this, "audioplayerdir"));
	audioplPicSettings.addItem(new CMenuOptionChooser(LOCALE_AUDIOPLAYER_ENABLE_SC_METADATA, &g_settings.audioplayer_enable_sc_metadata, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true ));

}


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

#define INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT 4
const CMenuOptionChooser::keyval  INFOBAR_SUBCHAN_DISP_POS_OPTIONS[INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_SETTINGS_POS_TOP_RIGHT },
	{ 1 , LOCALE_SETTINGS_POS_TOP_LEFT },
	{ 2 , LOCALE_SETTINGS_POS_BOTTOM_LEFT },
	{ 3 , LOCALE_SETTINGS_POS_BOTTOM_RIGHT }
};

#define INFOBAR_SHOW_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval  INFOBAR_SHOW_OPTIONS[INFOBAR_SHOW_OPTIONS_COUNT]=
{
	{ 0 , LOCALE_OPTIONS_OFF },
	{ 1 , LOCALE_PICTUREVIEWER_RESIZE_SIMPLE },
	{ 2 , LOCALE_PICTUREVIEWER_RESIZE_COLOR_AVERAGE }
};

void CNeutrinoApp::InitMiscSettings(CMenuWidget &miscSettings)
{
	dprintf(DEBUG_DEBUG, "init miscsettings\n");
	miscSettings.addItem(GenericMenuSeparator);
	miscSettings.addItem(GenericMenuBack);
	miscSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_GENERAL));

	CMenuOptionChooser *m1 = new CMenuOptionChooser(LOCALE_MISCSETTINGS_SHUTDOWN_REAL_RCDELAY, &g_settings.shutdown_real_rcdelay, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, !g_settings.shutdown_real);

	CMiscNotifier* miscNotifier = new CMiscNotifier( m1 );

	miscSettings.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_SHUTDOWN_REAL, &g_settings.shutdown_real, OPTIONS_OFF1_ON0_OPTIONS, OPTIONS_OFF1_ON0_OPTION_COUNT, true, miscNotifier));

	CShutdownCountNotifier *shutdownCountNotifier = new CShutdownCountNotifier;

	CStringInput * miscSettings_shutdown_count = new CStringInput(LOCALE_MISCSETTINGS_SHUTDOWN_COUNT, g_settings.shutdown_count, 3, LOCALE_MISCSETTINGS_SHUTDOWN_COUNT_HINT1, LOCALE_MISCSETTINGS_SHUTDOWN_COUNT_HINT2, "0123456789 ", shutdownCountNotifier);

	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_SHUTDOWN_COUNT, true, g_settings.shutdown_count, miscSettings_shutdown_count));

	miscSettings.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_INFOBAR_SAT_DISPLAY, &g_settings.infobar_sat_display, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

	miscSettings.addItem(new CMenuOptionChooser(LOCALE_INFOVIEWER_SUBCHAN_DISP_POS, &g_settings.infobar_subchan_disp_pos, INFOBAR_SUBCHAN_DISP_POS_OPTIONS, INFOBAR_SUBCHAN_DISP_POS_OPTIONS_COUNT, true));

#ifndef TUXTXT_CFG_STANDALONE
	CTuxtxtCacheNotifier *tuxtxtcacheNotifier = new CTuxtxtCacheNotifier;
	miscSettings.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_TUXTXT_CACHE, &g_settings.tuxtxt_cache, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, tuxtxtcacheNotifier));
#endif

	miscSettings.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_VIRTUAL_ZAP_MODE, &g_settings.virtual_zap_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));
	miscSettings.addItem(new CMenuOptionChooser(LOCALE_MISCSETTINGS_INFOBAR_SHOW, &g_settings.infobar_show, INFOBAR_SHOW_OPTIONS, INFOBAR_SHOW_OPTIONS_COUNT, true));

	CSectionsdConfigNotifier* sectionsdConfigNotifier = new CSectionsdConfigNotifier;
	miscSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_EPG_HEAD));
	CStringInput * miscSettings_epg_cache = new CStringInput(LOCALE_MISCSETTINGS_EPG_CACHE, &g_settings.epg_cache, 2,LOCALE_MISCSETTINGS_EPG_CACHE_HINT1, LOCALE_MISCSETTINGS_EPG_CACHE_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_CACHE, true, g_settings.epg_cache, miscSettings_epg_cache));
	CStringInput * miscSettings_epg_extendedcache = new CStringInput(LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE, &g_settings.epg_extendedcache, 2,LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE_HINT1, LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_EXTENDEDCACHE, true, g_settings.epg_extendedcache, miscSettings_epg_extendedcache));
	CStringInput * miscSettings_epg_old_events = new CStringInput(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, &g_settings.epg_old_events, 2,LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, true, g_settings.epg_old_events, miscSettings_epg_old_events));
	CStringInput * miscSettings_epg_max_events = new CStringInput(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, &g_settings.epg_max_events, 5,LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT2 , "0123456789 ", sectionsdConfigNotifier);
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, true, g_settings.epg_max_events, miscSettings_epg_max_events));
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_DIR, true, g_settings.epg_dir, this, "epgdir"));

	keySetupNotifier = new CKeySetupNotifier;
	CStringInput * keySettings_repeat_genericblocker = new CStringInput(LOCALE_KEYBINDINGMENU_REPEATBLOCKGENERIC, g_settings.repeat_genericblocker, 3, LOCALE_REPEATBLOCKER_HINT_1, LOCALE_REPEATBLOCKER_HINT_2, "0123456789 ", keySetupNotifier);
	CStringInput * keySettings_repeatBlocker = new CStringInput(LOCALE_KEYBINDINGMENU_REPEATBLOCK, g_settings.repeat_blocker, 3, LOCALE_REPEATBLOCKER_HINT_1, LOCALE_REPEATBLOCKER_HINT_2, "0123456789 ", keySetupNotifier);
	keySetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);

	miscSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_RC));
	miscSettings.addItem(new CMenuForwarder(LOCALE_KEYBINDINGMENU_REPEATBLOCK, true, g_settings.repeat_blocker, keySettings_repeatBlocker));
	miscSettings.addItem(new CMenuForwarder(LOCALE_KEYBINDINGMENU_REPEATBLOCKGENERIC, true, g_settings.repeat_genericblocker, keySettings_repeat_genericblocker));
	miscSettings.addItem(m1);

	miscSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_FILEBROWSER_HEAD));
	miscSettings.addItem(new CMenuOptionChooser(LOCALE_FILESYSTEM_IS_UTF8            , &g_settings.filesystem_is_utf8            , MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTIONS, MISCSETTINGS_FILESYSTEM_IS_UTF8_OPTION_COUNT, true ));
	miscSettings.addItem(new CMenuOptionChooser(LOCALE_FILEBROWSER_SHOWRIGHTS        , &g_settings.filebrowser_showrights        , MESSAGEBOX_NO_YES_OPTIONS              , MESSAGEBOX_NO_YES_OPTION_COUNT              , true ));
	miscSettings.addItem(new CMenuOptionChooser(LOCALE_FILEBROWSER_DENYDIRECTORYLEAVE, &g_settings.filebrowser_denydirectoryleave, MESSAGEBOX_NO_YES_OPTIONS              , MESSAGEBOX_NO_YES_OPTION_COUNT              , true ));
}

#define DRIVERSETTINGS_FB_DESTINATION_OPTION_COUNT 3
const CMenuOptionChooser::keyval DRIVERSETTINGS_FB_DESTINATION_OPTIONS[DRIVERSETTINGS_FB_DESTINATION_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_NULL   },
	{ 1, LOCALE_OPTIONS_SERIAL },
	{ 2, LOCALE_OPTIONS_FB     }
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
#ifndef HAVE_DREAMBOX_HARDWARE
#if HAVE_DVB_API_VERSION == 1
	{LOCALE_DRIVERSETTINGS_STARTBHDRIVER , "/var/etc/.bh"            , OPTIONS_OFF0_ON1_OPTIONS },
#endif
	{LOCALE_DRIVERSETTINGS_HWSECTIONS    , "/var/etc/.hw_sections"   , OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_NOAVIAWATCHDOG, "/var/etc/.no_watchdog"   , OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_NOENXWATCHDOG , "/var/etc/.no_enxwatchdog", OPTIONS_OFF1_ON0_OPTIONS },
#endif
	{LOCALE_DRIVERSETTINGS_PMTUPDATE     , "/var/etc/.pmt_update"    , OPTIONS_OFF0_ON1_OPTIONS }
};

void CNeutrinoApp::InitDriverSettings(CMenuWidget &driverSettings)
{
	dprintf(DEBUG_DEBUG, "init driversettings\n");
	driverSettings.addItem(GenericMenuSeparator);
	driverSettings.addItem(GenericMenuBack);
	driverSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVERSETTINGS_DRIVER_BOOT));

#ifndef HAVE_DREAMBOX_HARDWARE
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

		driverSettings.addItem(new CMenuOptionChooser(driver_setting_files[i].name, &(g_settings.misc_option[i]), driver_setting_files[i].options, 2, true, new CTouchFileNotifier(driver_setting_files[i].filename)));
	}

#ifndef HAVE_DREAMBOX_HARDWARE
	driverSettings.addItem(new CMenuOptionChooser(LOCALE_DRIVERSETTINGS_FB_DESTINATION, &g_settings.uboot_console, DRIVERSETTINGS_FB_DESTINATION_OPTIONS, DRIVERSETTINGS_FB_DESTINATION_OPTION_COUNT, true, ConsoleDestinationChanger));
#endif
}

void CNeutrinoApp::InitLanguageSettings(CMenuWidget &languageSettings)
{
	languageSettings.addItem(GenericMenuSeparator);
	languageSettings.addItem(GenericMenuBack);
	languageSettings.addItem(GenericMenuSeparatorLine);

	//search available languages....

	struct dirent **namelist;
	int n;
	//		printf("scanning locale dir now....(perhaps)\n");

	char *pfad[] = {DATADIR "/neutrino/locale","/var/tuxbox/config/locale"};

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
				char * locale = namelist[count]->d_name;
				char * pos = strstr(locale, ".locale");
				if(pos != NULL)
				{
					*pos = '\0';
					CMenuOptionLanguageChooser* oj = new CMenuOptionLanguageChooser((char*)locale, this);
					oj->addOption(locale);
					languageSettings.addItem( oj );
				}
			}
			free(namelist);
		}
	}
}

#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, LOCALE_AUDIOMENU_STEREO    },
	{ 1, LOCALE_AUDIOMENU_MONOLEFT  },
	{ 2, LOCALE_AUDIOMENU_MONORIGHT }
};

#ifndef HAVE_DREAMBOX_HARDWARE
#define AUDIOMENU_AVS_CONTROL_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_AVS_CONTROL_OPTIONS[AUDIOMENU_AVS_CONTROL_OPTION_COUNT] =
{
	{ CControld::TYPE_OST , LOCALE_AUDIOMENU_OST  },
	{ CControld::TYPE_AVS , LOCALE_AUDIOMENU_AVS  },
	{ CControld::TYPE_LIRC, LOCALE_AUDIOMENU_LIRC }
};
#endif

#define AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_LEFT_RIGHT_SELECTABEL_OPTIONS[AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT] =
{
      { true, LOCALE_OPTIONS_ON },
      { false, LOCALE_OPTIONS_OFF }
};

#define AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_OPTIONS[AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT] =
{
        { true, LOCALE_OPTIONS_ON },
        { false, LOCALE_OPTIONS_OFF }
};

void CNeutrinoApp::InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier* audioSetupNotifier)
{
	audioSettings.addItem(GenericMenuSeparator);
	audioSettings.addItem(GenericMenuBack);
	audioSettings.addItem(GenericMenuSeparatorLine);

	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_ANALOGOUT, &g_settings.audio_AnalogMode, AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT, true, audioSetupNotifier);

	audioSettings.addItem( oj );
	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AUDIO_LEFT_RIGHT_SELECTABLE, &g_settings.audio_left_right_selectable, AUDIOMENU_LEFT_RIGHT_SELECTABEL_OPTIONS, AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT, true, audioSetupNotifier);
	audioSettings.addItem( oj );

	audioSettings.addItem(GenericMenuSeparatorLine);

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE, &g_settings.audiochannel_up_down_enable, AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_OPTIONS, AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT, true, audioSetupNotifier);
        audioSettings.addItem( oj );

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_DOLBYDIGITAL, &g_settings.audio_DolbyDigital, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, audioSetupNotifier);
	audioSettings.addItem(oj);

#ifndef HAVE_DREAMBOX_HARDWARE
	audioSettings.addItem(GenericMenuSeparatorLine);

	CStringInput * audio_PCMOffset = new CStringInput(LOCALE_AUDIOMENU_PCMOFFSET, g_settings.audio_PCMOffset, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ", audioSetupNotifier);
	CMenuForwarder *mf = new CMenuForwarder(LOCALE_AUDIOMENU_PCMOFFSET, (g_settings.audio_avs_Control == CControld::TYPE_LIRC), g_settings.audio_PCMOffset, audio_PCMOffset );
	CAudioSetupNotifier2 *audioSetupNotifier2 = new CAudioSetupNotifier2(mf);

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AVS_CONTROL, &g_settings.audio_avs_Control, AUDIOMENU_AVS_CONTROL_OPTIONS, AUDIOMENU_AVS_CONTROL_OPTION_COUNT, true, audioSetupNotifier2);
	audioSettings.addItem(oj);
	audioSettings.addItem(mf);
#endif
}

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

void CNeutrinoApp::InitParentalLockSettings(CMenuWidget &parentallockSettings)
{
	parentallockSettings.addItem(GenericMenuSeparator);
	parentallockSettings.addItem(GenericMenuBack);
	parentallockSettings.addItem(GenericMenuSeparatorLine);

	parentallockSettings.addItem(new CMenuOptionChooser(LOCALE_PARENTALLOCK_PROMPT , &g_settings.parentallock_prompt , PARENTALLOCK_PROMPT_OPTIONS , PARENTALLOCK_PROMPT_OPTION_COUNT , !parentallocked));

	parentallockSettings.addItem(new CMenuOptionChooser(LOCALE_PARENTALLOCK_LOCKAGE, &g_settings.parentallock_lockage, PARENTALLOCK_LOCKAGE_OPTIONS, PARENTALLOCK_LOCKAGE_OPTION_COUNT, !parentallocked));

	CPINChangeWidget * pinChangeWidget = new CPINChangeWidget(LOCALE_PARENTALLOCK_CHANGEPIN, g_settings.parentallock_pincode, 4, LOCALE_PARENTALLOCK_CHANGEPIN_HINT1);
	parentallockSettings.addItem( new CMenuForwarder(LOCALE_PARENTALLOCK_CHANGEPIN, true, g_settings.parentallock_pincode, pinChangeWidget));
}

#define OPTIONS_NTPENABLE_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_NTPENABLE_OPTIONS[OPTIONS_NTPENABLE_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_NTP_OFF },
	{ 1, LOCALE_OPTIONS_NTP_ON }
};

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
	networkSettings.addItem(GenericMenuSeparator);
	networkSettings.addItem(GenericMenuBack);
	networkSettings.addItem(GenericMenuSeparatorLine);

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
	networkSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_NETWORKMENU_MOUNT));
	networkSettings.addItem(new CMenuForwarder(LOCALE_NFS_MOUNT , true, NULL, new CNFSMountGui(), NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	networkSettings.addItem(new CMenuForwarder(LOCALE_NFS_UMOUNT, true, NULL, new CNFSUmountGui(), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
}

#define RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT 4
const CMenuOptionChooser::keyval RECORDINGMENU_RECORDING_TYPE_OPTIONS[RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT] =
{
	{ CNeutrinoApp::RECORDING_OFF   , LOCALE_RECORDINGMENU_OFF    },
	{ CNeutrinoApp::RECORDING_SERVER, LOCALE_RECORDINGMENU_SERVER },
	{ CNeutrinoApp::RECORDING_VCR   , LOCALE_RECORDINGMENU_VCR    },
	{ CNeutrinoApp::RECORDING_FILE  , LOCALE_RECORDINGMENU_FILE   }
};

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

	CMenuOptionChooser* oj4 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STOPSECTIONSD, &g_settings.recording_stopsectionsd, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE));

	CMenuOptionChooser* oj4b = new CMenuOptionChooser(LOCALE_RECORDINGMENU_ZAP_ON_ANNOUNCE, &g_settings.recording_zap_on_announce, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj5 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_NO_SCART, &g_settings.recording_vcr_no_scart, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_VCR));

	CMenuOptionChooser* oj12 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORD_IN_SPTS_MODE, &g_settings.recording_in_spts_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT,(g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE) );

	int pre,post;
	g_Timerd->getRecordingSafety(pre,post);
	sprintf(g_settings.record_safety_time_before, "%02d", pre/60);
	sprintf(g_settings.record_safety_time_after, "%02d", post/60);
	CRecordingSafetyNotifier *RecordingSafetyNotifier = new CRecordingSafetyNotifier;
	CStringInput * timerSettings_record_safety_time_before = new CStringInput(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, g_settings.record_safety_time_before, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_2,"0123456789 ", RecordingSafetyNotifier);
	CMenuForwarder *mf5 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, true, g_settings.record_safety_time_before, timerSettings_record_safety_time_before );

	CStringInput * timerSettings_record_safety_time_after = new CStringInput(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, g_settings.record_safety_time_after, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_2,"0123456789 ", RecordingSafetyNotifier);
	CMenuForwarder *mf6 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, true, g_settings.record_safety_time_after, timerSettings_record_safety_time_after );

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

	CStringInput * recordingSettings_ringbuffers = new CStringInput(LOCALE_RECORDINGMENU_RINGBUFFERS, g_settings.recording_ringbuffers, 2, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
	CMenuForwarder* mf10 = new CMenuForwarder(LOCALE_RECORDINGMENU_RINGBUFFERS, true, g_settings.recording_ringbuffers,recordingSettings_ringbuffers);
	CMenuOptionChooser* oj10 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_CHOOSE_DIRECT_REC_DIR, &g_settings.recording_choose_direct_rec_dir, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj11 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_EPG_FOR_FILENAME, &g_settings.recording_epg_for_filename, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CStringInput * recordingSettings_filenameTemplate = new CStringInput(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, &g_settings.recording_filename_template[0], 21, LOCALE_RECORDINGMENU_FILENAME_TEMPLATE_HINT, LOCALE_IPSETUP_HINT_2, "%/-_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
	CMenuForwarder* mf11 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, true, g_settings.recording_filename_template[0],recordingSettings_filenameTemplate);

	CStringInput * recordingSettings_dirPermissions = new CStringInput(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, g_settings.recording_dir_permissions[0], 3, LOCALE_RECORDINGMENU_DIR_PERMISSIONS_HINT, LOCALE_IPSETUP_HINT_2, "01234567");
	CMenuForwarder* mf12 = new CMenuForwarder(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, true, g_settings.recording_dir_permissions[0],recordingSettings_dirPermissions);

	CRecordingNotifier *RecordingNotifier = new CRecordingNotifier(mf1,mf2,oj2,mf3,oj3,oj4,oj5,mf7,oj12);

	CMenuOptionChooser* oj1 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORDING_TYPE, &g_settings.recording_type, RECORDINGMENU_RECORDING_TYPE_OPTIONS, RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT, true, RecordingNotifier);

	recordingSettings.addItem(GenericMenuSeparator);
	recordingSettings.addItem(GenericMenuBack);
	recordingSettings.addItem(GenericMenuSeparatorLine);
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
	directRecordingSettings->addItem(mf10);
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

void CNeutrinoApp::InitStreamingSettings(CMenuWidget &streamingSettings)
{
	streamingSettings.addItem(GenericMenuSeparator);
	streamingSettings.addItem(GenericMenuBack);
	streamingSettings.addItem(GenericMenuSeparatorLine);

	CIPInput * streamingSettings_server_ip = new CIPInput(LOCALE_STREAMINGMENU_SERVER_IP, g_settings.streaming_server_ip, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	CStringInput * streamingSettings_server_port = new CStringInput(LOCALE_STREAMINGMENU_SERVER_PORT, g_settings.streaming_server_port, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2,"0123456789 ");
 	CStringInputSMS * cddriveInput = new CStringInputSMS(LOCALE_STREAMINGMENU_STREAMING_SERVER_CDDRIVE, g_settings.streaming_server_cddrive, 20, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-:\\ ");
	CStringInput * streamingSettings_videorate = new CStringInput(LOCALE_STREAMINGMENU_STREAMING_VIDEORATE, g_settings.streaming_videorate, 5, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2,"0123456789 ");
	CStringInput * streamingSettings_audiorate = new CStringInput(LOCALE_STREAMINGMENU_STREAMING_AUDIORATE, g_settings.streaming_audiorate, 5, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2,"0123456789 ");
	CStringInputSMS * startdirInput = new CStringInputSMS(LOCALE_STREAMINGMENU_STREAMING_SERVER_STARTDIR, g_settings.streaming_server_startdir, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE,"abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-:\\ ");

	CMenuForwarder* mf1 = new CMenuForwarder(LOCALE_STREAMINGMENU_SERVER_IP                , (g_settings.streaming_type==1), g_settings.streaming_server_ip      , streamingSettings_server_ip);
	CMenuForwarder* mf2 = new CMenuForwarder(LOCALE_STREAMINGMENU_SERVER_PORT              , (g_settings.streaming_type==1), g_settings.streaming_server_port    , streamingSettings_server_port);
	CMenuForwarder* mf3 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_SERVER_CDDRIVE , (g_settings.streaming_type==1), g_settings.streaming_server_cddrive , cddriveInput);
	CMenuForwarder* mf4 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_VIDEORATE      , (g_settings.streaming_type==1), g_settings.streaming_videorate      , streamingSettings_videorate);
	CMenuForwarder* mf5 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_AUDIORATE      , (g_settings.streaming_type==1), g_settings.streaming_audiorate      , streamingSettings_audiorate);
	CMenuForwarder* mf6 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_SERVER_STARTDIR, (g_settings.streaming_type==1), g_settings.streaming_server_startdir, startdirInput);
	CMenuForwarder* mf7 = new CMenuForwarder(LOCALE_MOVIEPLAYER_DEFDIR, true, g_settings.network_nfs_moviedir,this,"moviedir");
 	CMenuForwarder* mf8 = new CMenuForwarder(LOCALE_MOVIEPLAYER_DEFPLUGIN, true, g_settings.movieplayer_plugin,this,"movieplugin");
	CMenuOptionChooser* oj1 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_TRANSCODE_AUDIO      , &g_settings.streaming_transcode_audio      , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);

	CMenuOptionChooser* oj2 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_FORCE_AVI_RAWAUDIO   , &g_settings.streaming_force_avi_rawaudio   , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);

	CMenuOptionChooser* oj3 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_FORCE_TRANSCODE_VIDEO, &g_settings.streaming_force_transcode_video, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);

// not yet supported by VLC
	CMenuOptionChooser* oj4 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC, &g_settings.streaming_transcode_video_codec, STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC_OPTIONS, STREAMINGMENU_STREAMING_TRANSCODE_VIDEO_CODEC_OPTION_COUNT, true);

	CMenuOptionChooser* oj5 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_RESOLUTION           , &g_settings.streaming_resolution           , STREAMINGMENU_STREAMING_RESOLUTION_OPTIONS, STREAMINGMENU_STREAMING_RESOLUTION_OPTION_COUNT, true);

	CStreamingNotifier *StreamingNotifier = new CStreamingNotifier(mf1,mf2,mf3,mf4,mf5,mf6,oj1,oj2,oj3,oj4,oj5);

	CIntInput * streamingSettings_buffer_size = new CIntInput(LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE, (long&)g_settings.streaming_buffer_segment_size,3, LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE_HINT1, LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE_HINT2);
	CMenuForwarder* mf9 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE , g_settings.streaming_use_buffer, streamingSettings_buffer_size->getValue()      , streamingSettings_buffer_size);
	COnOffNotifier *bufferNotifier = new COnOffNotifier(mf9);
	CMenuOptionChooser* oj6 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_USE_BUFFER , &g_settings.streaming_use_buffer  , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true,bufferNotifier);
	CMenuOptionChooser* oj7 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_SHOW_TV_IN_BROWSER , &g_settings.streaming_show_tv_in_browser  , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);
	CMenuOptionChooser* oj8 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_FILEBROWSER_ALLOW_MULTISELECT , &g_settings.streaming_allow_multiselect  , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true);


	streamingSettings.addItem(new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_TYPE                  , &g_settings.streaming_type                 , STREAMINGMENU_STREAMING_TYPE_OPTIONS, STREAMINGMENU_STREAMING_TYPE_OPTION_COUNT, true, StreamingNotifier));
	streamingSettings.addItem(GenericMenuSeparatorLine);
	streamingSettings.addItem( mf1);                          //Server IP
	streamingSettings.addItem( mf2);                          //Server Port
	streamingSettings.addItem( mf3);                          //CD-Drive
	streamingSettings.addItem( mf6);                          //Startdir
	streamingSettings.addItem(GenericMenuSeparatorLine);
	streamingSettings.addItem( mf4);                          //Video-Rate
	streamingSettings.addItem( oj3);
	streamingSettings.addItem( oj4);
	streamingSettings.addItem( oj5);
	streamingSettings.addItem(GenericMenuSeparatorLine);
	streamingSettings.addItem( mf5);                          //Audiorate
	streamingSettings.addItem( oj1);
	streamingSettings.addItem( oj2);
	streamingSettings.addItem(GenericMenuSeparatorLine);
	streamingSettings.addItem( mf7);                          //default dir
	streamingSettings.addItem( mf8);				//default movieplugin
	streamingSettings.addItem(GenericMenuSeparatorLine);
	streamingSettings.addItem( oj6 );
	streamingSettings.addItem( mf9 );
	streamingSettings.addItem( oj7 );
	streamingSettings.addItem( oj8 );
}


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

	int exec(CMenuTarget * parent, const std::string & actionKey)
		{
			CStringInput input(text, (char *)getOption(), 3, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ", this);
			return input.exec(parent, actionKey);
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

void CNeutrinoApp::InitFontSettings(CMenuWidget &fontSettings)
{
	fontSettings.addItem(GenericMenuSeparator);
	fontSettings.addItem(GenericMenuBack);
	fontSettings.addItem(GenericMenuSeparatorLine);
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

void CNeutrinoApp::InitColorSettings(CMenuWidget &colorSettings, CMenuWidget &fontSettings)
{
	colorSettings.addItem(GenericMenuSeparator);
	colorSettings.addItem(GenericMenuBack);
	colorSettings.addItem(GenericMenuSeparatorLine);
	
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

	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_COLORMENU_OSD, &g_settings.widget_osd, OSD_STATUS_OPTIONS, OSD_STATUS_OPTIONS_COUNT, true );
	colorSettings.addItem(oj);

	colorSettings.addItem(GenericMenuSeparatorLine);
#ifndef HAVE_DREAMBOX_HARDWARE
	if ((g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS) || (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM)) // eNX
#endif
	{
		CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_COLORMENU_FADE, &g_settings.widget_fade, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true );
		colorSettings.addItem(oj);
	}
#ifndef HAVE_DREAMBOX_HARDWARE
	else // GTX, ...
	{
		CAlphaSetup* chAlphaSetup = new CAlphaSetup(LOCALE_COLORMENU_GTX_ALPHA, &g_settings.gtx_alpha1, &g_settings.gtx_alpha2);
		colorSettings.addItem( new CMenuForwarder(LOCALE_COLORMENU_GTX_ALPHA, true, NULL, chAlphaSetup, NULL, CRCInput::RC_2));
	}
#endif
}


void CNeutrinoApp::InitColorThemesSettings(CMenuWidget &colorSettings_Themes)
{
	colorSettings_Themes.addItem(GenericMenuSeparator);
	colorSettings_Themes.addItem(GenericMenuBack);
	colorSettings_Themes.addItem(GenericMenuSeparatorLine);
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_NEUTRINO_THEME, true, NULL, this, "theme_neutrino"));
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_CLASSIC_THEME, true, NULL, this, "theme_classic"));
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_VIRGINMEDIA_THEME, true, NULL, this, "theme_virgin"));
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_DBLUE_THEME, true, NULL, this, "theme_dblue"));
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_DBROWN_THEME, true, NULL, this, "theme_dbrown"));
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_DVB2K_THEME, true, NULL, this, "theme_dvb2k"));
}
	

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

void CNeutrinoApp::InitColorSettingsTiming(CMenuWidget &colorSettings_timing)
{
	/* note: SetupTiming() is already called in CNeutrinoApp::run */

	colorSettings_timing.addItem(GenericMenuSeparator);
	colorSettings_timing.addItem(GenericMenuBack);
	colorSettings_timing.addItem(GenericMenuSeparatorLine);

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		CStringInput * colorSettings_timing_item = new CStringInput(timing_setting_name[i], g_settings.timing_string[i], 3, LOCALE_TIMING_HINT_1, LOCALE_TIMING_HINT_2, "0123456789 ", &timingsettingsnotifier);
		colorSettings_timing.addItem(new CMenuForwarder(timing_setting_name[i], true, g_settings.timing_string[i], colorSettings_timing_item));
	}

	colorSettings_timing.addItem(GenericMenuSeparatorLine);
	colorSettings_timing.addItem(new CMenuForwarder(LOCALE_OPTIONS_DEFAULT, true, NULL, this, "osd.def"));
}

#define LCDMENU_STATUSLINE_OPTION_COUNT 4
const CMenuOptionChooser::keyval LCDMENU_STATUSLINE_OPTIONS[LCDMENU_STATUSLINE_OPTION_COUNT] =
{
	{ 0, LOCALE_LCDMENU_STATUSLINE_PLAYTIME   },
	{ 1, LOCALE_LCDMENU_STATUSLINE_VOLUME     },
	{ 2, LOCALE_LCDMENU_STATUSLINE_BOTH       },
	{ 3, LOCALE_LCDMENU_STATUSLINE_BOTH_AUDIO }
};

void CNeutrinoApp::InitLcdSettings(CMenuWidget &lcdSettings)
{
	lcdSettings.addItem(GenericMenuSeparator);

	lcdSettings.addItem(GenericMenuBack);
	lcdSettings.addItem(GenericMenuSeparatorLine);

	CLcdControler* lcdsliders = new CLcdControler(LOCALE_LCDMENU_HEAD, NULL);

	CLcdNotifier* lcdnotifier = new CLcdNotifier();

	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_LCDMENU_INVERSE, &g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE], OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, lcdnotifier);
	lcdSettings.addItem(oj);

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
}

#define KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT 3
const CMenuOptionChooser::keyval KEYBINDINGMENU_BOUQUETHANDLING_OPTIONS[KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT] =
{
	{ 0, LOCALE_KEYBINDINGMENU_BOUQUETCHANNELS_ON_OK },
	{ 1, LOCALE_KEYBINDINGMENU_BOUQUETLIST_ON_OK     },
	{ 2, LOCALE_KEYBINDINGMENU_ALLCHANNELS_ON_OK     }
};

enum keynames {
	KEY_TV_RADIO_MODE,
	KEY_PAGE_UP,
	KEY_PAGE_DOWN,
	KEY_CANCEL_ACTION,
	KEY_SORT,
	KEY_ADD_RECORD,
	KEY_ADD_REMIND,
	KEY_RELOAD,
	KEY_CHANNEL_UP,
	KEY_CHANNEL_DOWN,
	KEY_BOUQUET_UP,
	KEY_BOUQUET_DOWN,
	KEY_SUBCHANNEL_UP,
	KEY_SUBCHANNEL_DOWN,
	KEY_ZAP_HISTORY,
	KEY_LASTCHANNEL,

	MAX_NUM_KEYNAMES
};

const neutrino_locale_t keydescription_head[] =
{
	LOCALE_KEYBINDINGMENU_TVRADIOMODE_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEUP_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_CANCEL_HEAD,
	LOCALE_KEYBINDINGMENU_SORT_HEAD,
	LOCALE_KEYBINDINGMENU_ADDRECORD_HEAD,
	LOCALE_KEYBINDINGMENU_ADDREMIND_HEAD,
	LOCALE_KEYBINDINGMENU_RELOAD_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETUP_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELDOWN_HEAD,
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
	LOCALE_KEYBINDINGMENU_ADDRECORD,
	LOCALE_KEYBINDINGMENU_ADDREMIND,
	LOCALE_KEYBINDINGMENU_RELOAD,
	LOCALE_KEYBINDINGMENU_CHANNELUP,
	LOCALE_KEYBINDINGMENU_CHANNELDOWN,
	LOCALE_KEYBINDINGMENU_BOUQUETUP,
	LOCALE_KEYBINDINGMENU_BOUQUETDOWN,
	LOCALE_KEYBINDINGMENU_SUBCHANNELUP,
	LOCALE_KEYBINDINGMENU_SUBCHANNELDOWN,
	LOCALE_KEYBINDINGMENU_ZAPHISTORY,
	LOCALE_KEYBINDINGMENU_LASTCHANNEL
};

void CNeutrinoApp::InitKeySettings(CMenuWidget &keySettings)
{
	keySettings.addItem(GenericMenuSeparator);
	keySettings.addItem(GenericMenuBack);
	// USERMENU
	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_USERMENU_HEAD));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_RED, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_RED,0)));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_GREEN, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_GREEN,1)));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_YELLOW, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_YELLOW,2)));
	keySettings.addItem(new CMenuForwarder(LOCALE_USERMENU_BUTTON_BLUE, true, NULL, new CUserMenuMenu(LOCALE_USERMENU_BUTTON_BLUE,3)));

	int * keyvalue_p[] =
		{
			&g_settings.key_tvradio_mode,
			&g_settings.key_channelList_pageup,
			&g_settings.key_channelList_pagedown,
			&g_settings.key_channelList_cancel,
			&g_settings.key_channelList_sort,
			&g_settings.key_channelList_addrecord,
			&g_settings.key_channelList_addremind,
			&g_settings.key_channelList_reload,
			&g_settings.key_quickzap_up,
			&g_settings.key_quickzap_down,
			&g_settings.key_bouquet_up,
			&g_settings.key_bouquet_down,
			&g_settings.key_subchannel_up,
			&g_settings.key_subchannel_down,
			&g_settings.key_zaphistory,
			&g_settings.key_lastchannel
		};

	CKeyChooser * keychooser[MAX_NUM_KEYNAMES];

	for (int i = 0; i < MAX_NUM_KEYNAMES; i++)
		keychooser[i] = new CKeyChooser(keyvalue_p[i], keydescription_head[i], NEUTRINO_ICON_SETTINGS);

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_MODECHANGE));
	keySettings.addItem(new CMenuForwarder(keydescription[KEY_TV_RADIO_MODE], true, NULL, keychooser[KEY_TV_RADIO_MODE]));

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_CHANNELLIST));

	CMenuOptionChooser *oj = new CMenuOptionChooser(LOCALE_KEYBINDINGMENU_BOUQUETHANDLING, &g_settings.bouquetlist_mode, KEYBINDINGMENU_BOUQUETHANDLING_OPTIONS, KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT, true );
	keySettings.addItem(oj);

	for (int i = KEY_PAGE_UP; i <= KEY_RELOAD; i++)
		keySettings.addItem(new CMenuForwarder(keydescription[i], true, NULL, keychooser[i]));

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_QUICKZAP));

	for (int i = KEY_CHANNEL_UP; i <= KEY_SUBCHANNEL_DOWN; i++)
		keySettings.addItem(new CMenuForwarder(keydescription[i], true, NULL, keychooser[i]));

	keySettings.addItem(new CMenuForwarder(keydescription[KEY_ZAP_HISTORY], true, NULL, keychooser[KEY_ZAP_HISTORY]));

	keySettings.addItem(new CMenuForwarder(keydescription[KEY_LASTCHANNEL], true, NULL, keychooser[KEY_LASTCHANNEL]));
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
						keyhelper.get(&key, &icon, cnt == 0 ? CRCInput::RC_blue : 0);
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
						keyhelper.get(&key, &icon, cnt == 0 ? CRCInput::RC_blue : 0);
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
				menu_item = new CMenuForwarder(LOCALE_MOVIEBROWSER_HEAD, true, NULL, this->moviePlayerGui, "tsmoviebrowser", key, icon);
				menu->addItem(menu_item, false);
				break;

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
				int dummy = g_Sectionsd->getIsScanningActive();
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

	// -- Add TS Playback to blue button
	StreamFeatureSelector->addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, this->moviePlayerGui, "tsplayback", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), false);

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
	AudioMute(current_muted, true);
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
			menu->addItem(new CMenuForwarderNonLocalized((Latin1_to_UTF8(e->subservice_name)).c_str(), true, NULL, NVODChanger, nvod_id, CRCInput::convertDigitToKey(count)), (count == g_RemoteControl->selected_subchannel));
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

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (Virgin Media)                           *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_virginmedia()
{
	g_settings.menu_Head_alpha = 0;
	g_settings.menu_Head_red   = 0;
	g_settings.menu_Head_green = 0;
	g_settings.menu_Head_blue  = 0;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 100;

	g_settings.menu_Content_alpha = 0;
	g_settings.menu_Content_red   = 0;
	g_settings.menu_Content_green = 0;
	g_settings.menu_Content_blue  = 0;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 20;
	g_settings.menu_Content_Selected_red   = 100;
	g_settings.menu_Content_Selected_green = 85;
	g_settings.menu_Content_Selected_blue  = 0;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 0;
	g_settings.menu_Content_Selected_Text_green  = 0;
	g_settings.menu_Content_Selected_Text_blue   = 0;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 0;
	g_settings.menu_Content_inactive_green = 15;
	g_settings.menu_Content_inactive_blue  = 35;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 100;
	g_settings.menu_Content_inactive_Text_green  = 100;
	g_settings.menu_Content_inactive_Text_blue   = 100;

	g_settings.infobar_alpha = 20;
	g_settings.infobar_red   = 0;
	g_settings.infobar_green = 0;
	g_settings.infobar_blue  = 0;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (classic)                                *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_classic()
{
	g_settings.menu_Head_alpha = 20;
	g_settings.menu_Head_red   =  5;
	g_settings.menu_Head_green = 10;
	g_settings.menu_Head_blue  = 60;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 100;

	g_settings.menu_Content_alpha = 20;
	g_settings.menu_Content_red   = 50;
	g_settings.menu_Content_green = 50;
	g_settings.menu_Content_blue  = 50;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 20;
	g_settings.menu_Content_Selected_red   = 5;
	g_settings.menu_Content_Selected_green = 10;
	g_settings.menu_Content_Selected_blue  = 60;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 100;
	g_settings.menu_Content_Selected_Text_green  = 100;
	g_settings.menu_Content_Selected_Text_blue   = 100;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 50;
	g_settings.menu_Content_inactive_green = 50;
	g_settings.menu_Content_inactive_blue  = 50;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 80;
	g_settings.menu_Content_inactive_Text_green  = 80;
	g_settings.menu_Content_inactive_Text_blue   = 80;

	g_settings.infobar_alpha = 20;
	g_settings.infobar_red   = 5;
	g_settings.infobar_green = 10;
	g_settings.infobar_blue  = 60;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (darkblue)                               *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_dblue()
{
	g_settings.menu_Head_alpha = 0;
	g_settings.menu_Head_red   = 0;
	g_settings.menu_Head_green = 0;
	g_settings.menu_Head_blue  = 50;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 95;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 100;

	g_settings.menu_Content_alpha = 20;
	g_settings.menu_Content_red   = 0;
	g_settings.menu_Content_green = 0;
	g_settings.menu_Content_blue  = 20;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 15;
	g_settings.menu_Content_Selected_red   = 0;
	g_settings.menu_Content_Selected_green = 65;
	g_settings.menu_Content_Selected_blue  = 0;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 0;
	g_settings.menu_Content_Selected_Text_green  = 0;
	g_settings.menu_Content_Selected_Text_blue   = 0;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 0;
	g_settings.menu_Content_inactive_green = 0;
	g_settings.menu_Content_inactive_blue  = 15;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 55;
	g_settings.menu_Content_inactive_Text_green  = 70;
	g_settings.menu_Content_inactive_Text_blue   = 85;

	g_settings.infobar_alpha = 20;
	g_settings.infobar_red   = 0;
	g_settings.infobar_green = 0;
	g_settings.infobar_blue  = 20;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (dvb2000)                                *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_dvb2k()
{
	g_settings.menu_Head_alpha = 0;
	g_settings.menu_Head_red   = 25;
	g_settings.menu_Head_green = 25;
	g_settings.menu_Head_blue  = 25;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 100;
	g_settings.menu_Head_Text_green = 100;
	g_settings.menu_Head_Text_blue  = 0;

	g_settings.menu_Content_alpha = 0;
	g_settings.menu_Content_red   = 0;
	g_settings.menu_Content_green = 20;
	g_settings.menu_Content_blue  = 0;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 0;
	g_settings.menu_Content_Selected_red   = 100;
	g_settings.menu_Content_Selected_green = 100;
	g_settings.menu_Content_Selected_blue  = 100;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 0;
	g_settings.menu_Content_Selected_Text_green  = 0;
	g_settings.menu_Content_Selected_Text_blue   = 0;

	g_settings.menu_Content_inactive_alpha = 20;
	g_settings.menu_Content_inactive_red   = 0;
	g_settings.menu_Content_inactive_green = 25;
	g_settings.menu_Content_inactive_blue  = 0;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 100;
	g_settings.menu_Content_inactive_Text_green  = 100;
	g_settings.menu_Content_inactive_Text_blue   = 0;

	g_settings.infobar_alpha = 5;
	g_settings.infobar_red   = 0;
	g_settings.infobar_green = 19;
	g_settings.infobar_blue  = 0;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (darkbrown)                              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_dbrown()
{
	g_settings.menu_Head_alpha = 0;
	g_settings.menu_Head_red   = 25;
	g_settings.menu_Head_green = 10;
	g_settings.menu_Head_blue  = 10;

	g_settings.menu_Head_Text_alpha = 0;
	g_settings.menu_Head_Text_red   = 65;
	g_settings.menu_Head_Text_green = 65;
	g_settings.menu_Head_Text_blue  = 65;

	g_settings.menu_Content_alpha = 5;
	g_settings.menu_Content_red   = 20;
	g_settings.menu_Content_green = 20;
	g_settings.menu_Content_blue  = 20;

	g_settings.menu_Content_Text_alpha = 0;
	g_settings.menu_Content_Text_red   = 100;
	g_settings.menu_Content_Text_green = 100;
	g_settings.menu_Content_Text_blue  = 100;

	g_settings.menu_Content_Selected_alpha = 5;
	g_settings.menu_Content_Selected_red   = 55;
	g_settings.menu_Content_Selected_green = 55;
	g_settings.menu_Content_Selected_blue  = 55;

	g_settings.menu_Content_Selected_Text_alpha  = 0;
	g_settings.menu_Content_Selected_Text_red    = 0;
	g_settings.menu_Content_Selected_Text_green  = 0;
	g_settings.menu_Content_Selected_Text_blue   = 0;

	g_settings.menu_Content_inactive_alpha = 5;
	g_settings.menu_Content_inactive_red   = 20;
	g_settings.menu_Content_inactive_green = 20;
	g_settings.menu_Content_inactive_blue  = 20;

	g_settings.menu_Content_inactive_Text_alpha  = 0;
	g_settings.menu_Content_inactive_Text_red    = 60;
	g_settings.menu_Content_inactive_Text_green  = 60;
	g_settings.menu_Content_inactive_Text_blue   = 60;

	g_settings.infobar_alpha = 0;
	g_settings.infobar_red   = 25;
	g_settings.infobar_green = 10;
	g_settings.infobar_blue  = 10;

	g_settings.infobar_Text_alpha = 0;
	g_settings.infobar_Text_red   = 100;
	g_settings.infobar_Text_green = 100;
	g_settings.infobar_Text_blue  = 100;
}
