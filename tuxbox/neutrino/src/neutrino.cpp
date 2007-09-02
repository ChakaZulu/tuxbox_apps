/*
	$Id: neutrino.cpp,v 1.870 2007/09/02 20:40:59 houdini Exp $
	
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define NEUTRINO_CPP

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>

#include <sys/socket.h>

#include <iostream>
#include <fstream>
#include <string>


#include "global.h"
#include "neutrino.h"

#include <daemonc/remotecontrol.h>

#include <driver/encoding.h>
#include <driver/framebuffer.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>
#include <driver/stream2file.h>
#include <driver/vcrcontrol.h>
#include <driver/shutdown_count.h>
#include <irsend/irsend.h>

#include "gui/widget/dirchooser.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/icons.h"
#include "gui/widget/rgbcsynccontroler.h"
#include "gui/widget/messagebox.h"

#include "gui/audioplayer.h"
#include "gui/bouquetlist.h"
#include "gui/movieplayer.h"
#include "gui/nfs.h"
#include "gui/screensetup.h"

#include <system/setting_helpers.h>
#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>
#include <system/fsmounter.h>

#include <timerdclient/timerdmsg.h>

#include <string.h>

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_init();
#endif

CBouquetList    	* bouquetList;
CBouquetList    	* bouquetListTV;
CBouquetList    	* bouquetListRADIO;
CBouquetList    	* bouquetListRecord;
CAPIDChangeExec		* APIDChanger;
CAudioSetupNotifier	* audioSetupNotifier;

// Globale Variablen - to use import global.h
static char **global_argv;

extern font_sizes_groups font_sizes_groups[];

// USERMENU
const char* usermenu_button_def[SNeutrinoSettings::BUTTON_MAX]={"red","green","yellow","blue"};

CVCRControl::CDevice * recordingdevice = NULL;

#define NEUTRINO_SETTINGS_FILE          CONFIGDIR "/neutrino.conf"
#define NEUTRINO_RECORDING_TIMER_SCRIPT CONFIGDIR "/recording.timer"
#define NEUTRINO_RECORDING_START_SCRIPT CONFIGDIR "/recording.start"
#define NEUTRINO_RECORDING_ENDED_SCRIPT CONFIGDIR "/recording.end"
#define NEUTRINO_ENTER_STANDBY_SCRIPT   CONFIGDIR "/standby.on"
#define NEUTRINO_LEAVE_STANDBY_SCRIPT   CONFIGDIR "/standby.off"
#define NEUTRINO_SCAN_SETTINGS_FILE     CONFIGDIR "/scan.conf"
#define NEUTRINO_DEFAULTLOCALE_FILE     CONFIGDIR "/defaultlocale"
#define NEUTRINO_PARENTALLOCKED_FILE    DATADIR   "/neutrino/.plocked"

static void initGlobals(void)
{
	g_fontRenderer  = NULL;

	g_RCInput       = NULL;
	g_Controld      = new CControldClient;
	g_Timerd        = NULL;
	g_Zapit         = new CZapitClient;
	g_RemoteControl = NULL;

	g_EpgData       = NULL;
	g_InfoViewer    = NULL;
	g_EventList     = NULL;

	g_Locale        = new CLocaleManager;
	g_PluginList    = NULL;
}

static void execute_start_file(const char *filename)
{
	struct stat statbuf;
	if (stat(filename, &statbuf) == 0)
	{
		printf("[neutrino] executing %s\n", filename);
		int result = system(filename);
		if (result)
			printf("[neutrino] %s failed with return code = %d\n", filename, WEXITSTATUS(result));
	} else
		printf("[neutrino] no file %s was found\n", filename);
}


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                                                     +
+          CNeutrinoApp - Constructor, initialize g_fontRenderer                      +
+                                                                                     +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
CNeutrinoApp::CNeutrinoApp()
: configfile('\t'),
	recordingstatus(0)
{
	standby_pressed_at.tv_sec = 0;

	frameBuffer = CFrameBuffer::getInstance();
	frameBuffer->setIconBasePath(DATADIR "/neutrino/icons/");

	SetupFrameBuffer();

	mode = mode_unknown;
	channelList       = NULL;
	bouquetList       = NULL;
	channelListTV     = NULL;
	bouquetListTV     = NULL;
	channelListRADIO  = NULL;
	bouquetListRADIO  = NULL;
	channelListRecord = NULL;
	bouquetListRecord = NULL;
	nextRecordingInfo = NULL;
	skipShutdownTimer = false;
	parentallocked    = false;
	waitforshutdown   = false;
}

/*-------------------------------------------------------------------------------------
-                                                                                     -
-           CNeutrinoApp - Destructor                                                 -
-                                                                                     -
-------------------------------------------------------------------------------------*/
CNeutrinoApp::~CNeutrinoApp()
{
	if (channelListTV)
		delete channelListTV;
	if (channelListRADIO)
		delete channelListRADIO;
	if (channelListRecord)
		delete channelListRecord;
	if (bouquetListTV)
		delete bouquetListTV;
	if (bouquetListRADIO)
		delete bouquetListRADIO;
	if (bouquetListRecord)
		delete bouquetListRecord;
}

CNeutrinoApp* CNeutrinoApp::getInstance()
{
	static CNeutrinoApp* neutrinoApp = NULL;

	if(!neutrinoApp)
	{
		neutrinoApp = new CNeutrinoApp();
		dprintf(DEBUG_DEBUG, "NeutrinoApp Instance created\n");
	}
	return neutrinoApp;
}


const font_sizes_struct neutrino_font[FONT_TYPE_COUNT] =
{
	{LOCALE_FONTSIZE_MENU               ,  20, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_MENU_TITLE         ,  30, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_MENU_INFO          ,  16, FONT_STYLE_REGULAR, 0},
	{LOCALE_FONTSIZE_EPG_TITLE          ,  25, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_EPG_INFO1          ,  17, FONT_STYLE_ITALIC , 2},
	{LOCALE_FONTSIZE_EPG_INFO2          ,  17, FONT_STYLE_REGULAR, 2},
	{LOCALE_FONTSIZE_EPG_DATE           ,  15, FONT_STYLE_REGULAR, 2},
	{LOCALE_FONTSIZE_EVENTLIST_TITLE    ,  30, FONT_STYLE_REGULAR, 0},
	{LOCALE_FONTSIZE_EVENTLIST_ITEMLARGE,  20, FONT_STYLE_BOLD   , 1},
	{LOCALE_FONTSIZE_EVENTLIST_ITEMSMALL,  14, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_EVENTLIST_DATETIME ,  16, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_GAMELIST_ITEMLARGE ,  20, FONT_STYLE_BOLD   , 1},
	{LOCALE_FONTSIZE_GAMELIST_ITEMSMALL ,  16, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_CHANNELLIST        ,  20, FONT_STYLE_BOLD   , 1},
	{LOCALE_FONTSIZE_CHANNELLIST_DESCR  ,  20, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_CHANNELLIST_NUMBER ,  14, FONT_STYLE_BOLD   , 2},
	{LOCALE_FONTSIZE_CHANNEL_NUM_ZAP    ,  40, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_INFOBAR_NUMBER     ,  50, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_INFOBAR_CHANNAME   ,  30, FONT_STYLE_BOLD   , 0},
	{LOCALE_FONTSIZE_INFOBAR_INFO       ,  20, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_INFOBAR_SMALL      ,  14, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_FILEBROWSER_ITEM   ,  16, FONT_STYLE_BOLD   , 1},
	{LOCALE_FONTSIZE_IMAGEINFO_INFO     ,  18, FONT_STYLE_REGULAR, 1},
	{LOCALE_FONTSIZE_IMAGEINFO_SMALL    ,  15, FONT_STYLE_REGULAR, 1}
};

typedef struct lcd_setting_t
{
	const char * const name;
	const unsigned int default_value;
} lcd_setting_struct_t;

