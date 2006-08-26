/*
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
#include <dirent.h>
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

#include "gui/widget/colorchooser.h"
#include "gui/widget/menue.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/hintbox.h"
#include "gui/widget/icons.h"
#include "gui/widget/lcdcontroler.h"
#include "gui/widget/rgbcsynccontroler.h"
#include "gui/widget/keychooser.h"
#include "gui/widget/stringinput.h"
#include "gui/widget/stringinput_ext.h"
#include "gui/widget/mountchooser.h"

#include "gui/color.h"

#include "gui/bedit/bouqueteditor_bouquets.h"
#include "gui/audio_select.h"
#include "gui/bouquetlist.h"
#include "gui/eventlist.h"
#include "gui/channellist.h"
#include "gui/screensetup.h"
#include "gui/pluginlist.h"
#include "gui/plugins.h"
#include "gui/infoviewer.h"
#include "gui/epgview.h"
#include "gui/epg_menu.h"
#include "gui/update.h"
#include "gui/scan.h"
#include "gui/favorites.h"
#include "gui/sleeptimer.h"
#include "gui/rc_lock.h"
#include "gui/timerlist.h"
#include "gui/alphasetup.h"
#include "gui/audioplayer.h"
#include "gui/imageinfo.h"

#if HAVE_DVB_API_VERSION >= 3
#include "gui/movieplayer.h"
#endif

#include "gui/nfs.h"
#include "gui/pictureviewer.h"
#include "gui/motorcontrol.h"


#include <system/setting_helpers.h>
#include <system/settings.h>
#include <system/debug.h>
#include <system/flashtool.h>
#include <system/fsmounter.h>

#include <timerdclient/timerdmsg.h>

#include <string.h>

// external menus
#include "gui/experimental_menu.h"

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_init();
#endif


CBouquetList    	* bouquetList;
CPlugins        	* g_PluginList;
CRemoteControl  	* g_RemoteControl;
CAPIDChangeExec		* APIDChanger;
CAudioSetupNotifier	* audioSetupNotifier;

// Globale Variablen - to use import global.h

// I don't like globals, I would have hidden them in classes,
// but if you wanna do it so... ;)
static bool parentallocked = false;
static bool waitforshutdown = false;

static char **global_argv;

static CTimingSettingsNotifier timingsettingsnotifier;
static CFontSizeNotifier fontsizenotifier;

extern const char * locale_real_names[]; /* #include <system/locals_intern.h> */

CZapitClient::SatelliteList satList;

CVCRControl::CDevice * recordingdevice = NULL;

#define NEUTRINO_SETTINGS_FILE          CONFIGDIR "/neutrino.conf"
#define NEUTRINO_RECORDING_TIMER_SCRIPT CONFIGDIR "/recording.timer"
#define NEUTRINO_RECORDING_START_SCRIPT CONFIGDIR "/recording.start"
#define NEUTRINO_RECORDING_ENDED_SCRIPT CONFIGDIR "/recording.end"
#define NEUTRINO_ENTER_STANDBY_SCRIPT   CONFIGDIR "/standby.on"
#define NEUTRINO_LEAVE_STANDBY_SCRIPT   CONFIGDIR "/standby.off"
#define NEUTRINO_SCAN_SETTINGS_FILE     CONFIGDIR "/scan.conf"
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
	nextRecordingInfo = NULL;
	skipShutdownTimer=false;
}