const lcd_setting_struct_t lcd_setting[LCD_SETTING_COUNT] =
{
	{"lcd_brightness"       , DEFAULT_LCD_BRIGHTNESS       },
	{"lcd_standbybrightness", DEFAULT_LCD_STANDBYBRIGHTNESS},
	{"lcd_contrast"         , DEFAULT_LCD_CONTRAST         },
	{"lcd_power"            , DEFAULT_LCD_POWER            },
	{"lcd_inverse"          , DEFAULT_LCD_INVERSE          },
	{"lcd_show_volume"      , DEFAULT_LCD_SHOW_VOLUME      },
	{"lcd_autodimm"         , DEFAULT_LCD_AUTODIMM         }
};


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  loadSetup, load the application-settings                   *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::loadSetup()
{
	int erg = 0;

	//settings laden - und dabei Defaults setzen!
	if(!configfile.loadConfig(NEUTRINO_SETTINGS_FILE))
	{
		//file existiert nicht
		erg = 1;
	}
        std::ifstream checkParentallocked(NEUTRINO_PARENTALLOCKED_FILE);
	if(checkParentallocked)
	{
	        parentallocked = true;
	        checkParentallocked.close();
	}

	//video
	g_settings.video_Format = configfile.getInt32("video_Format", CControldClient::VIDEOFORMAT_4_3);
	g_settings.video_csync = configfile.getInt32( "video_csync", 0 );

	//fb-alphavalues for gtx
	g_settings.gtx_alpha1 = configfile.getInt32( "gtx_alpha1", 0);
	g_settings.gtx_alpha2 = configfile.getInt32( "gtx_alpha2", 1);

	// EPG-Config
	g_settings.epg_cache 		= configfile.getString("epg_cache_time", "14");
	g_settings.epg_extendedcache	= configfile.getString("epg_extendedcache_time", "6");
	g_settings.epg_old_events 	= configfile.getString("epg_old_events", "1");
	g_settings.epg_max_events 	= configfile.getString("epg_max_events", "6000");
	g_settings.epg_dir 		= configfile.getString("epg_dir", "");
        // NTP-Server for sectionsd
	g_settings.network_ntpserver	= configfile.getString("network_ntpserver", "de.pool.ntp.org");
	g_settings.network_ntprefresh	= configfile.getString("network_ntprefresh", "30" );
	g_settings.network_ntpenable 	= configfile.getBool("network_ntpenable", false);

	//misc
	g_settings.shutdown_real		= configfile.getBool("shutdown_real"             , true );
	g_settings.shutdown_real_rcdelay	= configfile.getBool("shutdown_real_rcdelay"     , true );
	strcpy(g_settings.shutdown_count, configfile.getString("shutdown_count","0").c_str());
	g_settings.infobar_sat_display		= configfile.getBool("infobar_sat_display"       , true );
	g_settings.infobar_subchan_disp_pos	= configfile.getInt32("infobar_subchan_disp_pos" , 0 );
	g_settings.misc_spts			= configfile.getBool("misc_spts"                 , false );
#ifndef TUXTXT_CFG_STANDALONE
	g_settings.tuxtxt_cache			= configfile.getBool("tuxtxt_cache"              , false );
#endif
	g_settings.virtual_zap_mode		= configfile.getBool("virtual_zap_mode"          , false);
	g_settings.infobar_show			= configfile.getInt32("infobar_show"             , 0);

	//audio
	g_settings.audio_AnalogMode 		= configfile.getInt32( "audio_AnalogMode"        , 0 );
	g_settings.audio_DolbyDigital		= configfile.getBool("audio_DolbyDigital"        , false);
#ifndef HAVE_DREAMBOX_HARDWARE
	g_settings.audio_avs_Control 		= configfile.getInt32( "audio_avs_Control", CControld::TYPE_AVS );
#else
	g_settings.audio_avs_Control 		= CControld::TYPE_OST;
#endif
	strcpy( g_settings.audio_PCMOffset, configfile.getString( "audio_PCMOffset", "0" ).c_str() );

	//vcr
	g_settings.vcr_AutoSwitch		= configfile.getBool("vcr_AutoSwitch"            , true );

	//language
	strcpy(g_settings.language, configfile.getString("language", "").c_str());

	//widget settings
	g_settings.widget_fade           = configfile.getBool("widget_fade"          , true );
	g_settings.widget_osd            = configfile.getInt32("widget_osd"          , true );

	//colors (neutrino defaultcolors)
	g_settings.menu_Head_alpha = configfile.getInt32( "menu_Head_alpha", 0x00 );
	g_settings.menu_Head_red = configfile.getInt32( "menu_Head_red", 0x00 );
	g_settings.menu_Head_green = configfile.getInt32( "menu_Head_green", 0x0A );
	g_settings.menu_Head_blue = configfile.getInt32( "menu_Head_blue", 0x19 );

	g_settings.menu_Head_Text_alpha = configfile.getInt32( "menu_Head_Text_alpha", 0x00 );
	g_settings.menu_Head_Text_red = configfile.getInt32( "menu_Head_Text_red", 0x5f );
	g_settings.menu_Head_Text_green = configfile.getInt32( "menu_Head_Text_green", 0x46 );
	g_settings.menu_Head_Text_blue = configfile.getInt32( "menu_Head_Text_blue", 0x00 );

	g_settings.menu_Content_alpha = configfile.getInt32( "menu_Content_alpha", 0x14 );
	g_settings.menu_Content_red = configfile.getInt32( "menu_Content_red", 0x00 );
	g_settings.menu_Content_green = configfile.getInt32( "menu_Content_green", 0x0f );
	g_settings.menu_Content_blue = configfile.getInt32( "menu_Content_blue", 0x23 );

	g_settings.menu_Content_Text_alpha = configfile.getInt32( "menu_Content_Text_alpha", 0x00 );
	g_settings.menu_Content_Text_red = configfile.getInt32( "menu_Content_Text_red", 0x64 );
	g_settings.menu_Content_Text_green = configfile.getInt32( "menu_Content_Text_green", 0x64 );
	g_settings.menu_Content_Text_blue = configfile.getInt32( "menu_Content_Text_blue", 0x64 );

	g_settings.menu_Content_Selected_alpha = configfile.getInt32( "menu_Content_Selected_alpha", 0x14 );
	g_settings.menu_Content_Selected_red = configfile.getInt32( "menu_Content_Selected_red", 0x19 );
	g_settings.menu_Content_Selected_green = configfile.getInt32( "menu_Content_Selected_green", 0x37 );
	g_settings.menu_Content_Selected_blue = configfile.getInt32( "menu_Content_Selected_blue", 0x64 );

	g_settings.menu_Content_Selected_Text_alpha = configfile.getInt32( "menu_Content_Selected_Text_alpha", 0x00 );
	g_settings.menu_Content_Selected_Text_red = configfile.getInt32( "menu_Content_Selected_Text_red", 0x00 );
	g_settings.menu_Content_Selected_Text_green = configfile.getInt32( "menu_Content_Selected_Text_green", 0x00 );
	g_settings.menu_Content_Selected_Text_blue = configfile.getInt32( "menu_Content_Selected_Text_blue", 0x00 );

	g_settings.menu_Content_inactive_alpha = configfile.getInt32( "menu_Content_inactive_alpha", 0x14 );
	g_settings.menu_Content_inactive_red = configfile.getInt32( "menu_Content_inactive_red", 0x00 );
	g_settings.menu_Content_inactive_green = configfile.getInt32( "menu_Content_inactive_green", 0x0f );
	g_settings.menu_Content_inactive_blue = configfile.getInt32( "menu_Content_inactive_blue", 0x23 );

	g_settings.menu_Content_inactive_Text_alpha = configfile.getInt32( "menu_Content_inactive_Text_alpha", 0x00 );
	g_settings.menu_Content_inactive_Text_red = configfile.getInt32( "menu_Content_inactive_Text_red", 55 );
	g_settings.menu_Content_inactive_Text_green = configfile.getInt32( "menu_Content_inactive_Text_green", 70 );
	g_settings.menu_Content_inactive_Text_blue = configfile.getInt32( "menu_Content_inactive_Text_blue", 85 );

	g_settings.infobar_alpha = configfile.getInt32( "infobar_alpha", 0x14 );
	g_settings.infobar_red = configfile.getInt32( "infobar_red", 0x00 );
	g_settings.infobar_green = configfile.getInt32( "infobar_green", 0x0e );
	g_settings.infobar_blue = configfile.getInt32( "infobar_blue", 0x23 );

	g_settings.infobar_Text_alpha = configfile.getInt32( "infobar_Text_alpha", 0x00 );
	g_settings.infobar_Text_red = configfile.getInt32( "infobar_Text_red", 0x64 );
	g_settings.infobar_Text_green = configfile.getInt32( "infobar_Text_green", 0x64 );
	g_settings.infobar_Text_blue = configfile.getInt32( "infobar_Text_blue", 0x64 );

	//network
	char cfg_key[81];
	for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
	{
		sprintf(cfg_key, "network_nfs_ip_%d", i);
		g_settings.network_nfs_ip[i] = configfile.getString(cfg_key, "");
		sprintf(cfg_key, "network_nfs_dir_%d", i);
		strcpy( g_settings.network_nfs_dir[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_local_dir_%d", i);
		strcpy( g_settings.network_nfs_local_dir[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_automount_%d", i);
		g_settings.network_nfs_automount[i] = configfile.getInt32( cfg_key, 0);
		sprintf(cfg_key, "network_nfs_type_%d", i);
		g_settings.network_nfs_type[i] = configfile.getInt32( cfg_key, 0);
		sprintf(cfg_key, "network_nfs_username_%d", i);
		strcpy( g_settings.network_nfs_username[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_password_%d", i);
		strcpy( g_settings.network_nfs_password[i], configfile.getString( cfg_key, "" ).c_str() );
		sprintf(cfg_key, "network_nfs_mount_options1_%d", i);
		strcpy( g_settings.network_nfs_mount_options1[i], configfile.getString( cfg_key, "ro,soft,udp" ).c_str() );
		sprintf(cfg_key, "network_nfs_mount_options2_%d", i);
		strcpy( g_settings.network_nfs_mount_options2[i], configfile.getString( cfg_key, "nolock,rsize=8192,wsize=8192" ).c_str() );
		sprintf(cfg_key, "network_nfs_mac_%d", i);
		strcpy( g_settings.network_nfs_mac[i], configfile.getString( cfg_key, "11:22:33:44:55:66").c_str() );
	}
	strcpy( g_settings.network_nfs_audioplayerdir, configfile.getString( "network_nfs_audioplayerdir", "" ).c_str() );
	strcpy( g_settings.network_nfs_picturedir, configfile.getString( "network_nfs_picturedir", "" ).c_str() );
	strcpy( g_settings.network_nfs_moviedir, configfile.getString( "network_nfs_moviedir", "" ).c_str() );
	g_settings.filesystem_is_utf8              = configfile.getBool("filesystem_is_utf8"                 , true );

	// Personalization
	g_settings.personalize_pinstatus = configfile.getInt32("personalize_pinstatus", 0);
	strcpy( g_settings.personalize_pincode, configfile.getString( "personalize_pincode", "0000" ).c_str() );
	g_settings.personalize_bluebutton = configfile.getInt32("personalize_bluebutton", 1);
	g_settings.personalize_redbutton = configfile.getInt32("personalize_redbutton", 1);

	g_settings.personalize_tvmode = configfile.getInt32("personalize_tvmode", 1);
	g_settings.personalize_radiomode = configfile.getInt32("personalize_radiomode", 1);
	g_settings.personalize_scartmode = configfile.getInt32("personalize_scartmode", 1);
	g_settings.personalize_games = configfile.getInt32("personalize_games", 1);
	g_settings.personalize_audioplayer = configfile.getInt32("personalize_audioplayer", 1);
	g_settings.personalize_movieplayer = configfile.getInt32("personalize_movieplayer", 1);
	g_settings.personalize_pictureviewer = configfile.getInt32("personalize_pictureviewer", 1);
#if ENABLE_UPNP
 	g_settings.personalize_upnpbrowser = configfile.getInt32("personalize_upnpbrowser", 1);
#endif
	g_settings.personalize_settings = configfile.getInt32("personalize_settings", 1);
	g_settings.personalize_service = configfile.getInt32("personalize_service", 1);
	g_settings.personalize_sleeptimer = configfile.getInt32("personalize_sleeptimer", 1);
	g_settings.personalize_reboot = configfile.getInt32("personalize_reboot", 1);
	g_settings.personalize_shutdown = configfile.getInt32("personalize_shutdown", 1);

	g_settings.personalize_bouqueteditor = configfile.getInt32("personalize_bouqueteditor", 1);
	g_settings.personalize_scants = configfile.getInt32("personalize_scants", 1);
	g_settings.personalize_reload = configfile.getInt32("personalize_reload", 1);
	g_settings.personalize_getplugins = configfile.getInt32("personalize_getplugins", 1);
	g_settings.personalize_restart = configfile.getInt32("personalize_restart", 1);
	g_settings.personalize_ucodecheck = configfile.getInt32("personalize_ucodecheck", 1);
	g_settings.personalize_imageinfo = configfile.getInt32("personalize_imageinfo", 1);
	g_settings.personalize_update = configfile.getInt32("personalize_update", 1);

	g_settings.personalize_audio = configfile.getInt32("personalize_audio", 1);
	g_settings.personalize_video = configfile.getInt32("personalize_video", 1);
	g_settings.personalize_youth = configfile.getInt32("personalize_youth", 1);
	g_settings.personalize_network = configfile.getInt32("personalize_network", 1);
	g_settings.personalize_recording = configfile.getInt32("personalize_recording", 1);
	g_settings.personalize_streaming = configfile.getInt32("personalize_streaming", 1);
	g_settings.personalize_keybinding = configfile.getInt32("personalize_keybinding", 1);
	g_settings.personalize_language = configfile.getInt32("personalize_language", 1);
	g_settings.personalize_colors = configfile.getInt32("personalize_colors", 1);
	g_settings.personalize_lcd = configfile.getInt32("personalize_lcd", 1);
	g_settings.personalize_audpic = configfile.getInt32("personalize_audpic", 1);
	g_settings.personalize_driver = configfile.getInt32("personalize_driver", 1);
	g_settings.personalize_misc = configfile.getInt32("personalize_misc", 1);

	//recording (server + vcr)
	g_settings.recording_type = configfile.getInt32("recording_type", RECORDING_OFF);
	g_settings.recording_stopplayback = configfile.getBool("recording_stopplayback", false);
	g_settings.recording_stopsectionsd = configfile.getBool("recording_stopsectionsd", true );
	g_settings.recording_server_ip = configfile.getString("recording_server_ip", "10.10.10.10");
	strcpy( g_settings.recording_server_port, configfile.getString( "recording_server_port", "4000").c_str() );
	g_settings.recording_server_wakeup = configfile.getInt32( "recording_server_wakeup", 0 );
	strcpy( g_settings.recording_server_mac, configfile.getString( "recording_server_mac", "11:22:33:44:55:66").c_str() );
	g_settings.recording_vcr_no_scart = configfile.getInt32( "recording_vcr_no_scart", false);
	strcpy( g_settings.recording_splitsize, configfile.getString( "recording_splitsize", "2048").c_str() );
	g_settings.recording_use_o_sync            = configfile.getBool("recordingmenu.use_o_sync"           , false);
	g_settings.recording_use_fdatasync         = configfile.getBool("recordingmenu.use_fdatasync"        , false);
	g_settings.recording_audio_pids_default    = configfile.getInt32("recording_audio_pids_default", TIMERD_APIDS_STD );
	g_settings.recording_stream_vtxt_pid       = configfile.getBool("recordingmenu.stream_vtxt_pid"      , false);
	g_settings.recording_stream_subtitle_pid        = configfile.getBool("recordingmenu.stream_subtitle_pid"      , false);
	strcpy( g_settings.recording_ringbuffers, configfile.getString( "recordingmenu.ringbuffers", "20").c_str() );
	g_settings.recording_choose_direct_rec_dir = configfile.getInt32( "recording_choose_direct_rec_dir", 0 );
	g_settings.recording_epg_for_filename      = configfile.getBool("recording_epg_for_filename"         , true);
	g_settings.recording_in_spts_mode          = configfile.getBool("recording_in_spts_mode"         , true);
	g_settings.recording_zap_on_announce       = configfile.getBool("recording_zap_on_announce"      , false);
	for(int i=0 ; i < REC_FILENAME_TEMPLATE_NR_OF_ENTRIES; i++)
	{
		sprintf(cfg_key, "recording_filename_template_%d", i);
		g_settings.recording_filename_template[i] = configfile.getString(cfg_key, "%C_%T_%d_%t");
		sprintf(cfg_key, "recording_dir_permissions_%d", i);
		strncpy(g_settings.recording_dir_permissions[i], configfile.getString(cfg_key,"755").c_str(),3);
		g_settings.recording_dir_permissions[i][3] = '\0';
	}

	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		sprintf(cfg_key, "recording_dir_%d", i);
		g_settings.recording_dir[i] = configfile.getString( cfg_key, "" );
	}
	//streaming (server)
	g_settings.streaming_type = configfile.getInt32( "streaming_type", 0 );
	g_settings.streaming_server_ip = configfile.getString("streaming_server_ip", "10.10.10.10");
	strcpy( g_settings.streaming_server_port, configfile.getString( "streaming_server_port", "8080").c_str() );
	strcpy( g_settings.streaming_server_cddrive, configfile.getString("streaming_server_cddrive", "D:").c_str() );
	strcpy( g_settings.streaming_videorate,  configfile.getString("streaming_videorate", "1000").c_str() );
	strcpy( g_settings.streaming_audiorate, configfile.getString("streaming_audiorate", "192").c_str() );
	strcpy( g_settings.streaming_server_startdir, configfile.getString("streaming_server_startdir", "C:/Movies").c_str() );
	g_settings.streaming_transcode_audio = configfile.getInt32( "streaming_transcode_audio", 0 );
	g_settings.streaming_force_transcode_video = configfile.getInt32( "streaming_force_transcode_video", 0 );
	g_settings.streaming_transcode_video_codec = configfile.getInt32( "streaming_transcode_video_codec", 0 );
	g_settings.streaming_force_avi_rawaudio = configfile.getInt32( "streaming_force_avi_rawaudio", 0 );
	g_settings.streaming_resolution = configfile.getInt32( "streaming_resolution", 0 );
	g_settings.streaming_use_buffer = configfile.getInt32("streaming_use_buffer", 1);
	g_settings.streaming_buffer_segment_size = configfile.getInt32("streaming_buffer_segment_size", 24);
	g_settings.streaming_show_tv_in_browser = configfile.getInt32("streaming_show_tv_in_browser", 0);
	g_settings.streaming_allow_multiselect = configfile.getBool("streaming_allow_multiselect", false);

	// default plugin for movieplayer
	g_settings.movieplayer_plugin = configfile.getString( "movieplayer_plugin", "Teletext" );

	//rc-key configuration
	g_settings.key_tvradio_mode = configfile.getInt32( "key_tvradio_mode", CRCInput::RC_nokey );

	g_settings.key_channelList_pageup = configfile.getInt32( "key_channelList_pageup",  CRCInput::RC_minus );
	g_settings.key_channelList_pagedown = configfile.getInt32( "key_channelList_pagedown", CRCInput::RC_plus );
	g_settings.key_channelList_cancel = configfile.getInt32( "key_channelList_cancel",  CRCInput::RC_home );
	g_settings.key_channelList_sort = configfile.getInt32( "key_channelList_sort",  CRCInput::RC_blue );
	g_settings.key_channelList_addrecord = configfile.getInt32( "key_channelList_addrecord",  CRCInput::RC_nokey );
	g_settings.key_channelList_addremind = configfile.getInt32( "key_channelList_addremind",  CRCInput::RC_nokey );
	g_settings.key_channelList_reload = configfile.getInt32( "key_channelList_reload",  CRCInput::RC_nokey );

	g_settings.key_quickzap_up = configfile.getInt32( "key_quickzap_up",  CRCInput::RC_up );
	g_settings.key_quickzap_down = configfile.getInt32( "key_quickzap_down",  CRCInput::RC_down );
	g_settings.key_bouquet_up = configfile.getInt32( "key_bouquet_up",  CRCInput::RC_right );
	g_settings.key_bouquet_down = configfile.getInt32( "key_bouquet_down",  CRCInput::RC_left );
	g_settings.key_subchannel_up = configfile.getInt32( "key_subchannel_up",  CRCInput::RC_right );
	g_settings.key_subchannel_down = configfile.getInt32( "key_subchannel_down",  CRCInput::RC_left );
	g_settings.key_zaphistory = configfile.getInt32( "key_zaphistory",  CRCInput::RC_home );
	g_settings.key_lastchannel = configfile.getInt32( "key_lastchannel",  CRCInput::RC_0 );

	strcpy(g_settings.repeat_blocker, configfile.getString("repeat_blocker", g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS ? "150" : "25").c_str());
#ifndef HAVE_DREAMBOX_HARDWARE
	strcpy(g_settings.repeat_genericblocker, configfile.getString("repeat_genericblocker", g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS ? "25" : "0").c_str());
#else
	// my dm500s needs this large setting - seife
	strcpy(g_settings.repeat_genericblocker, configfile.getString("repeat_genericblocker", "222").c_str());
#endif
	g_settings.audiochannel_up_down_enable = configfile.getBool("audiochannel_up_down_enable", false);
	g_settings.audio_left_right_selectable = configfile.getBool("audio_left_right_selectable", false);

	//screen configuration
	g_settings.screen_StartX = configfile.getInt32( "screen_StartX", 37 );
	g_settings.screen_StartY = configfile.getInt32( "screen_StartY", 23 );
	g_settings.screen_EndX = configfile.getInt32( "screen_EndX", 668 );
	g_settings.screen_EndY = configfile.getInt32( "screen_EndY", 555 );

	//Software-update
	g_settings.softupdate_mode = configfile.getInt32( "softupdate_mode", 1 );
	strcpy(g_settings.softupdate_url_file, configfile.getString("softupdate_url_file", "/etc/cramfs.urls").c_str());
	strcpy(g_settings.softupdate_proxyserver, configfile.getString("softupdate_proxyserver", "" ).c_str());
	strcpy(g_settings.softupdate_proxyusername, configfile.getString("softupdate_proxyusername", "" ).c_str());
	strcpy(g_settings.softupdate_proxypassword, configfile.getString("softupdate_proxypassword", "" ).c_str());

	//BouquetHandling
	g_settings.bouquetlist_mode = configfile.getInt32( "bouquetlist_mode", 0 );

	// parentallock
	if (!parentallocked)
	{
		g_settings.parentallock_prompt = configfile.getInt32( "parentallock_prompt", 0 );
		g_settings.parentallock_lockage = configfile.getInt32( "parentallock_lockage", 12 );
	}
	else
	{
	        g_settings.parentallock_prompt = 3;
	        g_settings.parentallock_lockage = 18;
	}
	strcpy( g_settings.parentallock_pincode, configfile.getString( "parentallock_pincode", "0000" ).c_str() );

	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		g_settings.timing[i] = configfile.getInt32(locale_real_names[timing_setting_name[i]], default_timing[i]);

	for (int i = 0; i < LCD_SETTING_COUNT; i++)
		g_settings.lcd_setting[i] = configfile.getInt32(lcd_setting[i].name, lcd_setting[i].default_value);
	strcpy(g_settings.lcd_setting_dim_time, configfile.getString("lcd_dim_time","0").c_str());
	strcpy(g_settings.lcd_setting_dim_brightness, configfile.getString("lcd_dim_brightness","0").c_str());

	//Picture-Viewer
	strcpy( g_settings.picviewer_slide_time, configfile.getString( "picviewer_slide_time", "10" ).c_str() );
	g_settings.picviewer_scaling = configfile.getInt32("picviewer_scaling", 1 /*(int)CPictureViewer::SIMPLE*/);
	g_settings.picviewer_decode_server_ip = configfile.getString("picviewer_decode_server_ip", "");
	strcpy(g_settings.picviewer_decode_server_port, configfile.getString("picviewer_decode_server_port", "").c_str());

	//Audio-Player
	g_settings.audioplayer_display = configfile.getInt32("audioplayer_display",(int)CAudioPlayerGui::ARTIST_TITLE);
	g_settings.audioplayer_follow  = configfile.getInt32("audioplayer_follow",0);
	strcpy( g_settings.audioplayer_screensaver, configfile.getString( "audioplayer_screensaver", "0" ).c_str() );
	g_settings.audioplayer_highprio  = configfile.getInt32("audioplayer_highprio",0);
	g_settings.audioplayer_select_title_by_name = configfile.getInt32("audioplayer_select_title_by_name",0);
	g_settings.audioplayer_repeat_on = configfile.getInt32("audioplayer_repeat_on",0);
	g_settings.audioplayer_show_playlist = configfile.getInt32("audioplayer_show_playlist",1);
	g_settings.audioplayer_enable_sc_metadata = configfile.getInt32("audioplayer_enable_sc_metadata",1);

	//Filebrowser
	g_settings.filebrowser_showrights =  configfile.getInt32("filebrowser_showrights", 1);
	g_settings.filebrowser_sortmethod = configfile.getInt32("filebrowser_sortmethod", 0);
	if ((g_settings.filebrowser_sortmethod < 0) || (g_settings.filebrowser_sortmethod >= FILEBROWSER_NUMBER_OF_SORT_VARIANTS))
		g_settings.filebrowser_sortmethod = 0;
	g_settings.filebrowser_denydirectoryleave = configfile.getBool("filebrowser_denydirectoryleave", false);

	// USERMENU
	//-------------------------------------------
	// this is as the current neutrino usermenu
	const char* usermenu_default[SNeutrinoSettings::BUTTON_MAX]={
		"2,3,4,16",			// RED
		"6",				// GREEN
		"7",				// YELLOW
		"8,9,1,15,1,10,11,13,1,14,5"	// BLUE
	};
	char txt1[81];
	std::string txt2;
	const char* txt2ptr;
	for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++)
	{
		snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
		txt1[80] = 0; // terminate for sure
		g_settings.usermenu_text[button] = configfile.getString(txt1, "" );

		snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);
		txt2 = configfile.getString(txt1,usermenu_default[button]);
		txt2ptr = txt2.c_str();
		for( int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++)
		{
			// find next comma or end of string - if it's not the first round
			if(pos != 0)
			{
				while(*txt2ptr != 0 && *txt2ptr != ',')
					txt2ptr++;
				if(*txt2ptr != 0)
					txt2ptr++;
			}
			if(*txt2ptr != 0)
			{
				g_settings.usermenu[button][pos] = atoi(txt2ptr);  // there is still a string
				if(g_settings.usermenu[button][pos] >= SNeutrinoSettings::ITEM_MAX)
					g_settings.usermenu[button][pos] = 0;
			}
			else
				g_settings.usermenu[button][pos] = 0;     // string empty, fill up with 0
		}
	}
	
	if(configfile.getUnknownKeyQueryedFlag() && (erg==0))
	{
		erg = 2;
	}

	if (!scanSettings.loadSettings(NEUTRINO_SCAN_SETTINGS_FILE, (g_info.delivery_system = g_Zapit->getDeliverySystem())))
	{
		dprintf(DEBUG_NORMAL, "Loading of scan settings failed. Using defaults.\n");
	}

	// uboot config file
	if(fromflash)
	{
		g_settings.uboot_console	= 0;
		g_settings.uboot_lcd_inverse	= -1;
		g_settings.uboot_lcd_contrast	= -1;

		FILE* fd = fopen("/var/tuxbox/boot/boot.conf", "r");
		if(fd)
		{
			char buffer[100];

			while(fgets(buffer, 99, fd) != NULL)
			{
				if(strncmp(buffer,"console=",8) == 0)
				{
					if(strncmp(&buffer[8], "null", 4)==0)
						g_settings.uboot_console = 0;
					else if(strncmp(&buffer[8], "ttyS0", 5)==0)
						g_settings.uboot_console = 1;
					else if(strncmp(&buffer[8], "tty", 3)==0)
						g_settings.uboot_console = 2;
				}
				else if(strncmp(buffer,"lcd_inverse=", 12) == 0)
				{
					g_settings.uboot_lcd_inverse = atoi(&buffer[12]);
				}
				else if(strncmp(buffer,"lcd_contrast=", 13) == 0)
				{
					g_settings.uboot_lcd_contrast = atoi(&buffer[13]);
				}
				else
					printf("unknown entry found in boot.conf\n");
			}

			fclose(fd);
		}
		g_settings.uboot_console_bak = g_settings.uboot_console;
	}

	return erg;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  saveSetup, save the application-settings                   *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::saveSetup()
{
	//uboot; write config only on changes
	if (fromflash &&
	    ((g_settings.uboot_console_bak != g_settings.uboot_console) ||
	     (g_settings.uboot_lcd_inverse  != g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE]) ||
	     (g_settings.uboot_lcd_contrast != g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST])))
	{
		FILE* fd = fopen("/var/tuxbox/boot/boot.conf", "w");

		if(fd != NULL)
		{
			const char * buffer;
			g_settings.uboot_console_bak    = g_settings.uboot_console;
			g_settings.uboot_lcd_inverse	= g_settings.lcd_setting[SNeutrinoSettings::LCD_INVERSE];
			g_settings.uboot_lcd_contrast	= g_settings.lcd_setting[SNeutrinoSettings::LCD_CONTRAST];

			switch(g_settings.uboot_console)
			{
			case 1:
				buffer = "ttyS0";
				break;
			case 2:
				buffer = "tty";
				break;
			default:
				buffer = "null";
				break;
			}
			fprintf(fd, "console=%s\n" "lcd_inverse=%d\n" "lcd_contrast=%d\n", buffer, g_settings.uboot_lcd_inverse, g_settings.uboot_lcd_contrast);
			fclose(fd);
		}
		else
		{
			dprintf(DEBUG_NORMAL, "unable to write file /var/tuxbox/boot/boot.conf\n");
		}
	}

	//scan settings
	if(!scanSettings.saveSettings(NEUTRINO_SCAN_SETTINGS_FILE))
	{
		dprintf(DEBUG_NORMAL, "error while saving scan-settings!\n");
	}

	//video
	configfile.setInt32( "video_Format", g_settings.video_Format );
	configfile.setInt32( "video_csync", g_settings.video_csync );

	//fb-alphavalues for gtx
	configfile.setInt32( "gtx_alpha1", g_settings.gtx_alpha1 );
	configfile.setInt32( "gtx_alpha2", g_settings.gtx_alpha2 );

	// EPG-Config
	configfile.setString("epg_cache_time"          ,g_settings.epg_cache );
	configfile.setString("epg_extendedcache_time"  ,g_settings.epg_extendedcache );
	configfile.setString("epg_old_events"          ,g_settings.epg_old_events );
	configfile.setString("epg_max_events"          ,g_settings.epg_max_events );
	configfile.setString("epg_dir"                 ,g_settings.epg_dir);

	//misc
	configfile.setBool("shutdown_real"             , g_settings.shutdown_real);
	configfile.setBool("shutdown_real_rcdelay"     , g_settings.shutdown_real_rcdelay);
	configfile.setString("shutdown_count"          , g_settings.shutdown_count);
	configfile.setBool("infobar_sat_display"       , g_settings.infobar_sat_display);
	configfile.setInt32("infobar_subchan_disp_pos" , g_settings.infobar_subchan_disp_pos);
	configfile.setBool("misc_spts"                 , g_settings.misc_spts);
#ifndef TUXTXT_CFG_STANDALONE
	configfile.setBool("tuxtxt_cache"              , g_settings.tuxtxt_cache);
#endif
	configfile.setBool("virtual_zap_mode"          , g_settings.virtual_zap_mode);
	configfile.setInt32("infobar_show"             , g_settings.infobar_show);

	//audio
	configfile.setInt32( "audio_AnalogMode" , g_settings.audio_AnalogMode);
	configfile.setBool("audio_DolbyDigital" , g_settings.audio_DolbyDigital);
	configfile.setInt32( "audio_avs_Control", g_settings.audio_avs_Control);
	configfile.setString( "audio_PCMOffset" , g_settings.audio_PCMOffset);

	//vcr
	configfile.setBool("vcr_AutoSwitch"     , g_settings.vcr_AutoSwitch);

	//language
	configfile.setString("language"         , g_settings.language);

	//widget settings
	configfile.setBool("widget_fade"        , g_settings.widget_fade);
	configfile.setInt32("widget_osd"        , g_settings.widget_osd);

	//colors
	configfile.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	configfile.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	configfile.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	configfile.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );

	configfile.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	configfile.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	configfile.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	configfile.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );

	configfile.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	configfile.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	configfile.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	configfile.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );

	configfile.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	configfile.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	configfile.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	configfile.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );

	configfile.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	configfile.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	configfile.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	configfile.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );

	configfile.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	configfile.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	configfile.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	configfile.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );

	configfile.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	configfile.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	configfile.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	configfile.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );

	configfile.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	configfile.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	configfile.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	configfile.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );

	configfile.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	configfile.setInt32( "infobar_red", g_settings.infobar_red );
	configfile.setInt32( "infobar_green", g_settings.infobar_green );
	configfile.setInt32( "infobar_blue", g_settings.infobar_blue );

	configfile.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	configfile.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	configfile.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	configfile.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );

	//network
	char cfg_key[81];
	for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
	{
		sprintf(cfg_key, "network_nfs_ip_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_ip[i] );
		sprintf(cfg_key, "network_nfs_dir_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_dir[i] );
		sprintf(cfg_key, "network_nfs_local_dir_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_local_dir[i] );
		sprintf(cfg_key, "network_nfs_automount_%d", i);
		configfile.setInt32( cfg_key, g_settings.network_nfs_automount[i]);
		sprintf(cfg_key, "network_nfs_type_%d", i);
		configfile.setInt32( cfg_key, g_settings.network_nfs_type[i]);
		sprintf(cfg_key,"network_nfs_username_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_username[i] );
		sprintf(cfg_key, "network_nfs_password_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_password[i] );
		sprintf(cfg_key, "network_nfs_mount_options1_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mount_options1[i]);
		sprintf(cfg_key, "network_nfs_mount_options2_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mount_options2[i]);
		sprintf(cfg_key, "network_nfs_mac_%d", i);
		configfile.setString( cfg_key, g_settings.network_nfs_mac[i]);
	}
	configfile.setString( "network_nfs_audioplayerdir", g_settings.network_nfs_audioplayerdir);
	configfile.setString( "network_nfs_picturedir", g_settings.network_nfs_picturedir);
	configfile.setString( "network_nfs_moviedir", g_settings.network_nfs_moviedir);
	configfile.setBool  ("filesystem_is_utf8"                 , g_settings.filesystem_is_utf8             );

	// Personalization
	configfile.setInt32 ( "personalize_pinstatus", g_settings.personalize_pinstatus );
	configfile.setInt32 ( "personalize_bluebutton", g_settings.personalize_bluebutton );
	configfile.setInt32 ( "personalize_redbutton", g_settings.personalize_redbutton );

	configfile.setString( "personalize_pincode", g_settings.personalize_pincode);
	configfile.setInt32 ( "personalize_tvmode", g_settings.personalize_tvmode );
	configfile.setInt32 ( "personalize_radiomode", g_settings.personalize_radiomode );
	configfile.setInt32 ( "personalize_scartmode", g_settings.personalize_scartmode );
	configfile.setInt32 ( "personalize_games", g_settings.personalize_games );
	configfile.setInt32 ( "personalize_audioplayer", g_settings.personalize_audioplayer );
	configfile.setInt32 ( "personalize_movieplayer", g_settings.personalize_movieplayer );
	configfile.setInt32 ( "personalize_pictureviewer", g_settings.personalize_pictureviewer );
#if ENABLE_UPNP
 	configfile.setInt32 ( "personalize_upnpbrowser", g_settings.personalize_upnpbrowser );
#endif
	configfile.setInt32 ( "personalize_settings", g_settings.personalize_settings );
	configfile.setInt32 ( "personalize_service", g_settings.personalize_service );
	configfile.setInt32 ( "personalize_sleeptimer", g_settings.personalize_sleeptimer );
	configfile.setInt32 ( "personalize_reboot", g_settings.personalize_reboot );
	configfile.setInt32 ( "personalize_shutdown", g_settings.personalize_shutdown );

	configfile.setInt32 ( "personalize_bouqueteditor", g_settings.personalize_bouqueteditor );
	configfile.setInt32 ( "personalize_scants", g_settings.personalize_scants );
	configfile.setInt32 ( "personalize_reload", g_settings.personalize_reload );
	configfile.setInt32 ( "personalize_getplugins", g_settings.personalize_getplugins );
	configfile.setInt32 ( "personalize_restart", g_settings.personalize_restart );
	configfile.setInt32 ( "personalize_ucodecheck", g_settings.personalize_ucodecheck );
	configfile.setInt32 ( "personalize_imageinfo", g_settings.personalize_imageinfo );
	configfile.setInt32 ( "personalize_update", g_settings.personalize_update );

	configfile.setInt32 ( "personalize_audio", g_settings.personalize_audio );
	configfile.setInt32 ( "personalize_video", g_settings.personalize_video );
	configfile.setInt32 ( "personalize_youth", g_settings.personalize_youth );
	configfile.setInt32 ( "personalize_network", g_settings.personalize_network );
	configfile.setInt32 ( "personalize_recording", g_settings.personalize_recording );
	configfile.setInt32 ( "personalize_streaming", g_settings.personalize_streaming );
	configfile.setInt32 ( "personalize_keybinding", g_settings.personalize_keybinding );
	configfile.setInt32 ( "personalize_language", g_settings.personalize_language );
	configfile.setInt32 ( "personalize_colors", g_settings.personalize_colors );
	configfile.setInt32 ( "personalize_lcd", g_settings.personalize_lcd );
	configfile.setInt32 ( "personalize_audpic", g_settings.personalize_audpic );
	configfile.setInt32 ( "personalize_driver", g_settings.personalize_driver );
	configfile.setInt32 ( "personalize_misc", g_settings.personalize_misc );

	// NTP-Server for sectionsd
	configfile.setString( "network_ntpserver", g_settings.network_ntpserver);
	configfile.setString( "network_ntprefresh", g_settings.network_ntprefresh);
	configfile.setBool( "network_ntpenable", g_settings.network_ntpenable);

	//recording (server + vcr)
	configfile.setInt32 ("recording_type",                      g_settings.recording_type);
	configfile.setBool  ("recording_stopplayback"             , g_settings.recording_stopplayback         );
	configfile.setBool  ("recording_stopsectionsd"            , g_settings.recording_stopsectionsd        );
	configfile.setString("recording_server_ip",                 g_settings.recording_server_ip);
	configfile.setString("recording_server_port",               g_settings.recording_server_port);
	configfile.setInt32 ("recording_server_wakeup",             g_settings.recording_server_wakeup);
	configfile.setString("recording_server_mac",                g_settings.recording_server_mac);
	configfile.setInt32 ("recording_vcr_no_scart",              g_settings.recording_vcr_no_scart);
	configfile.setString("recording_splitsize",                 g_settings.recording_splitsize);
	configfile.setBool  ("recordingmenu.use_o_sync"           , g_settings.recording_use_o_sync           );
	configfile.setBool  ("recordingmenu.use_fdatasync"        , g_settings.recording_use_fdatasync        );
	configfile.setInt32 ("recording_audio_pids_default"       , g_settings.recording_audio_pids_default);
	configfile.setBool  ("recordingmenu.stream_vtxt_pid"      , g_settings.recording_stream_vtxt_pid      );
	configfile.setBool  ("recordingmenu.stream_subtitle_pid"       , g_settings.recording_stream_subtitle_pid      );
	configfile.setString("recordingmenu.ringbuffers"          , g_settings.recording_ringbuffers);
	configfile.setInt32 ("recording_choose_direct_rec_dir"    , g_settings.recording_choose_direct_rec_dir);
	configfile.setBool  ("recording_epg_for_filename"         , g_settings.recording_epg_for_filename     );
	configfile.setBool  ("recording_in_spts_mode"             , g_settings.recording_in_spts_mode         );
	configfile.setBool  ("recording_zap_on_announce"          , g_settings.recording_zap_on_announce      );
	for(int i=0 ; i < REC_FILENAME_TEMPLATE_NR_OF_ENTRIES ; i++)
	{
		sprintf(cfg_key, "recording_filename_template_%d", i);
		configfile.setString( cfg_key, g_settings.recording_filename_template[i] );
		sprintf(cfg_key, "recording_dir_permissions_%d", i);
		configfile.setString( cfg_key, g_settings.recording_dir_permissions[i] );
	}

	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		sprintf(cfg_key, "recording_dir_%d", i);
		configfile.setString( cfg_key, g_settings.recording_dir[i] );
	}
	//streaming
	configfile.setInt32 ( "streaming_type", g_settings.streaming_type );
	configfile.setString( "streaming_server_ip", g_settings.streaming_server_ip );
	configfile.setString( "streaming_server_port", g_settings.streaming_server_port );
	configfile.setString( "streaming_server_cddrive", g_settings.streaming_server_cddrive );
	configfile.setString( "streaming_videorate", g_settings.streaming_videorate );
	configfile.setString( "streaming_audiorate", g_settings.streaming_audiorate );
	configfile.setString( "streaming_server_startdir", g_settings.streaming_server_startdir );
	configfile.setInt32 ( "streaming_transcode_audio", g_settings.streaming_transcode_audio );
	configfile.setInt32 ( "streaming_force_avi_rawaudio", g_settings.streaming_force_avi_rawaudio );
	configfile.setInt32 ( "streaming_force_transcode_video", g_settings.streaming_force_transcode_video );
	configfile.setInt32 ( "streaming_transcode_video_codec", g_settings.streaming_transcode_video_codec );
	configfile.setInt32 ( "streaming_resolution", g_settings.streaming_resolution );
	configfile.setInt32 ( "streaming_use_buffer", g_settings.streaming_use_buffer);
	configfile.setInt32 ( "streaming_buffer_segment_size", g_settings.streaming_buffer_segment_size);
	configfile.setInt32 ( "streaming_show_tv_in_browser", g_settings.streaming_show_tv_in_browser);
	configfile.setBool ("streaming_allow_multiselect", g_settings.streaming_allow_multiselect);

	// default plugin for movieplayer
	configfile.setString( "movieplayer_plugin", g_settings.movieplayer_plugin );

	//rc-key configuration
	configfile.setInt32 ( "key_tvradio_mode", g_settings.key_tvradio_mode );

	configfile.setInt32( "key_channelList_pageup", g_settings.key_channelList_pageup );
	configfile.setInt32( "key_channelList_pagedown", g_settings.key_channelList_pagedown );
	configfile.setInt32( "key_channelList_cancel", g_settings.key_channelList_cancel );
	configfile.setInt32( "key_channelList_sort", g_settings.key_channelList_sort );
	configfile.setInt32( "key_channelList_addrecord", g_settings.key_channelList_addrecord );
	configfile.setInt32( "key_channelList_addremind", g_settings.key_channelList_addremind );
	configfile.setInt32( "key_channelList_reload",  g_settings.key_channelList_reload );

	configfile.setInt32( "key_quickzap_up", g_settings.key_quickzap_up );
	configfile.setInt32( "key_quickzap_down", g_settings.key_quickzap_down );
	configfile.setInt32( "key_bouquet_up", g_settings.key_bouquet_up );
	configfile.setInt32( "key_bouquet_down", g_settings.key_bouquet_down );
	configfile.setInt32( "key_subchannel_up", g_settings.key_subchannel_up );
	configfile.setInt32( "key_subchannel_down", g_settings.key_subchannel_down );
	configfile.setInt32( "key_zaphistory", g_settings.key_zaphistory );
	configfile.setInt32( "key_lastchannel", g_settings.key_lastchannel );

	configfile.setString( "repeat_blocker", g_settings.repeat_blocker );
	configfile.setString( "repeat_genericblocker", g_settings.repeat_genericblocker );
	configfile.setBool  ( "audiochannel_up_down_enable", g_settings.audiochannel_up_down_enable );
	configfile.setBool  ( "audio_left_right_selectable", g_settings.audio_left_right_selectable );

	//screen configuration
	configfile.setInt32( "screen_StartX", g_settings.screen_StartX );
	configfile.setInt32( "screen_StartY", g_settings.screen_StartY );
	configfile.setInt32( "screen_EndX", g_settings.screen_EndX );
	configfile.setInt32( "screen_EndY", g_settings.screen_EndY );

	//Software-update
	configfile.setInt32 ("softupdate_mode"          , g_settings.softupdate_mode          );
	configfile.setString("softupdate_url_file"      , g_settings.softupdate_url_file      );
	configfile.setString("softupdate_proxyserver"   , g_settings.softupdate_proxyserver   );
	configfile.setString("softupdate_proxyusername" , g_settings.softupdate_proxyusername );
	configfile.setString("softupdate_proxypassword" , g_settings.softupdate_proxypassword );

	//BouquetHandling
	configfile.setInt32( "bouquetlist_mode", g_settings.bouquetlist_mode );

	//parentallock
	configfile.setInt32( "parentallock_prompt", g_settings.parentallock_prompt );
	configfile.setInt32( "parentallock_lockage", g_settings.parentallock_lockage );
	configfile.setString( "parentallock_pincode", g_settings.parentallock_pincode );

	//timing
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		configfile.setInt32(locale_real_names[timing_setting_name[i]], g_settings.timing[i]);

	for (int i = 0; i < LCD_SETTING_COUNT; i++)
		configfile.setInt32(lcd_setting[i].name, g_settings.lcd_setting[i]);
	configfile.setString("lcd_dim_time", g_settings.lcd_setting_dim_time);
	configfile.setString("lcd_dim_brightness", g_settings.lcd_setting_dim_brightness);

	//Picture-Viewer
	configfile.setString( "picviewer_slide_time", g_settings.picviewer_slide_time );
	configfile.setInt32( "picviewer_scaling", g_settings.picviewer_scaling );
	configfile.setString( "picviewer_decode_server_ip", g_settings.picviewer_decode_server_ip );
	configfile.setString( "picviewer_decode_server_port", g_settings.picviewer_decode_server_port);

	//Audio-Player
	configfile.setInt32( "audioplayer_display", g_settings.audioplayer_display );
	configfile.setInt32( "audioplayer_follow", g_settings.audioplayer_follow );
	configfile.setString( "audioplayer_screensaver", g_settings.audioplayer_screensaver );
	configfile.setInt32( "audioplayer_highprio", g_settings.audioplayer_highprio );
	configfile.setInt32( "audioplayer_select_title_by_name", g_settings.audioplayer_select_title_by_name );
	configfile.setInt32( "audioplayer_repeat_on", g_settings.audioplayer_repeat_on );
	configfile.setInt32( "audioplayer_show_playlist", g_settings.audioplayer_show_playlist );
	configfile.setInt32( "audioplayer_enable_sc_metadata", g_settings.audioplayer_enable_sc_metadata );

	//Filebrowser
	configfile.setInt32("filebrowser_showrights", g_settings.filebrowser_showrights);
	configfile.setInt32("filebrowser_sortmethod", g_settings.filebrowser_sortmethod);
	configfile.setBool("filebrowser_denydirectoryleave", g_settings.filebrowser_denydirectoryleave);

	// USERMENU
	//---------------------------------------
	char txt1[81];
	char txt2[81];
	for(int button = 0; button < SNeutrinoSettings::BUTTON_MAX; button++)
	{
		snprintf(txt1,80,"usermenu_tv_%s_text",usermenu_button_def[button]);
		txt1[80] = 0; // terminate for sure
		configfile.setString(txt1,g_settings.usermenu_text[button]);

		char* txt2ptr = txt2;
		snprintf(txt1,80,"usermenu_tv_%s",usermenu_button_def[button]);
		for(int pos = 0; pos < SNeutrinoSettings::ITEM_MAX; pos++)
		{
			if( g_settings.usermenu[button][pos] != 0)
			{
				if(pos != 0)
					*txt2ptr++ = ',';
				txt2ptr += snprintf(txt2ptr,80,"%d",g_settings.usermenu[button][pos]);
			}
		}
		configfile.setString(txt1,txt2);
	}

	if (configfile.getModifiedFlag())
	{
		dprintf(DEBUG_INFO, "saving neutrino txt-config\n");
		configfile.saveConfig(NEUTRINO_SETTINGS_FILE);
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  firstChannel, get the initial channel                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::firstChannel()
{
	g_Zapit->getLastChannel(firstchannel.channelNumber, firstchannel.mode);
}

#ifndef HAVE_DREAMBOX_HARDWARE
/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  ucodes_available, check if ucodes are available            *
*                                                                                     *
**************************************************************************************/

bool CNeutrinoApp::ucodes_available(void)
{
	FILE * fd;

	fd = fopen(UCODEDIR "/avia500.ux", "r");
	if (fd)
		fclose(fd);

	bool ucodes_ok = (fd);

	fd = fopen(UCODEDIR "/avia600.ux", "r");
	if (fd)
		fclose(fd);
	ucodes_ok = ucodes_ok || (fd);

	fd = fopen(UCODEDIR "/cam-alpha.bin", "r");
	if (fd)
		fclose(fd);
	ucodes_ok = ucodes_ok && (fd);

	return ucodes_ok;
}
#endif


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit, get the Channellist from daemon              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit(int init_mode, int mode)
{
	dprintf(DEBUG_DEBUG, "doing channelsInit\n");

	if (init_mode == init_mode_switch) {
		if (mode_radio == mode) {
			if (channelListRADIO && bouquetListRADIO) {
				channelList = channelListRADIO;
				bouquetList = bouquetListRADIO;
				// otherwise zapit will not tune to the new TP
				channelList->clearTuned();
				return;
			} // else load
		} 
		else if (mode_tv == mode) {
			if (channelListTV && bouquetListTV) {
				channelList = channelListTV;
				bouquetList = bouquetListTV;
				// otherwise zapit will not tune to the new TP
				channelList->clearTuned();
				return;
			} // else load
		}
	}

	if (mode_unknown == mode) { // stay in current TV/Radio mode
		if (channelList == channelListTV) {
			mode = mode_tv;
		} else {
			mode = mode_radio;
		}
	}

	CZapitClient::BouquetChannelList zapitChannels;
	CZapitClient::BouquetList zapitBouquets;

	//deleting old channelList for mode-switching.
	if (channelListTV)
		delete channelListTV;
	channelListTV = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));

	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_TV, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channelListTV->addChannel(zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}

	if (bouquetListTV)
		delete bouquetListTV;
	bouquetListTV = new CBouquetList();
	bouquetListTV->orgChannelList = channelListTV;

	/* load non-empty bouquets only */
	g_Zapit->getBouquets(zapitBouquets, false, true, CZapitClient::MODE_TV); // UTF-8
	for(uint i = 0; i < zapitBouquets.size(); i++)
	{
		CZapitClient::BouquetChannelList zapitChannels;

		bouquetListTV->addBouquet(zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
		g_Zapit->getBouquetChannels(zapitBouquets[i].bouquet_nr, zapitChannels, CZapitClient::MODE_TV, true); // UTF-8

		for (uint j = 0; j < zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = new CChannelList::CChannel(zapitChannels[j].nr, zapitChannels[j].nr, zapitChannels[j].name, zapitChannels[j].satellitePosition, zapitChannels[j].channel_id); // UTF-8
	
			/* observe that "bouquetList->Bouquets[i]" refers to the bouquet we just created using bouquetList->addBouquet */
			bouquetListTV->Bouquets[i]->channelList->addChannel(channel);
			if (zapitBouquets[i].locked)
			{
				channel->bAlwaysLocked = true;

				for (int k=0; k<channelListTV->getSize(); k++)
				{
					if ((*channelListTV)[k]->channel_id == zapitChannels[j].channel_id) {
						(*channelListTV)[k]->bAlwaysLocked = true;
					}
				}
			}
		}
	}

	zapitBouquets.clear();
	zapitChannels.clear();

	// same for the RADIO channels
	if (channelListRADIO)
		delete channelListRADIO;

	channelListRADIO = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_RADIO, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channelListRADIO->addChannel(zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}

	if (bouquetListRADIO)
		delete bouquetListRADIO;
	bouquetListRADIO = new CBouquetList();
	bouquetListRADIO->orgChannelList = channelListRADIO;

	/* load non-empty bouquets only */
	g_Zapit->getBouquets(zapitBouquets, false, true, CZapitClient::MODE_RADIO); // UTF-8
	for(uint i = 0; i < zapitBouquets.size(); i++)
	{
		CZapitClient::BouquetChannelList zapitChannels;

		bouquetListRADIO->addBouquet(zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
		g_Zapit->getBouquetChannels(zapitBouquets[i].bouquet_nr, zapitChannels, CZapitClient::MODE_RADIO, true); // UTF-8

		for (uint j = 0; j < zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = new CChannelList::CChannel(zapitChannels[j].nr, zapitChannels[j].nr, zapitChannels[j].name, zapitChannels[j].satellitePosition, zapitChannels[j].channel_id); // UTF-8

			/* observe that "bouquetList->Bouquets[i]" refers to the bouquet we just created using bouquetList->addBouquet */
			bouquetListRADIO->Bouquets[i]->channelList->addChannel(channel);

			if (zapitBouquets[i].locked)
			{
				channel->bAlwaysLocked = true;
			}
		}
	}

	dprintf(DEBUG_DEBUG, "\nAll bouquets-channels received\n");
	if (mode == mode_radio) {
		if (channelListRADIO && bouquetListRADIO) {
			channelList = channelListRADIO;
			bouquetList = bouquetListRADIO;
			// otherwise zapit will not tune to the new TP
			channelList->clearTuned();
		}
	} 
	else if (mode == mode_tv) {
		if (channelListTV && bouquetListTV) {
			channelList = channelListTV;
			bouquetList = bouquetListTV;
			// otherwise zapit will not tune to the new TP
			channelList->clearTuned();
		}
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit4Record, get the Channellist from daemon       *
*                          during RecordMode                                          *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit4Record(void)
{
	dprintf(DEBUG_DEBUG, "doing channelsInit4Record\n");

	CZapitClient::BouquetChannelList zapitChannels;
	CZapitClient::BouquetList zapitBouquets;

	//deleting old channelList for mode-switching.
	if (channelListRecord)
		delete channelListRecord;

	channelListRecord = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_CURRENT, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channelListRecord->addChannel(zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}

	if (bouquetListRecord)
		delete bouquetListRecord;
	bouquetListRecord = new CBouquetList();
	bouquetListRecord ->orgChannelList = channelListRecord;

	/* load non-empty bouquets only */
	g_Zapit->getBouquets(zapitBouquets, false, true); // UTF-8
	for(uint i = 0; i < zapitBouquets.size(); i++)
	{
		CZapitClient::BouquetChannelList zapitChannels;

		bouquetListRecord->addBouquet(zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
		g_Zapit->getBouquetChannels(zapitBouquets[i].bouquet_nr, zapitChannels, CZapitClient::MODE_CURRENT, true); // UTF-8

		for (uint j = 0; j < zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = channelListRecord->getChannel(zapitChannels[j].nr);

			/* observe that "bouquetList->Bouquets[i]" refers to the bouquet we just created using bouquetList->addBouquet */
			bouquetListRecord->Bouquets[i]->channelList->addChannel(channel);

			if (zapitBouquets[i].locked)
			{
				channel->bAlwaysLocked = true;
			}
		}
	}

	channelList = channelListRecord;
	bouquetList = bouquetListRecord;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  run, the main runloop                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::CmdParser(int argc, char **argv)
{
	global_argv = new char *[argc+1];
	for (int i = 0; i < argc; i++)
		global_argv[i] = argv[i];
	global_argv[argc] = NULL;

	softupdate = false;
	fromflash = false;

	font.name = NULL;

	for(int x=1; x<argc; x++)
	{
		if ((!strcmp(argv[x], "-u")) || (!strcmp(argv[x], "--enable-update")))
		{
			dprintf(DEBUG_NORMAL, "Software update enabled\n");
			softupdate = true;
		}
		else if ((!strcmp(argv[x], "-f")) || (!strcmp(argv[x], "--enable-flash")))
		{
			dprintf(DEBUG_NORMAL, "enable flash\n");
			fromflash = true;
		}
		else if (!strcmp(argv[x], "--font"))
		{
			if ((x + 3) < argc)
			{
				font.name = argv[x + 1];
				font.size_offset = atoi(argv[x + 2]);
				font.filename[0] = argv[x + 3];
				if ((x + 4) < argc)
				{
					font.filename[1] = argv[x + 4];
					x++;
				}
				else
					font.filename[1] = NULL;

				if ((x + 4) < argc)
				{
					font.filename[2] = argv[x + 4];
					x++;
				}
				else
					font.filename[2] = NULL;
			}
			x += 3;
		}
		else if (((!strcmp(argv[x], "-v")) || (!strcmp(argv[x], "--verbose"))) && (x+1 < argc))
		{
			int dl = atoi(argv[x+ 1]);
			dprintf(DEBUG_NORMAL, "set debuglevel: %d\n", dl);
			setDebugLevel(dl);
			x++;
		}
		else
		{
			dprintf(DEBUG_NORMAL, "Usage: neutrino [-u | --enable-update] [-f | --enable-flash] [-v | --verbose 0..3] [--font name sizeoffset /dir/file.ttf [/dir/bold.ttf [/dir/italic.ttf]]]\n");
			exit(0);
		}
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup the framebuffer                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::SetupFrameBuffer()
{
	frameBuffer->init();
	if(frameBuffer->setMode(720, 576, 8 * sizeof(fb_pixel_t)))
	{
		dprintf(DEBUG_NORMAL, "Error while setting framebuffer mode\n");
		exit(-1);
	}

	//make 1..8 transparent for dummy painting
	for(int count =0;count<8;count++)
		frameBuffer->paletteSetColor(count, 0x000000, 0xffff);
	frameBuffer->paletteSet();
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup fonts                                                *
*                                                                                     *
**************************************************************************************/

const neutrino_font_descr_struct predefined_font[2] =
{
	{"Micron"            , {FONTDIR "/micron.ttf"        , FONTDIR "/micron_bold.ttf", FONTDIR "/micron_italic.ttf"}, 0},
	{"MD King KhammuRabi", {FONTDIR "/md_khmurabi_10.ttf", NULL                      , NULL                        }, 0}
};

const char* predefined_lcd_font[2][6] =
{
	{FONTDIR "/12.pcf.gz", "Fix12", FONTDIR "/14B.pcf.gz", "Fix14", FONTDIR "/15B.pcf.gz", "Fix15"},
	{FONTDIR "/md_khmurabi_10.ttf", "MD King KhammuRabi", NULL, NULL,  NULL, NULL}
};

void CNeutrinoApp::SetupFonts()
{
	const char * style[3];

	if (g_fontRenderer != NULL)
		delete g_fontRenderer;

	g_fontRenderer = new FBFontRenderClass();

	style[0] = g_fontRenderer->AddFont(font.filename[0]);

	style[1] = (font.filename[1] == NULL) ? "Bold Regular" : g_fontRenderer->AddFont(font.filename[1]);

	if (font.filename[2] == NULL)
	{
		g_fontRenderer->AddFont(font.filename[0], true);  // make italics
		style[2] = "Italic";
	}
	else
		style[2] = g_fontRenderer->AddFont(font.filename[2]);

	for (int i = 0; i < FONT_TYPE_COUNT; i++)
	{
		g_Font[i] = g_fontRenderer->getFont(font.name, style[neutrino_font[i].style], configfile.getInt32(locale_real_names[neutrino_font[i].name], neutrino_font[i].defaultsize) + neutrino_font[i].size_offset * font.size_offset);
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup the menu timouts                                     *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::SetupTiming()
{
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
		sprintf(g_settings.timing_string[i], "%d", g_settings.timing[i]);
}


/* cut here */
#define VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT 5
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOSIGNAL_OPTIONS[VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT] =
{
	{ 1, LOCALE_VIDEOMENU_VIDEOSIGNAL_RGB       },
	{ 2, LOCALE_VIDEOMENU_VIDEOSIGNAL_SVIDEO    },
	{ 3, LOCALE_VIDEOMENU_VIDEOSIGNAL_YUV_V     },
	{ 4, LOCALE_VIDEOMENU_VIDEOSIGNAL_YUV_C     },
	{ 0, LOCALE_VIDEOMENU_VIDEOSIGNAL_COMPOSITE }
};

#define VIDEOMENU_VCRSIGNAL_OPTION_COUNT 2
const CMenuOptionChooser::keyval VIDEOMENU_VCRSIGNAL_OPTIONS[VIDEOMENU_VCRSIGNAL_OPTION_COUNT] =
{
	{ 2, LOCALE_VIDEOMENU_VCRSIGNAL_SVIDEO    },
	{ 0, LOCALE_VIDEOMENU_VCRSIGNAL_COMPOSITE }
};

#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] =
{
	{ 2, LOCALE_VIDEOMENU_VIDEOFORMAT_43         },
	{ 3, LOCALE_VIDEOMENU_VIDEOFORMAT_431        },
	{ 1, LOCALE_VIDEOMENU_VIDEOFORMAT_169        },
	{ 0, LOCALE_VIDEOMENU_VIDEOFORMAT_AUTODETECT }
};

class CVideoSettings : public CMenuWidget, CChangeObserver
{
	CMenuForwarder *   SyncControlerForwarder;
	CMenuOptionChooser * VcrVideoOutSignalOptionChooser;
	CRGBCSyncControler RGBCSyncControler;
	CScreenSetup       ScreenSetup;
	int                video_out_signal;
	int                vcr_video_out_signal;

public:
	CVideoSettings() : CMenuWidget(LOCALE_VIDEOMENU_HEAD, "video.raw"), RGBCSyncControler(LOCALE_VIDEOMENU_RGB_CENTERING, &g_settings.video_csync)
		{
			addItem(GenericMenuSeparator);
			addItem(GenericMenuBack);
			addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_VIDEOMENU_TV_SCART));

			addItem(new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEOSIGNAL, &video_out_signal, VIDEOMENU_VIDEOSIGNAL_OPTIONS, VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT, true, this));

			CMenuOptionChooser * oj = new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEOFORMAT, &g_settings.video_Format, VIDEOMENU_VIDEOFORMAT_OPTIONS, VIDEOMENU_VIDEOFORMAT_OPTION_COUNT, true, this);

			if (g_settings.video_Format == CControldClient::VIDEOFORMAT_AUTO)
			{
				changeNotify(LOCALE_VIDEOMENU_VIDEOFORMAT, NULL);
			}

			addItem(oj);

			SyncControlerForwarder = new CMenuForwarder(LOCALE_VIDEOMENU_RGB_CENTERING, false, NULL, &RGBCSyncControler);
			addItem(SyncControlerForwarder);

			addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_VIDEOMENU_VCR_SCART));
			// Switching VCR Output presently does not work on the Philips.
			if (g_info.box_Type != CControld::TUXBOX_MAKER_PHILIPS)
			{
				VcrVideoOutSignalOptionChooser = new CMenuOptionChooser(LOCALE_VIDEOMENU_VCRSIGNAL, &vcr_video_out_signal, VIDEOMENU_VCRSIGNAL_OPTIONS, VIDEOMENU_VCRSIGNAL_OPTION_COUNT, false, this);
				addItem(VcrVideoOutSignalOptionChooser);
			}
			else
				VcrVideoOutSignalOptionChooser = 0;
			addItem(new CMenuOptionChooser(LOCALE_VIDEOMENU_VCRSWITCH, &g_settings.vcr_AutoSwitch, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true));

			addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_VIDEOMENU_OSD));
			addItem(new CMenuForwarder(LOCALE_VIDEOMENU_SCREENSETUP, true, NULL, &ScreenSetup));
		};

	virtual bool changeNotify(const neutrino_locale_t OptionName, void *)
		{
			if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEOSIGNAL))
			{
				while ((vcr_video_out_signal) == CControldClient::VIDEOOUTPUT_SVIDEO && (video_out_signal != CControldClient::VIDEOOUTPUT_SVIDEO) && (video_out_signal != CControldClient::VIDEOOUTPUT_COMPOSITE) )
					video_out_signal = (video_out_signal + 1) % 5;
				g_Controld->setVideoOutput(video_out_signal);
				if (VcrVideoOutSignalOptionChooser)
					VcrVideoOutSignalOptionChooser->setActive((video_out_signal == CControldClient::VIDEOOUTPUT_COMPOSITE) || (video_out_signal == CControldClient::VIDEOOUTPUT_SVIDEO));
				SyncControlerForwarder->setActive((video_out_signal == CControldClient::VIDEOOUTPUT_RGB) || (video_out_signal == CControldClient::VIDEOOUTPUT_YUV_VBS) || (video_out_signal == CControldClient::VIDEOOUTPUT_YUV_CVBS));
			}
			else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VCRSIGNAL))
			{
				g_Controld->setVCROutput(vcr_video_out_signal);
			}
			else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEOFORMAT))
			{
				g_Controld->setVideoFormat(g_settings.video_Format);
			}

			return true;
		};

	virtual void paint()
		{
			video_out_signal = g_Controld->getVideoOutput();
			vcr_video_out_signal = g_Controld->getVCROutput();

			if (VcrVideoOutSignalOptionChooser)
				VcrVideoOutSignalOptionChooser->active = ((video_out_signal == CControldClient::VIDEOOUTPUT_COMPOSITE) || (video_out_signal == CControldClient::VIDEOOUTPUT_SVIDEO));
			SyncControlerForwarder->active = ((video_out_signal == CControldClient::VIDEOOUTPUT_RGB) || (video_out_signal == CControldClient::VIDEOOUTPUT_YUV_VBS) || (video_out_signal ==  CControldClient::VIDEOOUTPUT_YUV_CVBS));

			g_settings.video_Format = g_Controld->getVideoFormat();

			CMenuWidget::paint();
		};
};






bool CNeutrinoApp::doGuiRecord(char * preselectedDir, bool addTimer)
{
	CTimerd::RecordingInfo eventinfo;
	bool refreshGui = false;
	if(CVCRControl::getInstance()->isDeviceRegistered())
	{
		if(recordingstatus == 1)
		{
			execute_start_file(NEUTRINO_RECORDING_START_SCRIPT);

			eventinfo.channel_id = g_Zapit->getCurrentServiceID();
			CEPGData epgData;
			if (g_Sectionsd->getActualEPGServiceKey(g_RemoteControl->current_channel_id, &epgData ))
			{
				eventinfo.epgID = epgData.eventID;
				eventinfo.epg_starttime = epgData.epg_times.startzeit;
				strncpy(eventinfo.epgTitle, epgData.title.c_str(), EPG_TITLE_MAXLEN-1);
				eventinfo.epgTitle[EPG_TITLE_MAXLEN-1]=0;
			}
			else
			{
				eventinfo.epgID = 0;
				eventinfo.epg_starttime = 0;
				strcpy(eventinfo.epgTitle, "");
			}
			eventinfo.apids = TIMERD_APIDS_CONF;
			bool doRecord = true;
			if (g_settings.recording_type == RECORDING_FILE)
			{
				std::string recDir = (preselectedDir != NULL) ? preselectedDir : g_settings.recording_dir[0];
				if( preselectedDir == NULL && g_settings.recording_choose_direct_rec_dir)
				{
					CRecDirChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR,NEUTRINO_ICON_SETTINGS,NULL,&recDir);
					recDirs.exec(NULL,"");
					refreshGui = true;
					recDir = recDirs.get_selected_dir();
                    //printf("dir : %s\n",recDir.c_str());
					if( recDir != "")
					{
						int nfs_nr = getNFSIDOfDir(recDir.c_str());
				        if(nfs_nr != -1)
						{
							recDir = g_settings.network_nfs_local_dir[nfs_nr];
							if (!CFSMounter::isMounted(g_settings.network_nfs_local_dir[nfs_nr]))
							{
				         		printf("not mounted, try to mount: %d\n",nfs_nr);
								CFSMounter::MountRes mres =
									CFSMounter::mount(g_settings.network_nfs_ip[nfs_nr].c_str(),
											  g_settings.network_nfs_dir[nfs_nr],
											  g_settings.network_nfs_local_dir[nfs_nr],
											  (CFSMounter::FSType) g_settings.network_nfs_type[nfs_nr],
											  g_settings.network_nfs_username[nfs_nr],
											  g_settings.network_nfs_password[nfs_nr],
											  g_settings.network_nfs_mount_options1[nfs_nr],
											  g_settings.network_nfs_mount_options2[nfs_nr]);
								if (mres != CFSMounter::MRES_OK)
								{
									doRecord = false;
									std::string msg = mntRes2Str(mres) + "\nDir: " + g_settings.network_nfs_local_dir[nfs_nr];
									ShowMsgUTF(LOCALE_MESSAGEBOX_ERROR, msg.c_str(),
											CMessageBox::mbrBack, CMessageBox::mbBack,NEUTRINO_ICON_ERROR, 450, 10); // UTF-8
								}
							}
						}
				}
				else
					{
						doRecord = false;
					}
				}
				CVCRControl::CFileDevice *fileDevice;
				if ((fileDevice = dynamic_cast<CVCRControl::CFileDevice*>(recordingdevice)) != NULL)
				{
					if (preselectedDir != NULL) {
						// recording has been interrupted, we are starting again
						// all directories have already been created so we should not create them again
						fileDevice->CreateTemplateDirectories = false;
					}
					fileDevice->Directory = recDir;
					fileDevice->FilenameTemplate = g_settings.recording_filename_template[0];
				} else
				{
					puts("[neutrino] could not set directory and filename template");
				}

			}
			if(!doRecord || (CVCRControl::getInstance()->Record(&eventinfo)==false))
			{
				recordingstatus=0;
				return refreshGui;
			}
			else if (addTimer)
			{
				time_t now = time(NULL);
				recording_id = g_Timerd->addImmediateRecordTimerEvent(eventinfo.channel_id, now, now+4*60*60,
											eventinfo.epgID, eventinfo.epg_starttime,
											eventinfo.apids);
			}
		}
		else
		{
			g_Timerd->stopTimerEvent(recording_id);
			startNextRecording();
		}
		return refreshGui;
	}
	else
		puts("[neutrino.cpp] no recording devices");
	return false;
}

void CNeutrinoApp::SendSectionsdConfig(void)
{
	CSectionsdClient::epg_config config;
	config.scanMode 		= scanSettings.scanSectionsd;
	config.epg_cache 		= atoi(g_settings.epg_cache.c_str());
	config.epg_extendedcache        = atoi(g_settings.epg_extendedcache.c_str());
	config.epg_old_events 		= atoi(g_settings.epg_old_events.c_str());
	config.epg_max_events		= atoi(g_settings.epg_max_events.c_str());
	config.epg_dir			= g_settings.epg_dir;
	config.network_ntpserver	= g_settings.network_ntpserver;
	config.network_ntprefresh	= atoi(g_settings.network_ntprefresh.c_str());
	config.network_ntpenable	= g_settings.network_ntpenable;
	g_Sectionsd->setConfig(config);
}

#define LCD_UPDATE_TIME_RADIO_MODE (6 * 1000 * 1000)
#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

void CNeutrinoApp::InitZapper()
{

	g_InfoViewer->start();
	g_EpgData->start();

	firstChannel();

	// EPG-Config
	SendSectionsdConfig();

	if (g_settings.epg_dir.length() != 0)
		g_Sectionsd->readSIfromXML(g_settings.epg_dir.c_str());

#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache)
	{
		tuxtxt_init();
	}
#endif

#ifndef HAVE_DREAMBOX_HARDWARE
	// set initial PES/SPTS mode
	if (g_settings.misc_spts != g_Zapit->PlaybackState())
	{
		if (g_settings.misc_spts && firstchannel.mode == 't' )
			g_Zapit->PlaybackSPTS();
		else
			g_Zapit->PlaybackPES();
	}
#endif

	if(firstchannel.mode == 't')
	{
		channelsInit(init_mode_init, mode_tv);
		tvMode();
	}
	else
	{
		channelsInit(init_mode_init, mode_radio);
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );
		radioMode();
	}
}