/*-------------------------------------------------------------------------------------
-                                                                                     -
-           CNeutrinoApp - Destructor                                                 -
-                                                                                     -
-------------------------------------------------------------------------------------*/
CNeutrinoApp::~CNeutrinoApp()
{
	if (channelList)
		delete channelList;
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

typedef struct font_sizes
{
	const neutrino_locale_t name;
	const unsigned int      defaultsize;
	const unsigned int      style;
	const unsigned int      size_offset;
} font_sizes_struct;

#define FONT_STYLE_REGULAR 0
#define FONT_STYLE_BOLD    1
#define FONT_STYLE_ITALIC  2

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
	strcpy(g_settings.epg_cache , configfile.getString( "epg_cache_time", "14" ).c_str() );
	strcpy(g_settings.epg_old_events ,configfile.getString("epg_old_events", "1" ).c_str() );
	strcpy(g_settings.epg_max_events ,configfile.getString("epg_max_events", "6000" ).c_str() );

	//misc
	g_settings.shutdown_real            = configfile.getBool("shutdown_real"             , true );
	g_settings.shutdown_real_rcdelay    = configfile.getBool("shutdown_real_rcdelay"     , true );
	strcpy(g_settings.shutdown_count, configfile.getString("shutdown_count","0").c_str());
	g_settings.infobar_sat_display      = configfile.getBool("infobar_sat_display"       , true );
	g_settings.infobar_subchan_disp_pos = configfile.getInt32("infobar_subchan_disp_pos" , 0 );
	g_settings.misc_spts                = configfile.getBool("misc_spts"                 , false );
#ifndef TUXTXT_CFG_STANDALONE
	g_settings.tuxtxt_cache                = configfile.getBool("tuxtxt_cache"                 , false );
#endif
	g_settings.virtual_zap_mode	    = configfile.getBool("virtual_zap_mode"          , false);

	//audio
	g_settings.audio_AnalogMode = configfile.getInt32( "audio_AnalogMode", 0 );
	g_settings.audio_DolbyDigital    = configfile.getBool("audio_DolbyDigital"   , false);
	g_settings.audio_avs_Control = configfile.getInt32( "audio_avs_Control", CControld::TYPE_AVS );
	strcpy( g_settings.audio_PCMOffset, configfile.getString( "audio_PCMOffset", "0" ).c_str() );


	//vcr
	g_settings.vcr_AutoSwitch        = configfile.getBool("vcr_AutoSwitch"       , true );

	//language
	strcpy(g_settings.language, configfile.getString("language", "").c_str());

	//epg
	strcpy(g_settings.epg_dir, configfile.getString("epg_dir", "").c_str());

	//widget settings
	g_settings.widget_fade           = configfile.getBool("widget_fade"          , true );

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
		sprintf(cfg_key,"network_nfs_username_%d", i);
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
	strcpy( g_settings.network_nfs_recordingdir, configfile.getString( "network_nfs_recordingdir", "" ).c_str() );
	g_settings.filesystem_is_utf8              = configfile.getBool("filesystem_is_utf8"                 , true );

        // NTP-Server for sectionsd
	strcpy(g_settings.network_ntpserver, configfile.getString("network_ntpserver", "de.pool.ntp.org" ).c_str() );
	strcpy(g_settings.network_ntprefresh, configfile.getString("network_ntprefresh", "30" ).c_str() );
	g_settings.network_ntpenable = configfile.getBool("network_ntpenable", false);

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
	g_settings.recording_stream_pmt_pid        = configfile.getBool("recordingmenu.stream_pmt_pid"      , false);
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

	g_settings.key_quickzap_up = configfile.getInt32( "key_quickzap_up",  CRCInput::RC_up );
	g_settings.key_quickzap_down = configfile.getInt32( "key_quickzap_down",  CRCInput::RC_down );
	g_settings.key_bouquet_up = configfile.getInt32( "key_bouquet_up",  CRCInput::RC_right );
	g_settings.key_bouquet_down = configfile.getInt32( "key_bouquet_down",  CRCInput::RC_left );
	g_settings.key_subchannel_up = configfile.getInt32( "key_subchannel_up",  CRCInput::RC_right );
	g_settings.key_subchannel_down = configfile.getInt32( "key_subchannel_down",  CRCInput::RC_left );
	g_settings.key_zaphistory = configfile.getInt32( "key_zaphistory",  CRCInput::RC_home );
	g_settings.key_lastchannel = configfile.getInt32( "key_lastchannel",  CRCInput::RC_0 );

	strcpy(g_settings.repeat_blocker, configfile.getString("repeat_blocker", g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS ? "150" : "25").c_str());
	strcpy(g_settings.repeat_genericblocker, configfile.getString("repeat_genericblocker", g_info.box_Type == CControld::TUXBOX_MAKER_PHILIPS ? "25" : "0").c_str());
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
	configfile.setString("epg_cache_time"           ,g_settings.epg_cache );
	configfile.setString("epg_old_events"           ,g_settings.epg_old_events );
	configfile.setString("epg_max_events"           ,g_settings.epg_max_events );
	configfile.setString("epg_dir"                  ,g_settings.epg_dir);

	//misc
	configfile.setBool("shutdown_real"             , g_settings.shutdown_real);
	configfile.setBool("shutdown_real_rcdelay"     , g_settings.shutdown_real_rcdelay);
	configfile.setString("shutdown_count"           , g_settings.shutdown_count);
	configfile.setBool("infobar_sat_display"       , g_settings.infobar_sat_display);
	configfile.setInt32("infobar_subchan_disp_pos" , g_settings.infobar_subchan_disp_pos);
	configfile.setBool("misc_spts"                 , g_settings.misc_spts);
#ifndef TUXTXT_CFG_STANDALONE
	configfile.setBool("tuxtxt_cache"                 , g_settings.tuxtxt_cache);
#endif
	configfile.setBool("virtual_zap_mode"          , g_settings.virtual_zap_mode);

	//audio
	configfile.setInt32( "audio_AnalogMode", g_settings.audio_AnalogMode );
	configfile.setBool("audio_DolbyDigital"   , g_settings.audio_DolbyDigital   );
	configfile.setInt32( "audio_avs_Control", g_settings.audio_avs_Control );
	configfile.setString( "audio_PCMOffset", g_settings.audio_PCMOffset );

	//vcr
	configfile.setBool("vcr_AutoSwitch"       , g_settings.vcr_AutoSwitch       );

	//language
	configfile.setString("language", g_settings.language);

	//widget settings
	configfile.setBool("widget_fade"          , g_settings.widget_fade          );

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
	configfile.setString( "network_nfs_recordingdir", g_settings.network_nfs_recordingdir);
	configfile.setBool  ("filesystem_is_utf8"                 , g_settings.filesystem_is_utf8             );

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
	configfile.setBool  ("recordingmenu.stream_pmt_pid"       , g_settings.recording_stream_pmt_pid      );
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

	//streaming
	configfile.setInt32 ( "streaming_type", g_settings.streaming_type );
	configfile.setString( "streaming_server_ip", g_settings.streaming_server_ip );
	configfile.setString( "streaming_server_port", g_settings.streaming_server_port );
	configfile.setString( "streaming_server_cddrive", g_settings.streaming_server_cddrive );
	configfile.setString ( "streaming_videorate", g_settings.streaming_videorate );
	configfile.setString ( "streaming_audiorate", g_settings.streaming_audiorate );
	configfile.setString( "streaming_server_startdir", g_settings.streaming_server_startdir );
	configfile.setInt32 ( "streaming_transcode_audio", g_settings.streaming_transcode_audio );
	configfile.setInt32 ( "streaming_force_avi_rawaudio", g_settings.streaming_force_avi_rawaudio );
	configfile.setInt32 ( "streaming_force_transcode_video", g_settings.streaming_force_transcode_video );
	configfile.setInt32 ( "streaming_transcode_video_codec", g_settings.streaming_transcode_video_codec );
	configfile.setInt32 ( "streaming_resolution", g_settings.streaming_resolution );
	configfile.setInt32("streaming_use_buffer" , g_settings.streaming_use_buffer);
	configfile.setInt32("streaming_buffer_segment_size" , g_settings.streaming_buffer_segment_size);

	// default plugin for movieplayer
	configfile.setString ( "movieplayer_plugin", g_settings.movieplayer_plugin );

	//rc-key configuration
	configfile.setInt32 ( "key_tvradio_mode", g_settings.key_tvradio_mode );

	configfile.setInt32( "key_channelList_pageup", g_settings.key_channelList_pageup );
	configfile.setInt32( "key_channelList_pagedown", g_settings.key_channelList_pagedown );
	configfile.setInt32( "key_channelList_cancel", g_settings.key_channelList_cancel );
	configfile.setInt32( "key_channelList_sort", g_settings.key_channelList_sort );
	configfile.setInt32( "key_channelList_addrecord", g_settings.key_channelList_addrecord );
	configfile.setInt32( "key_channelList_addremind", g_settings.key_channelList_addremind );

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


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit, get the Channellist from daemon              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit()
{
	dprintf(DEBUG_DEBUG, "doing channelsInit\n");
	//deleting old channelList for mode-switching.

	if (channelList)
		delete channelList;

	channelList = new CChannelList(g_Locale->getText(LOCALE_CHANNELLIST_HEAD));
	CZapitClient::BouquetChannelList zapitChannels;
	g_Zapit->getChannels(zapitChannels, CZapitClient::MODE_CURRENT, CZapitClient::SORT_BOUQUET, true); // UTF-8
	for(uint i=0; i<zapitChannels.size(); i++)
	{
		channelList->addChannel(zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].satellitePosition, zapitChannels[i].channel_id); // UTF-8
	}

	delete bouquetList;
	bouquetList = new CBouquetList();
	bouquetList->orgChannelList = channelList;
	CZapitClient::BouquetList zapitBouquets;

	/* load non-empty bouquets only */
	g_Zapit->getBouquets(zapitBouquets, false, true); // UTF-8
	for(uint i = 0; i < zapitBouquets.size(); i++)
	{
		CZapitClient::BouquetChannelList zapitChannels;

		/* add terminating 0 to zapitBouquets[i].name */
		char bouquetname[sizeof(zapitBouquets[i].name) + 1];
		strncpy(bouquetname, zapitBouquets[i].name, sizeof(zapitBouquets[i].name));
		bouquetname[sizeof(zapitBouquets[i].name)] = 0;

		bouquetList->addBouquet(bouquetname, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);


		g_Zapit->getBouquetChannels(zapitBouquets[i].bouquet_nr, zapitChannels, CZapitClient::MODE_CURRENT, true); // UTF-8

		for (uint j = 0; j < zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = channelList->getChannel(zapitChannels[j].nr);

			/* observe that "bouquetList->Bouquets[i]" refers to the bouquet we just created using bouquetList->addBouquet */
			bouquetList->Bouquets[i]->channelList->addChannel(channel);

			if (zapitBouquets[i].locked)
			{
				channel->bAlwaysLocked = true;
			}
		}
	}
	dprintf(DEBUG_DEBUG, "\nAll bouquets-channels received\n");
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
		else if ((!strcmp(argv[x], "-v")) || (!strcmp(argv[x], "--verbose")))
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
	mainMenu.addItem(GenericMenuSeparator);

	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_TVMODE, true, NULL, this, "tv", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED), true);
	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_RADIOMODE, true, NULL, this, "radio", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SCARTMODE, true, NULL, this, "scart", CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW));
	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_GAMES, true, NULL, new CPluginList(LOCALE_MAINMENU_GAMES,CPlugins::P_TYPE_GAME), "", CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	mainMenu.addItem(GenericMenuSeparatorLine);
	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_AUDIOPLAYER, true, NULL, new CAudioPlayerGui(), NULL, CRCInput::RC_1));

	#if HAVE_DVB_API_VERSION >= 3
	//mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_MOVIEPLAYER, true, NULL, new CMoviePlayerGui()));
	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_MOVIEPLAYER, true, NULL, &moviePlayer, NULL, CRCInput::RC_2));

	moviePlayer.addItem(GenericMenuSeparator);
	moviePlayer.addItem(GenericMenuBack);
	moviePlayer.addItem(GenericMenuSeparatorLine);