void CNeutrinoApp::setupRecordingDevice(void)
{
	if (g_settings.recording_type == RECORDING_SERVER)
	{
		unsigned int port;
		sscanf(g_settings.recording_server_port, "%u", &port);

		recordingdevice = new CVCRControl::CServerDevice(g_settings.recording_stopplayback, g_settings.recording_stopsectionsd, g_settings.recording_server_ip.c_str(), port);

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else if (g_settings.recording_type == RECORDING_FILE)
	{
		unsigned int splitsize, ringbuffers;
		sscanf(g_settings.recording_splitsize, "%u", &splitsize);
		sscanf(g_settings.recording_ringbuffers, "%u", &ringbuffers);

		recordingdevice = new CVCRControl::CFileDevice(g_settings.recording_stopplayback, g_settings.recording_stopsectionsd, g_settings.recording_dir[0].c_str(), splitsize, g_settings.recording_use_o_sync, g_settings.recording_use_fdatasync, g_settings.recording_stream_vtxt_pid, g_settings.recording_stream_subtitle_pid, ringbuffers,true);

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else if(g_settings.recording_type == RECORDING_VCR)
	{
		recordingdevice = new CVCRControl::CVCRDevice((g_settings.recording_vcr_no_scart == 0));

		CVCRControl::getInstance()->registerDevice(recordingdevice);
	}
	else
	{
		if (CVCRControl::getInstance()->isDeviceRegistered())
			CVCRControl::getInstance()->unregisterDevice();
	}
}

int CNeutrinoApp::run(int argc, char **argv)
{
	CmdParser(argc, argv);

	int loadSettingsErg = loadSetup();

	/* load locales before setting up any fonts to determine whether we need a true unicode font */
	bool display_language_selection;
	CLocaleManager::loadLocale_ret_t loadLocale_ret = g_Locale->loadLocale(g_settings.language);
	if (loadLocale_ret == CLocaleManager::NO_SUCH_LOCALE)
	{
		strcpy(g_settings.language, "deutsch");	// Fallback if rest fails
		FILE *f = fopen(NEUTRINO_DEFAULTLOCALE_FILE, "r");
		if (f)
		{
			char loc[100];
			int res = fscanf(f, "%s", loc);
			if (res > 0)
				strcpy(g_settings.language, loc);
			fclose(f);
		}
		loadLocale_ret = g_Locale->loadLocale(g_settings.language);
		display_language_selection = true;
	}
	else
		display_language_selection = false;

	if (font.name == NULL) /* no font specified in command line */
	{
		unsigned int use_true_unicode_font = (loadLocale_ret == CLocaleManager::ISO_8859_1_FONT) ? 0 : 1;

		font = predefined_font[use_true_unicode_font];
		CLCD::getInstance()->init(predefined_lcd_font[use_true_unicode_font][0],
		                          predefined_lcd_font[use_true_unicode_font][1],
		                          predefined_lcd_font[use_true_unicode_font][2],
		                          predefined_lcd_font[use_true_unicode_font][3],
		                          predefined_lcd_font[use_true_unicode_font][4],
		                          predefined_lcd_font[use_true_unicode_font][5]);
	}
	else
		CLCD::getInstance()->init(font.filename[0], font.name);

	CLCD::getInstance()->showVolume(g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control));
	CLCD::getInstance()->setMuted(g_Controld->getMute((CControld::volume_type)g_settings.audio_avs_Control));

	g_info.box_Type = g_Controld->getBoxType();

	dprintf( DEBUG_DEBUG, "[neutrino] box_Type: %d\n", g_info.box_Type);

	SetupTiming();

	SetupFonts();

	frameBuffer->ClearFrameBuffer();

	g_RCInput = new CRCInput;

	g_Sectionsd = new CSectionsdClient;
	g_Timerd = new CTimerdClient;

	g_RemoteControl = new CRemoteControl;
	g_EpgData = new CEpgData;
	g_InfoViewer = new CInfoViewer;
	g_EventList = new EventList;

	g_PluginList = new CPlugins;
	g_PluginList->setPluginDir(PLUGINDIR);

	// mount shares before scanning for plugins
	CFSMounter::automount();

	//load Pluginlist before main menu (only show script menu if at least one
	// script is available
	g_PluginList->loadPlugins();


	colorSetupNotifier        = new CColorSetupNotifier;
	audioSetupNotifier        = new CAudioSetupNotifier;
	APIDChanger               = new CAPIDChangeExec;
#ifndef HAVE_DREAMBOX_HARDWARE
	UCodeChecker              = new CUCodeCheckExec;
#endif
	NVODChanger               = new CNVODChangeExec;
	StreamFeaturesChanger     = new CStreamFeaturesChangeExec;
	MoviePluginChanger        = new CMoviePluginChangeExec;
	MyIPChanger               = new CIPChangeNotifier;
	ConsoleDestinationChanger = new CConsoleDestChangeNotifier;
	fontsizenotifier          = new CFontSizeNotifier;

	rcLock                    = new CRCLock();
	moviePlayerGui            = new CMoviePlayerGui();
	//USERMENU
	Timerlist                 = new CTimerList;

	colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);

	// setup recording device
	if (g_settings.recording_type != RECORDING_OFF)
		setupRecordingDevice();

	dprintf( DEBUG_NORMAL, "menue setup\n");
	//Main settings
	CMenuWidget    mainMenu            (LOCALE_MAINMENU_HEAD                 , "mainmenue.raw"       );
	CMenuWidget    mainSettings        (LOCALE_MAINSETTINGS_HEAD             , NEUTRINO_ICON_SETTINGS);
	CMenuWidget    languageSettings    (LOCALE_LANGUAGESETUP_HEAD            , "language.raw"        );
	CVideoSettings videoSettings                                                                      ;
	CMenuWidget    audioSettings       (LOCALE_AUDIOMENU_HEAD                , "audio.raw"           );
	CMenuWidget    parentallockSettings(LOCALE_PARENTALLOCK_PARENTALLOCK     , "lock.raw"            , 500);
	CMenuWidget    networkSettings     (LOCALE_NETWORKMENU_HEAD              , "network.raw"         , 430);
	CMenuWidget    recordingSettings   (LOCALE_RECORDINGMENU_HEAD            , "recording.raw"       );
	CMenuWidget    streamingSettings   (LOCALE_STREAMINGMENU_HEAD            , "streaming.raw"       );
	CMenuWidget    colorSettings       (LOCALE_COLORMENU_HEAD                , "colors.raw"          );
	CMenuWidget    fontSettings        (LOCALE_FONTMENU_HEAD                 , "colors.raw"          );
	CMenuWidget    lcdSettings         (LOCALE_LCDMENU_HEAD                  , "lcd.raw"             , 420);
	CMenuWidget    keySettings         (LOCALE_KEYBINDINGMENU_HEAD           , "keybinding.raw"      , 400);
	CMenuWidget    miscSettings        (LOCALE_MISCSETTINGS_HEAD             , NEUTRINO_ICON_SETTINGS, 420);
	CMenuWidget    driverSettings      (LOCALE_DRIVERSETTINGS_HEAD             , NEUTRINO_ICON_SETTINGS);
	CMenuWidget    audioplPicSettings  (LOCALE_AUDIOPLAYERPICSETTINGS_GENERAL, NEUTRINO_ICON_SETTINGS);
	CMenuWidget    scanSettings        (LOCALE_SERVICEMENU_SCANTS            , NEUTRINO_ICON_SETTINGS);
	CMenuWidget    service             (LOCALE_SERVICEMENU_HEAD              , NEUTRINO_ICON_SETTINGS);
	CMenuWidget    moviePlayer         (LOCALE_MOVIEPLAYER_HEAD              , "streaming.raw"       );

	InitMainMenu(	mainMenu,
					mainSettings,
					audioSettings,
					parentallockSettings,
					networkSettings,
					recordingSettings,
					colorSettings,
					lcdSettings,
					keySettings,
					videoSettings,
					languageSettings,
					miscSettings,
					driverSettings,
					service,
					fontSettings,
					audioplPicSettings,
					streamingSettings,
					moviePlayer);

	//service
	InitServiceSettings(service, scanSettings);

	//language Setup
	InitLanguageSettings(languageSettings);

	//audioplayer/picviewer Setup
	InitAudioplPicSettings(audioplPicSettings);

	//driver Setup
	InitDriverSettings(driverSettings);

	//misc Setup
	InitMiscSettings(miscSettings);

	//audio Setup
	InitAudioSettings(audioSettings, audioSetupNotifier);

	// Parentallock settings
	InitParentalLockSettings(parentallockSettings);

	// ScanSettings
	InitScanSettings(scanSettings);

	dprintf( DEBUG_NORMAL, "registering as event client\n");

	g_Controld->registerEvent(CControldClient::EVT_MUTECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VOLUMECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_MODECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VCRCHANGED, 222, NEUTRINO_UDS_NAME);

	g_Sectionsd->registerEvent(CSectionsdClient::EVT_TIMESET, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_GOT_CN_EPG, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_SERVICES_UPDATE, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_BOUQUETS_UPDATE, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_WRITE_SI_FINISHED, 222, NEUTRINO_UDS_NAME);

#ifndef SKIP_CA_STATUS
#define ZAPIT_EVENT_COUNT 26
#else
#define ZAPIT_EVENT_COUNT 23
#endif
	const CZapitClient::events zapit_event[ZAPIT_EVENT_COUNT] =
		{
			CZapitClient::EVT_ZAP_COMPLETE,
			CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD,
			CZapitClient::EVT_ZAP_FAILED,
			CZapitClient::EVT_ZAP_SUB_COMPLETE,
			CZapitClient::EVT_ZAP_SUB_FAILED,
			CZapitClient::EVT_ZAP_MOTOR,
#ifndef SKIP_CA_STATUS
			CZapitClient::EVT_ZAP_CA_CLEAR,
			CZapitClient::EVT_ZAP_CA_LOCK,
			CZapitClient::EVT_ZAP_CA_FTA,
#endif
			CZapitClient::EVT_RECORDMODE_ACTIVATED,
			CZapitClient::EVT_RECORDMODE_DEACTIVATED,
			CZapitClient::EVT_SCAN_COMPLETE,
			CZapitClient::EVT_SCAN_FAILED,
			CZapitClient::EVT_SCAN_NUM_TRANSPONDERS,
			CZapitClient::EVT_SCAN_REPORT_NUM_SCANNED_TRANSPONDERS,
			CZapitClient::EVT_SCAN_REPORT_FREQUENCY,
			CZapitClient::EVT_SCAN_REPORT_FREQUENCYP,
			CZapitClient::EVT_SCAN_SATELLITE,
			CZapitClient::EVT_SCAN_NUM_CHANNELS,
			CZapitClient::EVT_SCAN_PROVIDER,
			CZapitClient::EVT_BOUQUETS_CHANGED,
			CZapitClient::EVT_SCAN_SERVICENAME,
			CZapitClient::EVT_SCAN_FOUND_A_CHAN,
			CZapitClient::EVT_SCAN_FOUND_TV_CHAN,
			CZapitClient::EVT_SCAN_FOUND_RADIO_CHAN,
			CZapitClient::EVT_SCAN_FOUND_DATA_CHAN,
		};

	for (int i = 0; i < ZAPIT_EVENT_COUNT; i++)
		g_Zapit->registerEvent(zapit_event[i], 222, NEUTRINO_UDS_NAME);

	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_ON, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_STANDBY_OFF, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_RECORD, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_START, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_RECORD_STOP, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ZAPTO, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_ANNOUNCE_SLEEPTIMER, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_REMIND, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_EXEC_PLUGIN, 222, NEUTRINO_UDS_NAME);

	if (display_language_selection)
		languageSettings.exec(NULL, "");

	InitNetworkSettings(networkSettings);

#ifndef HAVE_DREAMBOX_HARDWARE
	if (!ucodes_available())
	{
		/* display error message */
		DisplayErrorMessage(g_Locale->getText(LOCALE_UCODES_FAILURE));

		/* show network settings dialog */
		networkSettings.exec(NULL, "");
	}
#endif

	//settins
	if(loadSettingsErg==1)
	{
		dprintf(DEBUG_INFO, "config file missing\n");
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SETTINGS_NOCONFFILE));
		configfile.setModifiedFlag(true);
		saveSetup();
	}
	else if(loadSettingsErg==2)
	{
		dprintf(DEBUG_INFO, "parts of configfile missing\n");
		ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SETTINGS_MISSINGOPTIONSCONFFILE));
		configfile.setModifiedFlag(true);
		saveSetup();
	}

	//init programm
	InitZapper();

	//Recording Setup
	InitRecordingSettings(recordingSettings);

	//VLC Setup
	InitStreamingSettings(streamingSettings);

	//font Setup
	InitFontSettings(fontSettings);

	//color Setup
	InitColorSettings(colorSettings, fontSettings);

	//LCD Setup
	InitLcdSettings(lcdSettings);

	//keySettings
	InitKeySettings(keySettings);

	AudioMute( g_Controld->getMute((CControld::volume_type)g_settings.audio_avs_Control), true );

	// shutdown counter
	SHTDCNT::getInstance()->init();

	RealRun(mainMenu);

	ExitRun(true);

	return 0;
}