//	CMoviePlayerGui* moviePlayerGui = new CMoviePlayerGui();
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
#endif

	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_PICTUREVIEWER, true, NULL, new CPictureViewerGui(), NULL, CRCInput::RC_3));
	int shortcut = 4;
	if (g_PluginList->hasPlugin(CPlugins::P_TYPE_SCRIPT))
		mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SCRIPTS, true, NULL, new CPluginList(LOCALE_MAINMENU_SCRIPTS,CPlugins::P_TYPE_SCRIPT), "",
											CRCInput::convertDigitToKey(shortcut++)));
	mainMenu.addItem(GenericMenuSeparatorLine);

	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SETTINGS, true, NULL, &mainSettings, NULL,
										CRCInput::convertDigitToKey(shortcut++)));
	mainMenu.addItem(new CLockedMenuForwarder(LOCALE_MAINMENU_SERVICE, g_settings.parentallock_pincode, false, true, NULL, &service, NULL,
											  CRCInput::convertDigitToKey(shortcut++)));
	mainMenu.addItem(GenericMenuSeparatorLine);

	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SLEEPTIMER, true, NULL, new CSleepTimerWidget, NULL,
										CRCInput::convertDigitToKey(shortcut++)));
	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_REBOOT, true, NULL, this, "reboot",
										CRCInput::convertDigitToKey(shortcut++)));
	mainMenu.addItem(new CMenuForwarder(LOCALE_MAINMENU_SHUTDOWN, true, NULL, this, "shutdown", CRCInput::RC_standby, "power.raw"));

	mainSettings.addItem(GenericMenuSeparator);
	mainSettings.addItem(GenericMenuBack);
	mainSettings.addItem(GenericMenuSeparatorLine);
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_SAVESETTINGSNOW, true, NULL, this, "savesettings", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	mainSettings.addItem(GenericMenuSeparatorLine);
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_VIDEO     , true, NULL, &videoSettings    , NULL, CRCInput::RC_1));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_AUDIO     , true, NULL, &audioSettings    , NULL, CRCInput::RC_2));
	if(g_settings.parentallock_prompt)
		mainSettings.addItem(new CLockedMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, g_settings.parentallock_pincode, true, true, NULL, &parentallockSettings, NULL, CRCInput::RC_3));
	else
		mainSettings.addItem(new CMenuForwarder(LOCALE_PARENTALLOCK_PARENTALLOCK, true, NULL, &parentallockSettings, NULL, CRCInput::RC_3));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_NETWORK   , true, NULL, &networkSettings  , NULL, CRCInput::RC_4));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_RECORDING , true, NULL, &recordingSettings, NULL, CRCInput::RC_5));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_STREAMING , true, NULL, &streamingSettings, NULL, CRCInput::RC_6));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_LANGUAGE  , true, NULL, &languageSettings , NULL, CRCInput::RC_7));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_COLORS    , true, NULL, &colorSettings    , NULL, CRCInput::RC_8));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_LCD       , true, NULL, &lcdSettings      , NULL, CRCInput::RC_9));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_KEYBINDING, true, NULL, &keySettings      , NULL, CRCInput::RC_0));
	mainSettings.addItem(new CMenuForwarder(LOCALE_AUDIOPLAYERPICSETTINGS_GENERAL , true, NULL, &audiopl_picSettings   , NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_DRIVER , true, NULL, &driverSettings   , NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	mainSettings.addItem(new CMenuForwarder(LOCALE_MAINSETTINGS_MISC      , true, NULL, &miscSettings     , NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW ));
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

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[OPTIONS_OFF0_ON1_OPTION_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  }
};

#define SECTIONSD_SCAN_OPTIONS_COUNT 3
const CMenuOptionChooser::keyval SECTIONSD_SCAN_OPTIONS[SECTIONSD_SCAN_OPTIONS_COUNT] =
{
	{ 0, LOCALE_OPTIONS_OFF },
	{ 1, LOCALE_OPTIONS_ON  },
	{ 2, LOCALE_OPTIONS_ON_WITHOUT_MESSAGES  }
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
		//settings.addItem(GenericMenuSeparatorLine);
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
	if(scanSettings.TP_fec == 0)
		scanSettings.TP_fec = 1;

	settings.addItem(GenericMenuSeparatorLine);

	CStringInput* freq = new CStringInput(LOCALE_SCANTP_FREQ, (char *) scanSettings.TP_freq, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	CStringInput* rate = new CStringInput(LOCALE_SCANTP_RATE, (char *) scanSettings.TP_rate, 8, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	CMenuOptionChooser* fec = new CMenuOptionChooser(LOCALE_SCANTP_FEC, (int *)&scanSettings.TP_fec, SATSETUP_SCANTP_FEC, SATSETUP_SCANTP_FEC_COUNT, scanSettings.TP_scan);
	CMenuOptionChooser* pol = new CMenuOptionChooser(LOCALE_SCANTP_POL, (int *)&scanSettings.TP_pol, SATSETUP_SCANTP_POL, SATSETUP_SCANTP_POL_COUNT, scanSettings.TP_scan);
	CMenuOptionChooser* onoffscanSectionsd = ( new CMenuOptionChooser(LOCALE_SECTIONSD_SCANMODE, (int *)&scanSettings.scanSectionsd, SECTIONSD_SCAN_OPTIONS, SECTIONSD_SCAN_OPTIONS_COUNT, true,new CScanModeSectionsdNotifier));
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
	service.addItem(GenericMenuSeparator);
	service.addItem(GenericMenuBack);
	service.addItem(GenericMenuSeparatorLine);
	service.addItem(new CMenuForwarder(LOCALE_BOUQUETEDITOR_NAME    , true, NULL, new CBEBouquetWidget(), NULL            , CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_SCANTS    , true, NULL, &scanSettings         , NULL            , CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	service.addItem(GenericMenuSeparatorLine);
	service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_RELOAD    , true, NULL, this                  , "reloadchannels", CRCInput::RC_1));
	service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_GETPLUGINS, true, NULL, this                  , "reloadplugins" , CRCInput::RC_2));
	service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_RESTART   , true, NULL, this                  , "restart"       , CRCInput::RC_3));
	service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_UCODECHECK, true, NULL, UCodeChecker          , NULL            , CRCInput::RC_4));
	service.addItem(GenericMenuSeparatorLine);
	service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_IMAGEINFO,  true, NULL, new CImageInfo()     , NULL, CRCInput::RC_yellow  , NEUTRINO_ICON_BUTTON_YELLOW  ), false);

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

		updateSettings->addItem(GenericMenuSeparatorLine);
		CMenuOptionChooser *oj = new CMenuOptionChooser(LOCALE_FLASHUPDATE_UPDATEMODE, &g_settings.softupdate_mode, FLASHUPDATE_UPDATEMODE_OPTIONS, FLASHUPDATE_UPDATEMODE_OPTION_COUNT, true);
		updateSettings->addItem( oj );


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

		updateSettings->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_FLASHUPDATE_PROXYSERVER_SEP));

		CStringInputSMS * updateSettings_proxy = new CStringInputSMS(LOCALE_FLASHUPDATE_PROXYSERVER, g_settings.softupdate_proxyserver, 23, LOCALE_FLASHUPDATE_PROXYSERVER_HINT1, LOCALE_FLASHUPDATE_PROXYSERVER_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYSERVER, true, g_settings.softupdate_proxyserver, updateSettings_proxy));

		CStringInputSMS * updateSettings_proxyuser = new CStringInputSMS(LOCALE_FLASHUPDATE_PROXYUSERNAME, g_settings.softupdate_proxyusername, 23, LOCALE_FLASHUPDATE_PROXYUSERNAME_HINT1, LOCALE_FLASHUPDATE_PROXYUSERNAME_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYUSERNAME, true, g_settings.softupdate_proxyusername, updateSettings_proxyuser));

		CStringInputSMS * updateSettings_proxypass = new CStringInputSMS(LOCALE_FLASHUPDATE_PROXYPASSWORD, g_settings.softupdate_proxypassword, 20, LOCALE_FLASHUPDATE_PROXYPASSWORD_HINT1, LOCALE_FLASHUPDATE_PROXYPASSWORD_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_PROXYPASSWORD, true, g_settings.softupdate_proxypassword, updateSettings_proxypass));

		updateSettings->addItem(GenericMenuSeparatorLine);
		updateSettings->addItem(new CMenuForwarder(LOCALE_FLASHUPDATE_CHECKUPDATE, true, NULL, new CFlashUpdate(), NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));

		service.addItem(new CMenuForwarder(LOCALE_SERVICEMENU_UPDATE, true, NULL, updateSettings, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE));
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

	miscSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_MISCSETTINGS_EPG_HEAD));
	CStringInput * miscSettings_epg_cache = new CStringInput(LOCALE_MISCSETTINGS_EPG_CACHE, g_settings.epg_cache, 2,LOCALE_MISCSETTINGS_EPG_CACHE_HINT1, LOCALE_MISCSETTINGS_EPG_CACHE_HINT2 , "0123456789 ");
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_CACHE, true, g_settings.epg_cache, miscSettings_epg_cache));
	CStringInput * miscSettings_epg_old_events = new CStringInput(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, g_settings.epg_old_events, 2,LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_OLD_EVENTS_HINT2 , "0123456789 ");
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_OLD_EVENTS, true, g_settings.epg_old_events, miscSettings_epg_old_events));
	CStringInput * miscSettings_epg_max_events = new CStringInput(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, g_settings.epg_max_events, 5,LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT1, LOCALE_MISCSETTINGS_EPG_MAX_EVENTS_HINT2 , "0123456789 ");
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_MAX_EVENTS, true, g_settings.epg_max_events, miscSettings_epg_max_events));
	miscSettings.addItem(new CMenuForwarder(LOCALE_MISCSETTINGS_EPG_DIR, true, g_settings.epg_dir,this,"epgdir"));


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
#if HAVE_DVB_API_VERSION == 1
	{LOCALE_DRIVERSETTINGS_STARTBHDRIVER , "/var/etc/.bh"            , OPTIONS_OFF0_ON1_OPTIONS },
#endif
	{LOCALE_DRIVERSETTINGS_HWSECTIONS    , "/var/etc/.hw_sections"   , OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_NOAVIAWATCHDOG, "/var/etc/.no_watchdog"   , OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_NOENXWATCHDOG , "/var/etc/.no_enxwatchdog", OPTIONS_OFF1_ON0_OPTIONS },
	{LOCALE_DRIVERSETTINGS_PMTUPDATE     , "/var/etc/.pmt_update"    , OPTIONS_OFF0_ON1_OPTIONS }
};

void CNeutrinoApp::InitDriverSettings(CMenuWidget &driverSettings)
{
	dprintf(DEBUG_DEBUG, "init driversettings\n");
	driverSettings.addItem(GenericMenuSeparator);
	driverSettings.addItem(GenericMenuBack);
	driverSettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_DRIVERSETTINGS_DRIVER_BOOT));

	CSPTSNotifier *sptsNotifier = new CSPTSNotifier;
	driverSettings.addItem(new CMenuOptionChooser(LOCALE_DRIVERSETTINGS_SPTSMODE, &g_settings.misc_spts, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, sptsNotifier));

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

	driverSettings.addItem(new CMenuOptionChooser(LOCALE_DRIVERSETTINGS_FB_DESTINATION, &g_settings.uboot_console, DRIVERSETTINGS_FB_DESTINATION_OPTIONS, DRIVERSETTINGS_FB_DESTINATION_OPTION_COUNT, true, ConsoleDestinationChanger));
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