void CNeutrinoApp::RealRun(CMenuWidget &mainMenu)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	dprintf(DEBUG_NORMAL, "initialized everything\n");

	while( true )
	{
		g_RCInput->getMsg(&msg, &data, 100);	// 10 secs..

		if( ( mode == mode_tv ) || ( ( mode == mode_radio ) ) )
		{
			if( msg == NeutrinoMessages::SHOW_EPG )
			{
				// show EPG

//				g_EpgData->show( channelList->getActiveChannel_ChannelID() );
				g_EpgData->show( g_Zapit->getCurrentServiceID() );

			}
			else if( msg == (neutrino_msg_t) g_settings.key_tvradio_mode )
			{
				if( mode == mode_tv )
				{
					radioMode();
				}
				else if( mode == mode_radio )
				{
					tvMode();
				}
			}
			else if( msg == CRCInput::RC_setup )
			{
				mainMenu.exec(NULL, "");
				// restore mute symbol
				AudioMute(current_muted, true);
			}
			if( msg == CRCInput::RC_ok)
			{
				int bouqMode = g_settings.bouquetlist_mode;//bsmChannels;

				if((bouquetList!=NULL) && (bouquetList->Bouquets.empty()))
				{
					dprintf(DEBUG_DEBUG, "bouquets are empty\n");
					bouqMode = bsmAllChannels;
				}
				if((bouquetList!=NULL) && (bouqMode == 1/*bsmBouquets*/))
				{
					bouquetList->exec(true);
				}
				else if((bouquetList!=NULL) && (bouqMode == 0/*bsmChannels*/))
				{
					int nNewChannel = bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->show();
					if(nNewChannel>-1)
					{
						recordingstatus ? channelList->zapTo(nNewChannel):  channelList->zapTo(bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->getKey(nNewChannel)-1);
					}
				}
				else
				{
					dprintf(DEBUG_DEBUG, "using all channels\n");
					channelList->exec();
				}
			}
#ifdef HAVE_DREAMBOX_HARDWARE
			// support for additional buttons on Dreambox Remote
			else if( msg == CRCInput::RC_tv )
			{
				if( mode != mode_tv )
				{
					tvMode();
				}
			}
			else if( msg == CRCInput::RC_radio )
			{
				if( mode != mode_radio )
				{
					radioMode();
				}
			}
			else if( msg == CRCInput::RC_text )
			{
				g_PluginList->startPlugin("tuxtxt");
			}
			else if( msg == CRCInput::RC_audio )
			{
				// audio channel selection
				showUserMenu(SNeutrinoSettings::BUTTON_GREEN);
			}
			else if( msg == CRCInput::RC_video )
			{
				// nvod selection
				showUserMenu(SNeutrinoSettings::BUTTON_YELLOW);
			}
#endif
			else if( msg == CRCInput::RC_red )
			{	// eventlist
				if (g_settings.personalize_redbutton == 0)
				{
					// EventList Menu - Personalization Check
					ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_PERSONALIZE_MENUDISABLEDHINT));
				} else {
					showUserMenu(SNeutrinoSettings::BUTTON_RED);  //USERMENU
				}
			}
			else if( msg == CRCInput::RC_blue )
			{	// streaminfo
				if (g_settings.personalize_bluebutton == 0)
				{
					// Features Menu - Personalization Check
					ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_PERSONALIZE_MENUDISABLEDHINT));
				} else {
					showUserMenu(SNeutrinoSettings::BUTTON_BLUE);
				}
			}
			else if( msg == CRCInput::RC_green )
			{
				showUserMenu(SNeutrinoSettings::BUTTON_GREEN);
				}
			else if( msg == CRCInput::RC_yellow )
			{       // NVODs
				showUserMenu(SNeutrinoSettings::BUTTON_YELLOW);
			}
			else if (CRCInput::isNumeric(msg) && g_RemoteControl->director_mode )
			{
				g_RemoteControl->setSubChannel(CRCInput::getNumericValue(msg));
				g_InfoViewer->showSubchan();
			}
			else if( ( msg == (neutrino_msg_t) g_settings.key_quickzap_up ) || ( msg == (neutrino_msg_t) g_settings.key_quickzap_down ) )
			{
				//quickzap
				channelList->quickZap( msg );
			}
			else if( ( msg == CRCInput::RC_help ) ||
						( msg == NeutrinoMessages::SHOW_INFOBAR ) )
			{
			         // turn on LCD display
				CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
				// show Infoviewer
				g_InfoViewer->showTitle(channelList->getActiveChannelNumber(), channelList->getActiveChannelName(), channelList->getActiveSatellitePosition(), channelList->getActiveChannel_ChannelID()); // UTF-8
			}
			else if( msg == (neutrino_msg_t) g_settings.key_subchannel_up )
			{
				g_RemoteControl->subChannelUp();
				g_InfoViewer->showSubchan();
			}
			else if( msg == (neutrino_msg_t) g_settings.key_subchannel_down )
			{
				g_RemoteControl->subChannelDown();
				g_InfoViewer->showSubchan();
			}
			else if( msg == (neutrino_msg_t) g_settings.key_zaphistory )
			{
				// Zap-History "Bouquet"
				channelList->numericZap( msg );
			}
			else if( msg == (neutrino_msg_t) g_settings.key_lastchannel )
			{
				// Quick Zap
				channelList->numericZap( msg );
			}
			else if (CRCInput::isNumeric(msg))
			{
				//numeric zap
				channelList->numericZap( msg );
			}
			else
			{     // turn on LCD display by kicking it
				if (msg == CRCInput::RC_home)
					CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
				handleMsg(msg, data);
			}

		}
		else
		{
			// mode == mode_scart
			if( msg == CRCInput::RC_home )
			{
				if( mode == mode_scart )
				{
					//wenn VCR Aufnahme dann stoppen
					if(CVCRControl::getInstance()->isDeviceRegistered())
					{
						if ((CVCRControl::getInstance()->Device->getDeviceType() == CVCRControl::DEVICE_VCR) &&
						    (CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_RECORD ||
						     CVCRControl::getInstance()->getDeviceState() == CVCRControl::CMD_VCR_PAUSE))
						{
							CVCRControl::getInstance()->Stop();
							recordingstatus=0;
							startNextRecording();
						}
					}
					// Scart-Mode verlassen
					scartMode( false );
				}
			}
			else
			{
				handleMsg(msg, data);
			}
		}
	}
}