#define AUDIOMENU_AVS_CONTROL_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_AVS_CONTROL_OPTIONS[AUDIOMENU_AVS_CONTROL_OPTION_COUNT] =
{
	{ CControld::TYPE_OST , LOCALE_AUDIOMENU_OST  },
	{ CControld::TYPE_AVS , LOCALE_AUDIOMENU_AVS  },
	{ CControld::TYPE_LIRC, LOCALE_AUDIOMENU_LIRC }
};

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

	audioSettings.addItem(GenericMenuSeparatorLine);

	CStringInput * audio_PCMOffset = new CStringInput(LOCALE_AUDIOMENU_PCMOFFSET, g_settings.audio_PCMOffset, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ", audioSetupNotifier);
	CMenuForwarder *mf = new CMenuForwarder(LOCALE_AUDIOMENU_PCMOFFSET, (g_settings.audio_avs_Control == CControld::TYPE_LIRC), g_settings.audio_PCMOffset, audio_PCMOffset );
	CAudioSetupNotifier2 *audioSetupNotifier2 = new CAudioSetupNotifier2(mf);

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AVS_CONTROL, &g_settings.audio_avs_Control, AUDIOMENU_AVS_CONTROL_OPTIONS, AUDIOMENU_AVS_CONTROL_OPTION_COUNT, true, audioSetupNotifier2);
	audioSettings.addItem(oj);
	audioSettings.addItem(mf);
}


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
	CStringInputSMS * networkSettings_NtpServer = new CStringInputSMS(LOCALE_NETWORKMENU_NTPSERVER, g_settings.network_ntpserver, 30, LOCALE_NETWORKMENU_NTPSERVER_HINT1, LOCALE_NETWORKMENU_NTPSERVER_HINT2, "abcdefghijklmnopqrstuvwxyz0123456789-. ");
	CStringInput * networkSettings_NtpRefresh = new CStringInput(LOCALE_NETWORKMENU_NTPREFRESH, g_settings.network_ntprefresh, 3,LOCALE_NETWORKMENU_NTPREFRESH_HINT1, LOCALE_NETWORKMENU_NTPREFRESH_HINT2 , "0123456789 ");
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
	networkSettings.addItem(new CMenuOptionChooser(LOCALE_NETWORKMENU_NTPENABLE, &g_settings.network_ntpenable, OPTIONS_NTPENABLE_OPTIONS, OPTIONS_NTPENABLE_OPTION_COUNT , true));
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

	// for direct recording
	CMenuWidget *directRecordingSettings = new CMenuWidget(LOCALE_RECORDINGMENU_FILESETTINGS, NEUTRINO_ICON_RECORDING);

	CMenuForwarder* mf7 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILESETTINGS,(g_settings.recording_type == RECORDING_FILE),NULL,directRecordingSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);

	CMenuForwarder* mf8 = new CMenuForwarder(LOCALE_RECORDINGMENU_DEFDIR, true, g_settings.network_nfs_recordingdir,this,"recordingdir");
	CStringInput * recordingSettings_splitsize = new CStringInput(LOCALE_RECORDINGMENU_SPLITSIZE, g_settings.recording_splitsize, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
	CMenuForwarder* mf9 = new CMenuForwarder(LOCALE_RECORDINGMENU_SPLITSIZE, true, g_settings.recording_splitsize,recordingSettings_splitsize);

	CMenuOptionChooser* oj6 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_O_SYNC, &g_settings.recording_use_o_sync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj7 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_FDATASYNC, &g_settings.recording_use_fdatasync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj8 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_VTXT_PID, &g_settings.recording_stream_vtxt_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj9 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_PMT_PID, &g_settings.recording_stream_pmt_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

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
	directRecordingSettings->addItem(mf8);
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
	CMenuForwarder* mf9 = new CMenuForwarder(LOCALE_STREAMINGMENU_STREAMING_BUFFER_SEGMENT_SIZE , true, streamingSettings_buffer_size->getValue()      , streamingSettings_buffer_size);
	COnOffNotifier *bufferNotifier = new COnOffNotifier(mf9);
	CMenuOptionChooser* oj6 = new CMenuOptionChooser(LOCALE_STREAMINGMENU_STREAMING_USE_BUFFER , &g_settings.streaming_use_buffer  , MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true,bufferNotifier);

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
	streamingSettings.addItem( oj6 );
	streamingSettings.addItem( mf9 );
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
	fontSettings.addItem(new CMenuNumberInput(neutrino_font[number_of_fontsize_entry].name, neutrino_font[number_of_fontsize_entry].defaultsize, &fontsizenotifier, &configfile));
}


typedef struct font_sizes_groups
{
	const neutrino_locale_t                     groupname;
	const unsigned int                          count;
	const SNeutrinoSettings::FONT_TYPES * const content;
	const char * const                          actionkey;
} font_sizes_groups_struct;

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