int CNeutrinoApp::handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data)
{
	int res = 0;

	res = res | g_RemoteControl->handleMsg(msg, data);
	res = res | g_InfoViewer->handleMsg(msg, data);
	res = res | channelList->handleMsg(msg, data);

	if( res != messages_return::unhandled )
	{
		if( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
			delete (unsigned char*) data;
		return( res & ( 0xFFFFFFFF - messages_return::unhandled ) );
	}

	if (!waitforshutdown) {
	if( msg == NeutrinoMessages::EVT_VCRCHANGED )
	{
		if (g_settings.vcr_AutoSwitch)
		{
			if( data != VCR_STATUS_OFF )
				g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
			else
				g_RCInput->postMsg( NeutrinoMessages::VCR_OFF, 0 );
		}
		return messages_return::handled | messages_return::cancel_info;
	}
	else if (msg == CRCInput::RC_standby)
	{
		if (data == 0)
		{
			neutrino_msg_t new_msg;

			/* Note: pressing the power button on the dbox (not the remote control) over 1 second */
			/*       shuts down the system even if !g_settings.shutdown_real_rcdelay (see below)  */
			gettimeofday(&standby_pressed_at, NULL);

			if ((mode != mode_standby) && (g_settings.shutdown_real))
			{
				new_msg = NeutrinoMessages::SHUTDOWN;
			}
			else
			{
				new_msg = (mode == mode_standby) ? NeutrinoMessages::STANDBY_OFF : NeutrinoMessages::STANDBY_ON;

				if ((g_settings.shutdown_real_rcdelay))
				{
					neutrino_msg_t      msg;
					neutrino_msg_data_t data;
					struct timeval      endtime;
					time_t              seconds;

					int timeout = 0;
					int timeout1 = 0;

					sscanf(g_settings.repeat_blocker, "%d", &timeout);
					sscanf(g_settings.repeat_genericblocker, "%d", &timeout1);

					if (timeout1 > timeout)
						timeout = timeout1;

					timeout += 500;

					while(true)
					{
						g_RCInput->getMsg_ms(&msg, &data, timeout);

						if (msg == CRCInput::RC_timeout)
							break;

						gettimeofday(&endtime, NULL);
						seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
						if (endtime.tv_usec < standby_pressed_at.tv_usec)
							seconds--;
						if (seconds >= 1)
						{
							new_msg = NeutrinoMessages::SHUTDOWN;
							break;
						}
					}
				}
			}
			g_RCInput->postMsg(new_msg, 0);
			return messages_return::cancel_all | messages_return::handled;
		}
		else                                        /* data == 1: KEY_POWER released                         */
			if (standby_pressed_at.tv_sec != 0) /* check if we received a KEY_POWER pressed event before */
			{                                   /* required for correct handling of KEY_POWER events of  */
				                            /* the power button on the dbox (not the remote control) */
				struct timeval endtime;
				gettimeofday(&endtime, NULL);
				time_t seconds = endtime.tv_sec - standby_pressed_at.tv_sec;
				if (endtime.tv_usec < standby_pressed_at.tv_usec)
					seconds--;
				if (seconds >= 1)
				{
					g_RCInput->postMsg(NeutrinoMessages::SHUTDOWN, 0);
					return messages_return::cancel_all | messages_return::handled;
				}
			}
	}
	else if ((msg == CRCInput::RC_plus) ||
		 (msg == CRCInput::RC_minus) ||
		 (msg == NeutrinoMessages::EVT_VOLCHANGED))
	{
		setVolume(msg, (mode != mode_scart));
		return messages_return::handled;
	}
	else if( msg == CRCInput::RC_spkr )
	{
		if( mode == mode_standby )
		{
			//switch lcd off/on
			CLCD::getInstance()->togglePower();
		}
		else
		{
			//mute
			AudioMute( !current_muted );
		}
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_MUTECHANGED )
	{
		CControldMsg::commandMute* cmd = (CControldMsg::commandMute*) data;
		if(cmd->type == (CControld::volume_type)g_settings.audio_avs_Control)
			AudioMute( cmd->mute, true );
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::EVT_RECORDMODE )
	{
		dprintf(DEBUG_DEBUG, "neutrino - recordmode %s\n", ( data ) ? "on":"off" );

		recordingstatus = data;

		if( ( !g_InfoViewer->is_visible ) && data )
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

		static t_channel_id old_parent_id;
		t_channel_id old_id = g_Zapit->getCurrentServiceID();
		if (data) {
			old_parent_id = channelList->getActiveChannel_ChannelID();
			// if record on - get channelList from zapit
//			channelsInit(init_mode_init);
			channelsInit4Record();
		} else {
			// if record off - switch channelList to old mode
			if (g_Zapit->isChannelTVChannel(old_parent_id))
				channelsInit(init_mode_switch, mode_tv);
			else 
				channelsInit(init_mode_switch, mode_radio);
		}

		// if a neutrino channel for current channel_id cannot be found (eg tuned to a sub service)
		// adjust to old main channel
		if (!channelList->adjustToChannelID(old_id) && !data)
			channelList->adjustToChannelID(old_parent_id);

		if(old_id == 0)
			channelList->zapTo(0);
	}
	else if( msg == NeutrinoMessages::EVT_BOUQUETSCHANGED )			// EVT_BOUQUETSCHANGED: initiated by zapit ; EVT_SERVICESCHANGED: no longer used
	{
		t_channel_id old_id = channelList->getActiveChannel_ChannelID();

		channelsInit(init_mode_init);

		if((old_id == 0) || (!(channelList->adjustToChannelID(old_id))))
			channelList->zapTo(0);

		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::RECORD_START)
	{
		execute_start_file(NEUTRINO_RECORDING_START_SCRIPT);
		/* set nextRecordingInfo to current event (replace other scheduled recording if available) */

		/*
		 * Note: CTimerd::RecordingInfo is a class!
		 * What a brilliant idea to send classes via the eventserver!
		 * => typecast to avoid destructor call
		 */
		if (nextRecordingInfo != NULL)
			delete (unsigned char *) nextRecordingInfo;

		nextRecordingInfo = (CTimerd::RecordingInfo *) data;

		startNextRecording();

		return messages_return::handled | messages_return::cancel_all;
	}
	else if( msg == NeutrinoMessages::RECORD_STOP)
	{
		if(((CTimerd::RecordingStopInfo*)data)->eventID==recording_id)
		{ // passendes stop zur Aufnahme
			CVCRControl * vcr_control = CVCRControl::getInstance();
			if (vcr_control->isDeviceRegistered())
			{
				if ((vcr_control->getDeviceState() == CVCRControl::CMD_VCR_RECORD) ||
				    (vcr_control->getDeviceState() == CVCRControl::CMD_VCR_PAUSE ))
				{
					vcr_control->Stop();
					recordingstatus = 0;
				}
				else
					printf("falscher state\n");
			}
			else
				puts("[neutrino.cpp] no recording devices");

			startNextRecording();

			if (recordingstatus == 0)
			{
				execute_start_file(NEUTRINO_RECORDING_ENDED_SCRIPT);
			}
		}
		else if(nextRecordingInfo!=NULL)
		{
			if(((CTimerd::RecordingStopInfo*)data)->eventID == nextRecordingInfo->eventID)
			{

				/*
				 * Note: CTimerd::RecordingInfo is a class!
				 * What a brilliant idea to send classes via the eventserver!
				 * => typecast to avoid destructor call
				 */
				delete (unsigned char *) nextRecordingInfo;

				nextRecordingInfo=NULL;
			}
		}
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ZAPTO)
	{
		CTimerd::EventInfo * eventinfo;
		eventinfo = (CTimerd::EventInfo *) data;
		if(recordingstatus==0)
		{
			bool isTVMode = g_Zapit->isChannelTVChannel(eventinfo->channel_id);

			if ((!isTVMode) && (mode != mode_radio))
			{
				radioMode(false);
				channelsInit(init_mode_switch, mode_radio);
			}
			else if (isTVMode && (mode != mode_tv))
			{
				tvMode(false);
				channelsInit(init_mode_switch, mode_tv);
			}
			channelList->zapTo_ChannelID(eventinfo->channel_id);
		}
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_ZAPTO)
	{
		if( mode == mode_standby )
		{
			// WAKEUP
			standbyMode( false );
		}
		if( mode != mode_scart )
		{
			CTimerd::TimerList tmpTimerList;
			CTimerdClient tmpTimerdClient;

			tmpTimerList.clear();
			tmpTimerdClient.getTimerList( tmpTimerList );

			sort( tmpTimerList.begin(), tmpTimerList.end() );

			CTimerd::responseGetTimer &timer = tmpTimerList[0];

			CZapitClient Zapit;
			std::string name = g_Locale->getText(LOCALE_ZAPTOTIMER_ANNOUNCE);
			name += "\n";

			std::string zAddData = Zapit.getChannelName( timer.channel_id ); // UTF-8
			if( zAddData.empty() )
			{
				zAddData = g_Locale->getText(LOCALE_TIMERLIST_PROGRAM_UNKNOWN);
			}

			if(timer.epgID!=0)
			{
				CEPGData epgdata;
#warning fixme sectionsd should deliver data in UTF-8 format
				zAddData += " :\n";
				if (g_Sectionsd->getEPGid(timer.epgID, timer.epg_starttime, &epgdata))
				{
					zAddData += Latin1_to_UTF8(epgdata.title);
				}
				else if(strlen(timer.epgTitle)!=0)
				{
					zAddData += Latin1_to_UTF8(timer.epgTitle);
				}
			}
			else if(strlen(timer.epgTitle)!=0)
			{
				zAddData += Latin1_to_UTF8(timer.epgTitle);
			}

			name += zAddData;
			ShowHintUTF( LOCALE_MESSAGEBOX_INFO, name.c_str() );
//			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_ZAPTOTIMER_ANNOUNCE));
		}

		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_RECORD)
	{
		execute_start_file(NEUTRINO_RECORDING_TIMER_SCRIPT);

		if( g_settings.recording_server_wakeup )
		{
			std::string command = "etherwake ";
			command += g_settings.recording_server_mac;
			if(system(command.c_str()) != 0)
				perror("etherwake failed");
		}
		if (g_settings.recording_type == RECORDING_FILE)
		{
			char * recDir = ((CTimerd::RecordingInfo*)data)->recordingDir;
			for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
			{
				if (strcmp(g_settings.network_nfs_local_dir[i],recDir) == 0)
				{
					printf("[neutrino] waking up %s (%s)\n",g_settings.network_nfs_ip[i].c_str(),recDir);
					std::string command = "etherwake ";
					command += g_settings.network_nfs_mac[i];
					if(system(command.c_str()) != 0)
						perror("etherwake failed");
					break;
				}
			}
		}
		if( g_settings.recording_zap_on_announce )
		{
			if(recordingstatus==0)
			{
				t_channel_id channel_id=((CTimerd::RecordingInfo*)data)->channel_id;
				g_Zapit->zapTo_serviceID_NOWAIT(channel_id);
			}
		}
		delete (unsigned char*) data;
		if( mode != mode_scart )
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_RECORDTIMER_ANNOUNCE));
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SLEEPTIMER)
	{
		if( mode != mode_scart )
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SLEEPTIMERBOX_ANNOUNCE));
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::SLEEPTIMER)
	{
		CIRSend irs("sleep");
		irs.Send();

		if(g_settings.shutdown_real)
			ExitRun(true);
		else
			standbyMode( true );
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_TOGGLE )
	{
		standbyMode( !(mode & mode_standby) );
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_ON )
	{
		if( mode != mode_standby )
		{
			// noch nicht im Standby-Mode...
			standbyMode( true );
		}
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::STANDBY_OFF )
	{
		if( mode == mode_standby )
		{
			// WAKEUP
			standbyMode( false );
		}
		g_RCInput->clearRCMsg();
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::ANNOUNCE_SHUTDOWN)
	{
		if( mode != mode_scart )
			skipShutdownTimer = (ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SHUTDOWNTIMER_ANNOUNCE, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 5) == CMessageBox::mbrYes);
	}
	else if( msg == NeutrinoMessages::SHUTDOWN )
	{
		// AUSSCHALTEN...
		if(!skipShutdownTimer)
		{
			ExitRun(true);
		}
		else
		{
			skipShutdownTimer=false;
		}
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_POPUP)
	{
		if (mode != mode_scart)
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, (const char *) data); // UTF-8
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_EXTMSG)
	{
		if (mode != mode_scart)
			ShowMsgUTF(LOCALE_MESSAGEBOX_INFO, (const char *) data, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_RECORDING_ENDED)
	{
		if (mode != mode_scart)
		{
			neutrino_locale_t msgbody;

			if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_IDLE)
				msgbody = LOCALE_STREAMING_SUCCESS;
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_BUFFER_OVERFLOW)
				msgbody = LOCALE_STREAMING_BUFFER_OVERFLOW;
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_OPEN_FAILURE)
				msgbody = LOCALE_STREAMING_WRITE_ERROR_OPEN;
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_WRITE_FAILURE)
				msgbody = LOCALE_STREAMING_WRITE_ERROR;
			else if ((* (stream2file_status2_t *) data).status == STREAM2FILE_STATUS_RECORDING_THREADS_FAILED)
				msgbody = LOCALE_STREAMING_OUT_OF_MEMORY;
			else
				goto skip_message;

			/*
			 * use a short timeout of only 5 seconds in case it was only a temporary network problem
			 * in case of STREAM2FILE_STATUS_IDLE we might even have to immediately start the next recording
			 */