const font_sizes_groups font_sizes_groups[6] =
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

void CNeutrinoApp::InitColorSettings(CMenuWidget &colorSettings, CMenuWidget &fontSettings )
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

	colorSettings.addItem(GenericMenuSeparatorLine);
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
}


void CNeutrinoApp::InitColorThemesSettings(CMenuWidget &colorSettings_Themes)
{
	colorSettings_Themes.addItem(GenericMenuSeparator);
	colorSettings_Themes.addItem(GenericMenuBack);
	colorSettings_Themes.addItem(GenericMenuSeparatorLine);
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_NEUTRINO_THEME, true, NULL, this, "theme_neutrino"));
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_CLASSIC_THEME, true, NULL, this, "theme_classic"));
	colorSettings_Themes.addItem(new CMenuForwarder(LOCALE_COLORTHEMEMENU_DBLUE_THEME, true, NULL, this, "theme_dblue"));
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

#define LCDMENU_STATUSLINE_OPTION_COUNT 3
const CMenuOptionChooser::keyval LCDMENU_STATUSLINE_OPTIONS[LCDMENU_STATUSLINE_OPTION_COUNT] =
{
	{ 0, LOCALE_LCDMENU_STATUSLINE_PLAYTIME },
	{ 1, LOCALE_LCDMENU_STATUSLINE_VOLUME   },
	{ 2, LOCALE_LCDMENU_STATUSLINE_BOTH     }
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
	KEY_CHANNEL_UP,
	KEY_CHANNEL_DOWN,
	KEY_BOUQUET_UP,
	KEY_BOUQUET_DOWN,
	KEY_SUBCHANNEL_UP,
	KEY_SUBCHANNEL_DOWN,
	KEY_ZAP_HISTORY,
	KEY_LASTCHANNEL
};

const neutrino_locale_t keydescription_head[15] =
{
	LOCALE_KEYBINDINGMENU_TVRADIOMODE_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEUP_HEAD,
	LOCALE_KEYBINDINGMENU_PAGEDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_CANCEL_HEAD,
	LOCALE_KEYBINDINGMENU_SORT_HEAD,
	LOCALE_KEYBINDINGMENU_ADDRECORD_HEAD,
	LOCALE_KEYBINDINGMENU_ADDREMIND_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_CHANNELDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETUP_HEAD,
	LOCALE_KEYBINDINGMENU_BOUQUETDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELUP_HEAD,
	LOCALE_KEYBINDINGMENU_SUBCHANNELDOWN_HEAD,
	LOCALE_KEYBINDINGMENU_ZAPHISTORY_HEAD,
	LOCALE_KEYBINDINGMENU_LASTCHANNEL_HEAD
};

const neutrino_locale_t keydescription[15] =
{
	LOCALE_KEYBINDINGMENU_TVRADIOMODE,
	LOCALE_KEYBINDINGMENU_PAGEUP,
	LOCALE_KEYBINDINGMENU_PAGEDOWN,
	LOCALE_KEYBINDINGMENU_CANCEL,
	LOCALE_KEYBINDINGMENU_SORT,
	LOCALE_KEYBINDINGMENU_ADDRECORD,
	LOCALE_KEYBINDINGMENU_ADDREMIND,
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

	int * keyvalue_p[15] =
		{
			&g_settings.key_tvradio_mode,
			&g_settings.key_channelList_pageup,
			&g_settings.key_channelList_pagedown,
			&g_settings.key_channelList_cancel,
			&g_settings.key_channelList_sort,
			&g_settings.key_channelList_addrecord,
			&g_settings.key_channelList_addremind,
			&g_settings.key_quickzap_up,
			&g_settings.key_quickzap_down,
			&g_settings.key_bouquet_up,
			&g_settings.key_bouquet_down,
			&g_settings.key_subchannel_up,
			&g_settings.key_subchannel_down,
			&g_settings.key_zaphistory,
			&g_settings.key_lastchannel
		};

	CKeyChooser * keychooser[15];

	for (int i = 0; i < 15; i++)
		keychooser[i] = new CKeyChooser(keyvalue_p[i], keydescription_head[i], NEUTRINO_ICON_SETTINGS);

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_MODECHANGE));
	keySettings.addItem(new CMenuForwarder(keydescription[KEY_TV_RADIO_MODE], true, NULL, keychooser[KEY_TV_RADIO_MODE]));

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_CHANNELLIST));

	CMenuOptionChooser *oj = new CMenuOptionChooser(LOCALE_KEYBINDINGMENU_BOUQUETHANDLING, &g_settings.bouquetlist_mode, KEYBINDINGMENU_BOUQUETHANDLING_OPTIONS, KEYBINDINGMENU_BOUQUETHANDLING_OPTION_COUNT, true );
	keySettings.addItem(oj);

	for (int i = KEY_PAGE_UP; i <= KEY_ADD_REMIND; i++)
		keySettings.addItem(new CMenuForwarder(keydescription[i], true, NULL, keychooser[i]));

	keySettings.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_KEYBINDINGMENU_QUICKZAP));

	for (int i = KEY_CHANNEL_UP; i <= KEY_SUBCHANNEL_DOWN; i++)
		keySettings.addItem(new CMenuForwarder(keydescription[i], true, NULL, keychooser[i]));

	keySettings.addItem(new CMenuForwarder(keydescription[KEY_ZAP_HISTORY], true, NULL, keychooser[KEY_ZAP_HISTORY]));

	keySettings.addItem(new CMenuForwarder(keydescription[KEY_LASTCHANNEL], true, NULL, keychooser[KEY_LASTCHANNEL]));
}

void CNeutrinoApp::SelectNVOD()
{
	if (!(g_RemoteControl->subChannels.empty()))
	{
		// NVOD/SubService- Kanal!
		CMenuWidget NVODSelector(g_RemoteControl->are_subchannels ? LOCALE_NVODSELECTOR_SUBSERVICE : LOCALE_NVODSELECTOR_HEAD, "video.raw", 350);

		NVODSelector.addItem(GenericMenuSeparator);

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

				tmZeit= localtime(&e->startzeit );
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
				NVODSelector.addItem(new CMenuForwarderNonLocalized(nvod_s, true, NULL, NVODChanger, nvod_id), (count == g_RemoteControl->selected_subchannel));
			}
			else
			{
				NVODSelector.addItem(new CMenuForwarderNonLocalized((Latin1_to_UTF8(e->subservice_name)).c_str(), true, NULL, NVODChanger, nvod_id, CRCInput::convertDigitToKey(count)), (count == g_RemoteControl->selected_subchannel));
			}

			count++;
		}

		if( g_RemoteControl->are_subchannels )
		{
			NVODSelector.addItem(GenericMenuSeparatorLine);
			CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_NVODSELECTOR_DIRECTORMODE, &g_RemoteControl->director_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);
			NVODSelector.addItem(oj);
		}

		NVODSelector.exec(NULL, "");
	}
}