#warning TODO: it might make some sense to have some log-file (but where do we store this information? nfs/flash/ram?) that collects these messages and maybe a menu-entry to view the lasted XXX messages
			ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, msgbody, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO, 450, 5);

		skip_message:
			;
		}
		if (((* (stream2file_status2_t *) data).status != STREAM2FILE_STATUS_IDLE) && ((* (stream2file_status2_t *) data).status != STREAM2FILE_STATUS_RECORDING_THREADS_FAILED))
		{
#warning TODO: count restart-rate to catch endless loops
			/*
			 * note that changeNotify does not distinguish between LOCALE_MAINMENU_RECORDING_START and LOCALE_MAINMENU_RECORDING_STOP
			 * instead it checks the state of the variable recordingstatus
			 */
			/* restart recording */
			doGuiRecord((*(stream2file_status2_t *) data).dir);
			//changeNotify(LOCALE_MAINMENU_RECORDING_START, data);
		}

		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::REMIND)
	{
		std::string text = (char*)data;
		std::string::size_type pos;
		while((pos=text.find('/'))!= std::string::npos)
		{
			text[pos] = '\n';
		}
		if( mode != mode_scart )
			ShowMsgUTF(LOCALE_TIMERLIST_TYPE_REMIND, text, CMessageBox::mbrBack, CMessageBox::mbBack, NEUTRINO_ICON_INFO); // UTF-8
		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::LOCK_RC)
	{
		this->rcLock->exec(NULL,CRCLock::NO_USER_INPUT);
		return messages_return::handled;
	}
	else if( msg == NeutrinoMessages::CHANGEMODE )
	{
		if((data & mode_mask)== mode_radio)
		{
			if( mode != mode_radio )
				if((data & norezap)==norezap)
					radioMode(false);
				else
					radioMode(true);
		}
		if((data & mode_mask)== mode_tv)
		{
			if( mode != mode_tv )
				if((data & norezap)==norezap)
					tvMode(false);
				else
					tvMode(true);
		}
		if((data &mode_mask)== mode_standby)
		{
			if(mode != mode_standby)
				standbyMode( true );
		}
		if((data &mode_mask)== mode_audio)
		{
			lastMode=mode;
			mode=mode_audio;
		}
		if((data &mode_mask)== mode_pic)
		{
			lastMode=mode;
			mode=mode_pic;
		}
		if((data &mode_mask)== mode_ts)
		{
			lastMode=mode;
			mode=mode_ts;
		}
	}
	else if( msg == NeutrinoMessages::VCR_ON )
	{
		if( mode != mode_scart )
		{
			// noch nicht im Scart-Mode...
			scartMode( true );
		}
		else // sonst nur lcd akt.
			CLCD::getInstance()->setMode(CLCD::MODE_SCART);
	}

	else if( msg == NeutrinoMessages::VCR_OFF )
	{
		if( mode == mode_scart )
		{
			// noch nicht im Scart-Mode...
			scartMode( false );
		}
	}
	else if (msg == NeutrinoMessages::EVT_START_PLUGIN)
	{
		g_PluginList->startPlugin((const char *)data);
		std::string output = g_PluginList->getScriptOutput();
		if (!g_PluginList->getScriptOutput().empty())
		{
			ShowMsgUTF(LOCALE_PLUGINS_RESULT, Latin1_to_UTF8(g_PluginList->getScriptOutput()),
					   CMessageBox::mbrBack,CMessageBox::mbBack,NEUTRINO_ICON_SHELL);
		}

		delete (unsigned char*) data;
		return messages_return::handled;
	}
	else if (msg == NeutrinoMessages::EVT_SERVICES_UPD)
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO,
		g_Locale->getText(LOCALE_SERVICEMENU_RELOAD_HINT));
		hintBox->paint();
		g_Zapit->reloadCurrentServices();
		hintBox->hide();
		delete hintBox;
	}
	}
	else {
	if (msg == NeutrinoMessages::EVT_SI_FINISHED)
	{
		waitforshutdown = false;
		ExitRun(false);
	}
	}
	if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000))
		delete (unsigned char*) data;

	return messages_return::unhandled;
}

void CNeutrinoApp::ExitRun(const bool write_si)
{
	  //DisplayErrorMessage(g_Locale->getText(LOCALE_SHUTDOWNERROR_RECODING));
	if ((!recordingstatus ||
	    ShowLocalizedMessage(LOCALE_MESSAGEBOX_INFO, LOCALE_SHUTDOWN_RECODING_QUERY, CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo, NULL, 450, 30, true) == CMessageBox::mbrYes) && (!waitforshutdown))
	{

		if (write_si) {

			CLCD::getInstance()->setMode(CLCD::MODE_SHUTDOWN);

			dprintf(DEBUG_INFO, "exit\n");
			for(int x=0;x<256;x++)
				frameBuffer->paletteSetColor(x, 0x000000, 0xffff);
			frameBuffer->paletteSet();

			frameBuffer->loadPicture2FrameBuffer("shutdown.raw");
			frameBuffer->loadPal("shutdown.pal");

			networkConfig.automatic_start = (network_automatic_start == 1);
			networkConfig.commitConfig();
			saveSetup();

			if (frameBuffer != NULL)
				delete frameBuffer;

			if (g_settings.epg_dir.length() != 0) {
				waitforshutdown = true;
				AudioMute(true);
				g_Sectionsd->writeSI2XML(g_settings.epg_dir.c_str());
			}
			else {
				g_Controld->shutdown();
				if (g_RCInput != NULL)
					delete g_RCInput;

				exit(0);

			}
		} else {
			AudioMute(false);
			g_Controld->shutdown();
			if (g_RCInput != NULL)
				delete g_RCInput;

			exit(0);

		}
	}
}

void CNeutrinoApp::AudioMute( bool newValue, bool isEvent )
{
#ifndef HAVE_DREAMBOX_HARDWARE
	if((CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC) // lirc
	{ // bei LIRC wissen wir nicht wikrlich ob jetzt ge oder entmuted wird, deswegen nix zeigen---
		if( !isEvent )
			g_Controld->Mute((CControld::volume_type)g_settings.audio_avs_Control);
	}
	else
#endif
	{
		int dx = 40;
		int dy = 40;
		int x = g_settings.screen_EndX-dx;
		int y = g_settings.screen_StartY;

		CLCD::getInstance()->setMuted(newValue);
		if( newValue != current_muted )
		{
			current_muted = newValue;

			if( !isEvent )
			{
				if( current_muted )
					g_Controld->Mute((CControld::volume_type)g_settings.audio_avs_Control);
				else
					g_Controld->UnMute((CControld::volume_type)g_settings.audio_avs_Control);
			}
		}

		if( isEvent && ( mode != mode_scart ) && ( mode != mode_audio) && ( mode != mode_pic) && ( g_settings.widget_osd != 2 ) )
		{
		// anzeigen NUR, wenn es vom Event kommt
			if( current_muted )
			{
				frameBuffer->paintBoxRel(x, y, dx, dy, COL_INFOBAR_PLUS_0);
				frameBuffer->paintIcon(NEUTRINO_ICON_BUTTON_MUTE, x+5, y+5);
			}
			else
				frameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
		}
	}
}

void CNeutrinoApp::setVolume(const neutrino_msg_t key, const bool bDoPaint)
{
	neutrino_msg_t msg = key;

	int dx = 256;
	int dy = 40;
	int x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
	int y = g_settings.screen_EndY- 100;

	if( g_settings.widget_osd == 1 )
	{
		y = y + 50;
	}

	fb_pixel_t * pixbuf = NULL;

	if( (bDoPaint) && (g_settings.widget_osd != 2 ) ) 
	{
		pixbuf = new fb_pixel_t[dx * dy];
		if(pixbuf!= NULL)
			frameBuffer->SaveScreen(x, y, dx, dy, pixbuf);
		frameBuffer->paintIcon("volume.raw",x,y, COL_INFOBAR);
	}

	neutrino_msg_data_t data;

	unsigned long long timeoutEnd;

	char current_volume = g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control);

	do
	{
		if (msg <= CRCInput::RC_MaxRC)
		{
			if (msg == CRCInput::RC_plus)
			{
#ifndef HAVE_DREAMBOX_HARDWARE
				if((CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC)
				{
					current_volume = 60; //>50 is plus
				}
				else
				{
					if (current_volume < 100 - 5)
						current_volume += 5;
					else
						current_volume = 100;
				}
#else
				// The dreambox 500 has 32 steps, so 3% match much better than 5% steps - seife
				if (current_volume < 100 - 3)
					current_volume += 3;
				else
					current_volume = 100;
#endif
			}
			else if (msg == CRCInput::RC_minus)
			{
#ifndef HAVE_DREAMBOX_HARDWARE
				if((CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC)
				{
					current_volume = 40; //<40 is minus
				}
				else
				{
					if (current_volume > 5)
						current_volume -= 5;
					else
						current_volume = 0;
				}
#else
				if (current_volume > 3)
					current_volume -= 3;
				else
					current_volume = 0;
#endif
			}
			else
			{
				g_RCInput->postMsg(msg, data);
				break;
			}

			g_Controld->setVolume(current_volume, (CControld::volume_type)g_settings.audio_avs_Control);

#ifndef HAVE_DREAMBOX_HARDWARE
			if((CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC)
			{
				current_volume = 50;
			}
#endif
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] / 2);
		}
		else if (msg == NeutrinoMessages::EVT_VOLCHANGED)
		{
			current_volume = g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control);
			timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing[SNeutrinoSettings::TIMING_INFOBAR] / 2);
		}
		else if (handleMsg(msg, data) & messages_return::unhandled)
		{
			g_RCInput->postMsg(msg, data);
			break;
		}

		if( (bDoPaint) && (g_settings.widget_osd != 2 ) )
		{
			int vol = current_volume << 1;
			frameBuffer->paintBoxRel(x + 40      , y + 12, vol      , 15, COL_INFOBAR_PLUS_3);
			frameBuffer->paintBoxRel(x + 40 + vol, y + 12, 200 - vol, 15, COL_INFOBAR_PLUS_1);
		}

		CLCD::getInstance()->showVolume(current_volume);
		if (msg != CRCInput::RC_timeout)
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd );
		}

		}
	while (msg != CRCInput::RC_timeout);

	if( (bDoPaint) && (g_settings.widget_osd != 2 ) && (pixbuf!= NULL) )
	{
		frameBuffer->RestoreScreen(x, y, dx, dy, pixbuf);
		delete [] pixbuf;
	}
}

void CNeutrinoApp::tvMode( bool rezap )
{
	if(mode==mode_radio )
	{
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_TV_MODE, false );

#ifndef HAVE_DREAMBOX_HARDWARE
		if(g_settings.misc_spts==1)
			g_Zapit->PlaybackSPTS();
#endif
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	if( mode == mode_tv )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}

	mode = mode_tv;

	if(g_settings.video_Format != CControldClient::VIDEOFORMAT_4_3)
		g_Controld->setVideoFormat(g_settings.video_Format);

	//printf( "tv-mode\n" );

	frameBuffer->useBackground(false);
	frameBuffer->paintBackground();

	g_RemoteControl->tvMode();
	if( rezap )
	{
		firstChannel();
		channelsInit(init_mode_switch, mode_tv);
		channelList->zapTo( firstchannel.channelNumber -1 );
	}
}

void CNeutrinoApp::scartMode( bool bOnOff )
{
	//printf( ( bOnOff ) ? "mode: scart on\n" : "mode: scart off\n" );

	if( bOnOff )
	{
		// SCART AN
		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();

		g_Controld->setScartMode( 1 );
		CLCD::getInstance()->setMode(CLCD::MODE_SCART);
		lastMode = mode;
		mode = mode_scart;
	}
	else
	{
		// SCART AUS
		g_Controld->setScartMode( 0 );

		mode = mode_unknown;

		//re-set mode
		if( lastMode == mode_radio )
		{
			radioMode( false );
		}
		else if( lastMode == mode_tv )
		{
			tvMode( false );
		}
		else if( lastMode == mode_standby )
		{
			standbyMode( true );
		}
	}
}

void CNeutrinoApp::standbyMode( bool bOnOff )
{
	//printf( ( bOnOff ) ? "mode: standby on\n" : "mode: standby off\n" );

	if( bOnOff )
	{
		// STANDBY AN

		if( mode == mode_scart )
		{
			g_Controld->setScartMode( 0 );
		}

		frameBuffer->useBackground(false);
		frameBuffer->paintBackground();

		CLCD::getInstance()->setMode(CLCD::MODE_STANDBY);
		g_Controld->videoPowerDown(true);

		execute_start_file(NEUTRINO_ENTER_STANDBY_SCRIPT);

		lastMode = mode;
		mode = mode_standby;

		//Send ir
		CIRSend irs("sbon");
		irs.Send();
	}
	else
	{
		// STANDBY AUS

		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);

		execute_start_file(NEUTRINO_LEAVE_STANDBY_SCRIPT);

		//Send ir
		CIRSend irs("sboff");
		irs.Send();

		mode = mode_unknown;

		//re-set mode
		if( lastMode == mode_radio )
		{
			radioMode( false );
		}
		else
		{
			tvMode( false );
		}
	}
}

void CNeutrinoApp::radioMode( bool rezap)
{
	if(mode==mode_tv )
	{
		g_RCInput->killTimer(g_InfoViewer->lcdUpdateTimer);
		g_InfoViewer->lcdUpdateTimer = g_RCInput->addTimer( LCD_UPDATE_TIME_RADIO_MODE, false );

#ifndef HAVE_DREAMBOX_HARDWARE
		if(g_settings.misc_spts==1)
			g_Zapit->PlaybackPES();
#endif
	}

	CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
	if( mode==mode_radio )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		CLCD::getInstance()->setMode(CLCD::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}

	mode = mode_radio;

	if(g_settings.video_Format != CControldClient::VIDEOFORMAT_4_3)
		g_Controld->setVideoFormat(CControldClient::VIDEOFORMAT_4_3);

	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->useBackground(frameBuffer->loadBackground("radiomode.raw"));// set useBackground true or false
	frameBuffer->paintBackground();

	g_RemoteControl->radioMode();
	if( rezap )
	{
		firstChannel();
		channelsInit(init_mode_switch, mode_radio);
		channelList->zapTo( firstchannel.channelNumber -1 );
	}
}

void CNeutrinoApp::startNextRecording()
{
	if ((recordingstatus   == 0   ) &&
	    (nextRecordingInfo != NULL))
	{
		bool doRecord = true;
		if (CVCRControl::getInstance()->isDeviceRegistered())
		{
			recording_id = nextRecordingInfo->eventID;
			if (g_settings.recording_type == RECORDING_FILE)
			{
				const char *recDir = strlen(nextRecordingInfo->recordingDir) > 0 ?
					nextRecordingInfo->recordingDir : g_settings.recording_dir[0].c_str();

				int free = getFreeDiscSpaceGB(recDir);
				printf("[neutrino.cpp] getFreeDiscSpaceGB %d\n",free);
				if (free < 2)
				{
					int dirid = getFirstFreeRecDirNr(2);
					if (dirid != -1)
						recDir = g_settings.recording_dir[dirid].c_str();
					printf("[neutrino.cpp] getFirstFreeRecDirNr %d\n",dirid);
				}
				if (!CFSMounter::isMounted(recDir))
				{
					printf("[neutrino.cpp] trying to mount %s\n",recDir);
					doRecord = false;
					for(int i=0 ; i < NETWORK_NFS_NR_OF_ENTRIES ; i++)
					{
						if (strcmp(g_settings.network_nfs_local_dir[i],recDir) == 0)
						{
							CFSMounter::MountRes mres =
								CFSMounter::mount(g_settings.network_nfs_ip[i].c_str(), g_settings.network_nfs_dir[i],
										  g_settings.network_nfs_local_dir[i], (CFSMounter::FSType) g_settings.network_nfs_type[i],
										  g_settings.network_nfs_username[i], g_settings.network_nfs_password[i],
										  g_settings.network_nfs_mount_options1[i], g_settings.network_nfs_mount_options2[i]);
							if (mres == CFSMounter::MRES_OK)
							{
								printf("[neutrino.cpp] mount successful\n");
								doRecord = true;
							} else {
								std::string msg = mntRes2Str(mres) + "\nDir: " + nextRecordingInfo->recordingDir;
								ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, msg.c_str()); // UTF-8
								doRecord = false;
							}
							break;
						}
					}
					if (!doRecord)
					{
						// recording dir does not seem to exist in config anymore
						// or an error occured while mounting
						// -> try default dir
						recDir = g_settings.recording_dir[0].c_str();
						doRecord = true;
					}
				}
				if (doRecord)
				{
					printf("[neutrino.cpp] recording to %s\n",recDir);
					CVCRControl::CFileDevice *fileDevice;
					if ((fileDevice = dynamic_cast<CVCRControl::CFileDevice*>(recordingdevice)) != NULL)
					{
						fileDevice->Directory = recDir;
						fileDevice->FilenameTemplate = g_settings.recording_filename_template[0];
					} else
					{
						puts("[neutrino] could not set directory and filename template");
					}
				}
			}
			if(doRecord && CVCRControl::getInstance()->Record(nextRecordingInfo))
				recordingstatus = 1;
			else
				recordingstatus = 0;
		}
		else
			puts("[neutrino.cpp] no recording devices");

		/*
		 * Note: CTimerd::RecordingInfo is a class!
		 * What a brilliant idea to send classes via the eventserver!
		 * => typecast to avoid destructor call
		 */
		delete (unsigned char *) nextRecordingInfo;

		nextRecordingInfo = NULL;
	}
}
/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  exec, menuitem callback (shutdown)                         *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//	printf("ac: %s\n", actionKey.c_str());
	int returnval = menu_return::RETURN_REPAINT;

	if(actionKey == "help_recording")
	{
		ShowLocalizedMessage(LOCALE_SETTINGS_HELP, LOCALE_RECORDINGMENU_HELP, CMessageBox::mbrBack, CMessageBox::mbBack);
	}
	else if(actionKey=="shutdown")
	{
		ExitRun(true);
		returnval = menu_return::RETURN_NONE;
	}
	else if(actionKey=="reboot")
	{
		FILE *f = fopen("/tmp/.reboot", "w");
		fclose(f);
		ExitRun(true);
		returnval = menu_return::RETURN_NONE;
	}
	else if(actionKey=="tv")
	{
		tvMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="radio")
	{
		radioMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="scart")
	{
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="network")
	{
		networkConfig.automatic_start = (network_automatic_start == 1);
		networkConfig.commitConfig();
		networkConfig.stopNetwork();
		networkConfig.startNetwork();
	}
	else if(actionKey=="networktest")
	{
		dprintf(DEBUG_INFO, "doing network test...\n");
		testNetworkSettings(networkConfig.address.c_str(), networkConfig.netmask.c_str(), networkConfig.broadcast.c_str(), networkConfig.gateway.c_str(), networkConfig.nameserver.c_str(), networkConfig.inet_static);
	}
	else if(actionKey=="networkshow")
	{
		dprintf(DEBUG_INFO, "showing current network settings...\n");
		showCurrentNetworkSettings();
	}
	else if (actionKey=="theme_neutrino")
	{
		setupColors_neutrino();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if (actionKey=="theme_classic")
	{
		setupColors_classic();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if (actionKey=="theme_dblue")
	{
		setupColors_dblue();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if (actionKey=="theme_virgin")
	{
		setupColors_virginmedia();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if (actionKey=="theme_dvb2k")
	{
		setupColors_dvb2k();
		colorSetupNotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if(actionKey=="savesettings")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT)); // UTF-8
		hintBox->paint();

		g_Controld->saveSettings();
		networkConfig.automatic_start = (network_automatic_start == 1);
		networkConfig.commitConfig();
		saveSetup();

		/* send motor position list to zapit */
		if (scanSettings.diseqcMode == DISEQC_1_2)
		{
			printf("[neutrino] sending motor positions list to zapit...\n");
			CZapitClient::ScanMotorPosList motorPosList;
			CNeutrinoApp::getInstance()->getScanSettings().toMotorPosList(motorPosList);
			g_Zapit->setScanMotorPosList(motorPosList);
		}
		// Houdini set DiseqcType/Repeat so you don't have to reboot for changes to take effect
		/* send diseqc type to zapit */
		g_Zapit->setDiseqcType(CNeutrinoApp::getInstance()->getScanSettings().diseqcMode);
		/* send diseqc repeat to zapit */
		g_Zapit->setDiseqcRepeat(CNeutrinoApp::getInstance()->getScanSettings().diseqcRepeat);

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="recording")
	{
		setupRecordingDevice();
	}
	else if(actionKey=="reloadchannels")
	{
	 	CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RELOAD_HINT));
		hintBox->paint();

		g_Zapit->reinitChannels();

		int result = system(mode == mode_radio
				    ? "wget -q -O /dev/null http://127.0.0.1/control/setmode?radio > /dev/null 2>&1"
				    : "wget -q -O /dev/null http://127.0.0.1/control/setmode?tv > /dev/null 2>&1");
		if (result)
			perror("Kicking nhttpd failed");

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="reloadplugins")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_GETPLUGINS_HINT));
		hintBox->paint();

		g_PluginList->loadPlugins();

		hintBox->hide();
		delete hintBox;
	}
	else if(actionKey=="restart")
	{
		if (recordingstatus)
			DisplayErrorMessage(g_Locale->getText(LOCALE_SERVICEMENU_RESTART_REFUSED_RECORDING));
		else {
			CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_SERVICEMENU_RESTART_HINT));
			hintBox->paint();
			execvp(global_argv[0], global_argv); // no return if successful
			DisplayErrorMessage(g_Locale->getText(LOCALE_SERVICEMENU_RESTART_FAILED));

			hintBox->hide();
			delete hintBox;
		}
	}
	else if(strncmp(actionKey.c_str(), "fontsize.d", 10) == 0)
	{
		for (int i = 0; i < 6; i++)
		{
			if (actionKey == font_sizes_groups[i].actionkey)
				for (unsigned int j = 0; j < font_sizes_groups[i].count; j++)
				{
					SNeutrinoSettings::FONT_TYPES k = font_sizes_groups[i].content[j];
					configfile.setInt32(locale_real_names[neutrino_font[k].name], neutrino_font[k].defaultsize);
				}
		}
		fontsizenotifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else if(actionKey=="osd.def")
	{
		for (int i = 0; i < TIMING_SETTING_COUNT; i++)
			g_settings.timing[i] = default_timing[i];

		SetupTiming();
	}
	else if(actionKey == "audioplayerdir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.network_nfs_audioplayerdir))
			strncpy(g_settings.network_nfs_audioplayerdir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_audioplayerdir)-1);
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "picturedir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.network_nfs_picturedir))
			strncpy(g_settings.network_nfs_picturedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_picturedir)-1);
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "moviedir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.network_nfs_moviedir))
			strncpy(g_settings.network_nfs_moviedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_moviedir)-1);
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "epgdir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.epg_dir.c_str()))
		{
			if((b.getSelectedFile()->Name) == "/")
			{
				// if selected dir is root -> clear epg_dir
				g_settings.epg_dir = "";
			} else {
				g_settings.epg_dir = b.getSelectedFile()->Name + "/";
			}
			SendSectionsdConfig(); // update notifier
		}
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "movieplugin")
	{
		parent->hide();
		CMenuWidget MoviePluginSelector(LOCALE_MOVIEPLAYER_DEFPLUGIN, "features.raw", 350);
		MoviePluginSelector.addItem(GenericMenuSeparator);

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
				MoviePluginSelector.addItem(new CMenuForwarderNonLocalized(g_PluginList->getName(count), true, NULL, MoviePluginChanger, id, CRCInput::convertDigitToKey(count)), (cnt == 0));
				cnt++;
			}
		}

		MoviePluginSelector.exec(NULL, "");
 		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "clearSectionsd")
	{
		g_Sectionsd->freeMemory();
	}
	
	return returnval;
}

/**************************************************************************************
*                                                                                     *
*          changeNotify - features menu recording start / stop                        *
*                                                                                     *
**************************************************************************************/
bool CNeutrinoApp::changeNotify(const neutrino_locale_t OptionName, void *data)
{
	if ((ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING_START)) || (ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING)))
	{
		return doGuiRecord(NULL,ARE_LOCALES_EQUAL(OptionName, LOCALE_MAINMENU_RECORDING));
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_LANGUAGESETUP_SELECT))
	{
		bool use_true_unicode_font = g_Locale->loadLocale(g_settings.language);

		if (font.name == predefined_font[(!use_true_unicode_font)].name)
		{
			font = predefined_font[use_true_unicode_font];
			SetupFonts();
#warning TODO: reload LCD fonts, too
		}
		return true;
	}
	return false;
}

/**************************************************************************************
*                                                                                     *
*          Main programm - no function here                                           *
*                                                                                     *
**************************************************************************************/
int main(int argc, char **argv)
{
	setDebugLevel(DEBUG_NORMAL);

	tzset();
	initGlobals();
	return CNeutrinoApp::getInstance()->run(argc, argv);
}