#define MAINMENU_RECORDING_OPTION_COUNT 2
const CMenuOptionChooser::keyval MAINMENU_RECORDING_OPTIONS[MAINMENU_RECORDING_OPTION_COUNT] =
{
	{ 0, LOCALE_MAINMENU_RECORDING_START },
	{ 1, LOCALE_MAINMENU_RECORDING_STOP  }
};

void CNeutrinoApp::ShowStreamFeatures()
{
	CMenuWidget StreamFeatureSelector(LOCALE_STREAMFEATURES_HEAD, "features.raw", 350);
	StreamFeatureSelector.addItem(GenericMenuSeparator);
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

			StreamFeatureSelector.addItem(new CMenuForwarderNonLocalized(g_PluginList->getName(count), true, NULL, StreamFeaturesChanger, id, (cnt== 0) ? CRCInput::RC_blue : CRCInput::convertDigitToKey(enabled_count-1), (cnt == 0) ? NEUTRINO_ICON_BUTTON_BLUE : ""), (cnt == 0));
			cnt++;
		}
	}

	if(cnt>0)
	{
		StreamFeatureSelector.addItem(GenericMenuSeparatorLine);
	}

	sprintf(id, "%d", -1);

	// -- Add Channel to favorites
//	StreamFeatureSelector.addItem(new CMenuForwarder(LOCALE_FAVORITES_MENUEADD, true, NULL, new CFavorites, id, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), false);
	StreamFeatureSelector.addItem(new CMenuForwarder(LOCALE_FAVORITES_MENUEADD, true, NULL, new CFavorites, id, CRCInput::convertDigitToKey(enabled_count), ""), false);

	StreamFeatureSelector.addItem(GenericMenuSeparatorLine);

	// start/stop recording
	if (g_settings.recording_type != RECORDING_OFF)
	{
		StreamFeatureSelector.addItem(new CMenuOptionChooser(LOCALE_MAINMENU_RECORDING, &recordingstatus, MAINMENU_RECORDING_OPTIONS, MAINMENU_RECORDING_OPTION_COUNT, true, this, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	}

	// -- Add TS Playback to blue button
	StreamFeatureSelector.addItem(new CMenuForwarder(LOCALE_MOVIEPLAYER_TSPLAYBACK, true, NULL, this->moviePlayerGui, "tsplayback", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN), false);

	// -- Timer-Liste
	StreamFeatureSelector.addItem(new CMenuForwarder(LOCALE_TIMERLIST_NAME, true, NULL, new CTimerList(), id, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW), false);

	StreamFeatureSelector.addItem(GenericMenuSeparatorLine);

	// --  Lock remote control
	StreamFeatureSelector.addItem(new CMenuForwarder(LOCALE_RCLOCK_MENUEADD, true, NULL, this->rcLock, id, CRCInput::RC_nokey, ""), false);

	// -- Sectionsd pause
	int dummy = g_Sectionsd->getIsScanningActive();
	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_MAINMENU_PAUSESECTIONSD, &dummy, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, new CPauseSectionsdNotifier );
	StreamFeatureSelector.addItem(oj);

#ifdef _EXPERIMENTAL_SETTINGS_
	//Experimental Settings
	StreamFeatureSelector.addItem(new CMenuForwarder(LOCALE_EXPERIMENTALSETTINGS, true, NULL, new CExperimentalSettingsMenuHandler(), id, CRCInput::RC_nokey, ""), false);
#endif

	StreamFeatureSelector.exec(NULL,"");

	// restore mute symbol
	AudioMute(current_muted, true);

}

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
				char *recDir = (preselectedDir != NULL) ? preselectedDir : g_settings.network_nfs_recordingdir;
				// preselectedDir != NULL -> called after stream problem so do not show a dialog again
				if(preselectedDir == NULL && g_settings.recording_choose_direct_rec_dir) {
					int userDecision = -1;

					CMountChooser recDirs(LOCALE_TIMERLIST_RECORDING_DIR,NEUTRINO_ICON_SETTINGS,&userDecision,NULL,g_settings.network_nfs_recordingdir);
					if (recDirs.hasItem()) {
						recDirs.exec(NULL,"");
						refreshGui = true;
						if (userDecision != -1)
						{
							recDir = g_settings.network_nfs_local_dir[userDecision];
							if (!CFSMounter::isMounted(g_settings.network_nfs_local_dir[userDecision]))
							{
								CFSMounter::MountRes mres =
									CFSMounter::mount(g_settings.network_nfs_ip[userDecision].c_str(),
											  g_settings.network_nfs_dir[userDecision],
											  g_settings.network_nfs_local_dir[userDecision],
											  (CFSMounter::FSType) g_settings.network_nfs_type[userDecision],
											  g_settings.network_nfs_username[userDecision],
											  g_settings.network_nfs_password[userDecision],
											  g_settings.network_nfs_mount_options1[userDecision],
											  g_settings.network_nfs_mount_options2[userDecision]);
								if (mres != CFSMounter::MRES_OK)
								{
									doRecord = false;
									const char * merr = mntRes2Str(mres);
									int msglen = strlen(merr) + strlen(g_settings.network_nfs_local_dir[userDecision]) + 7;
									char msg[msglen];
									strcpy(msg,merr);
									strcat(msg,"\nDir: ");
									strcat(msg,g_settings.network_nfs_local_dir[userDecision]);

									ShowMsgUTF(LOCALE_MESSAGEBOX_ERROR, msg,
										   CMessageBox::mbrBack, CMessageBox::mbBack,NEUTRINO_ICON_ERROR, 450, 10); // UTF-8
								}
							}
						} else
						{
							doRecord = false;
						}
					} else
					{
						printf("[neutrino.cpp] no network devices available\n");
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

#define LCD_UPDATE_TIME_RADIO_MODE (6 * 1000 * 1000)
#define LCD_UPDATE_TIME_TV_MODE (60 * 1000 * 1000)

void CNeutrinoApp::InitZapper()
{

	g_InfoViewer->start();
	g_EpgData->start();

	firstChannel();

	if (strcmp(g_settings.epg_dir, "") != 0)
		g_Sectionsd->readSIfromXML(g_settings.epg_dir);
	g_Sectionsd->setSectionsdScanMode(scanSettings.scanSectionsd);

#ifndef TUXTXT_CFG_STANDALONE
	if(g_settings.tuxtxt_cache)
	{
		tuxtxt_init();
	}
#endif

	// set initial PES/SPTS mode
	if (g_settings.misc_spts != g_Zapit->PlaybackState())
	{
		if (g_settings.misc_spts && firstchannel.mode == 't' )
			g_Zapit->PlaybackSPTS();
		else
			g_Zapit->PlaybackPES();
	}

	if(firstchannel.mode == 't')
	{
		tvMode();
	}
	else
	{
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

		recordingdevice = new CVCRControl::CFileDevice(g_settings.recording_stopplayback, g_settings.recording_stopsectionsd, g_settings.network_nfs_recordingdir, splitsize, g_settings.recording_use_o_sync, g_settings.recording_use_fdatasync, g_settings.recording_stream_vtxt_pid, g_settings.recording_stream_pmt_pid, ringbuffers,true);

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
		strcpy(g_settings.language, "deutsch");
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
	UCodeChecker              = new CUCodeCheckExec;
	NVODChanger               = new CNVODChangeExec;
	StreamFeaturesChanger     = new CStreamFeaturesChangeExec;
	MoviePluginChanger        = new CMoviePluginChangeExec;
	MyIPChanger               = new CIPChangeNotifier;
	ConsoleDestinationChanger = new CConsoleDestChangeNotifier;
	rcLock                    = new CRCLock();
	moviePlayerGui 						= new CMoviePlayerGui();

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

	if (!ucodes_available())
	{
		/* display error message */
		DisplayErrorMessage(g_Locale->getText(LOCALE_UCODES_FAILURE));

		/* show network settings dialog */
		networkSettings.exec(NULL, "");
	}

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
			else if( msg == CRCInput::RC_red )
			{	// eventlist
				// -- !! obsolete (2004-03-06 rasc)
				// g_EventList->exec(channelList->getActiveChannel_ChannelID(), channelList->getActiveChannelName()); // UTF-8

				// -- new EPG Menu (rasc 2004-03-06)
				CEPGMenuHandler  *epg_menu;
				epg_menu = new CEPGMenuHandler;
				epg_menu->doMenu();
				delete epg_menu;
			}
			else if( msg == CRCInput::RC_blue )
			{	// streaminfo
				ShowStreamFeatures();
			}
			else if( msg == CRCInput::RC_green )
			{
				// -- new Audio Selector Menu (rasc 2005-08-30)
				if (g_settings.audio_left_right_selectable ||
				    g_RemoteControl->current_PIDs.APIDs.size() > 1)
				{
					CAudioSelectMenuHandler  *audio_menu;
					audio_menu = new CAudioSelectMenuHandler;
					audio_menu-> doMenu();
					delete audio_menu;
				}
			}
			else if( msg == CRCInput::RC_yellow )
			{       // NVODs
				SelectNVOD();
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
		t_channel_id old_id=g_Zapit->getCurrentServiceID();
		if (data)
			old_parent_id = channelList->getActiveChannel_ChannelID();
		channelsInit();

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

		channelsInit();

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
				channelsInit();
			}
			else if (isTVMode && (mode != mode_tv))
			{
				tvMode(false);
				channelsInit();
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
			ShowHintUTF(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_ZAPTOTIMER_ANNOUNCE));
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
		if ((* (stream2file_status2_t *) data).status != STREAM2FILE_STATUS_IDLE)
		{
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

			if (strcmp(g_settings.epg_dir, "") != 0) {
				waitforshutdown = true;
				AudioMute(true);
				g_Sectionsd->writeSI2XML(g_settings.epg_dir);
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
   if((CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC) //lirc
   { // bei LIRC wissen wir nicht wikrlich ob jetzt ge oder entmuted wird, deswegen nix zeigen---
		if( !isEvent )
			g_Controld->Mute((CControld::volume_type)g_settings.audio_avs_Control);
   }
   else
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

      if( isEvent && ( mode != mode_scart ) && ( mode != mode_audio) && ( mode != mode_pic))
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

	fb_pixel_t * pixbuf = NULL;

	if(bDoPaint)
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
			}
			else if (msg == CRCInput::RC_minus)
			{
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
			}
			else
			{
				g_RCInput->postMsg(msg, data);
				break;
			}

			g_Controld->setVolume(current_volume, (CControld::volume_type)g_settings.audio_avs_Control);

			if((CControld::volume_type)g_settings.audio_avs_Control==CControld::TYPE_LIRC)
			{
				current_volume = 50;
			}
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

		if (bDoPaint)
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

	if( (bDoPaint) && (pixbuf!= NULL) )
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

		if(g_settings.misc_spts==1)
			g_Zapit->PlaybackSPTS();
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
		channelsInit();
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

		if(g_settings.misc_spts==1)
			g_Zapit->PlaybackPES();
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
		channelsInit();
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
				char *recDir = strlen(nextRecordingInfo->recordingDir) > 0 ?
					nextRecordingInfo->recordingDir : g_settings.network_nfs_recordingdir;
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
								const char * merr = mntRes2Str(mres);
								int msglen = strlen(merr) + strlen(nextRecordingInfo->recordingDir) + 7;
								char msg[msglen];
								strcpy(msg,merr);
								strcat(msg,"\nDir: ");
								strcat(msg,nextRecordingInfo->recordingDir);

								ShowHintUTF(LOCALE_MESSAGEBOX_ERROR, msg); // UTF-8
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
						recDir = g_settings.network_nfs_recordingdir;
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
		fontsizenotifier.changeNotify(NONEXISTANT_LOCALE, NULL);
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
	else if(actionKey == "recordingdir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.network_nfs_recordingdir))
			strncpy(g_settings.network_nfs_recordingdir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.network_nfs_recordingdir)-1);
		return menu_return::RETURN_REPAINT;
	}
	else if(actionKey == "epgdir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.epg_dir))
		{
			std::string newepgdir = b.getSelectedFile()->Name + "/";
			strncpy(g_settings.epg_dir, newepgdir.c_str(), sizeof(g_settings.epg_dir)-1);
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
