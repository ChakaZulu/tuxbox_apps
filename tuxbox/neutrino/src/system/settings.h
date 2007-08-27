/* 
  $Id: settings.h,v 1.182 2007/08/27 12:15:51 nitr8 Exp $
 
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

#ifndef __settings__
#define __settings__

#include <system/localize.h>
#include <configfile.h>
#include <zapit/client/zapitclient.h>

#include <string>

// Houdini: dirty Hack to update menu settings
#include "gui/widget/menue.h"

struct SNeutrinoSettings
{
	//video
	int video_Format;
	unsigned char video_csync;

	//misc
	int shutdown_real;
	int shutdown_real_rcdelay;
	char shutdown_count[4];
	char record_safety_time_before[3];
	char record_safety_time_after[3];
	int infobar_sat_display;
	int infobar_subchan_disp_pos;
	int misc_spts;
#ifndef TUXTXT_CFG_STANDALONE
	int tuxtxt_cache;
#endif
	int virtual_zap_mode;
	int infobar_show;

	// EPG
	std::string epg_cache;
	std::string epg_extendedcache;
	std::string epg_old_events;
	std::string epg_max_events;
	std::string epg_dir;
	std::string network_ntpserver;
	std::string network_ntprefresh;
	int network_ntpenable;

	//audio
	int audio_AnalogMode;
	int audio_DolbyDigital;
	int audio_avs_Control;
	char audio_PCMOffset[3];

	//vcr
	int vcr_AutoSwitch;

	//language
	char language[25];

	//timing
#define TIMING_SETTING_COUNT 8
	enum TIMING_SETTINGS {
		TIMING_MENU        = 0,
		TIMING_CHANLIST    = 1,
		TIMING_EPG         = 2,
		TIMING_INFOBAR     = 3,
		TIMING_INFOBAR_RADIO = 4,
		TIMING_FILEBROWSER = 5,
		TIMING_NUMERICZAP  = 6,
		TIMING_INFOBAR_MOVIE = 7
	};

	int  timing       [TIMING_SETTING_COUNT]   ;
	char timing_string[TIMING_SETTING_COUNT][4];

	//widget settings
	int widget_fade;
	int widget_osd;

	//colors
	unsigned char gtx_alpha1;
	unsigned char gtx_alpha2;

	unsigned char menu_Head_alpha;
	unsigned char menu_Head_red;
	unsigned char menu_Head_green;
	unsigned char menu_Head_blue;

	unsigned char menu_Head_Text_alpha;
	unsigned char menu_Head_Text_red;
	unsigned char menu_Head_Text_green;
	unsigned char menu_Head_Text_blue;

	unsigned char menu_Content_alpha;
	unsigned char menu_Content_red;
	unsigned char menu_Content_green;
	unsigned char menu_Content_blue;

	unsigned char menu_Content_Text_alpha;
	unsigned char menu_Content_Text_red;
	unsigned char menu_Content_Text_green;
	unsigned char menu_Content_Text_blue;

	unsigned char menu_Content_Selected_alpha;
	unsigned char menu_Content_Selected_red;
	unsigned char menu_Content_Selected_green;
	unsigned char menu_Content_Selected_blue;

	unsigned char menu_Content_Selected_Text_alpha;
	unsigned char menu_Content_Selected_Text_red;
	unsigned char menu_Content_Selected_Text_green;
	unsigned char menu_Content_Selected_Text_blue;

	unsigned char menu_Content_inactive_alpha;
	unsigned char menu_Content_inactive_red;
	unsigned char menu_Content_inactive_green;
	unsigned char menu_Content_inactive_blue;

	unsigned char menu_Content_inactive_Text_alpha;
	unsigned char menu_Content_inactive_Text_red;
	unsigned char menu_Content_inactive_Text_green;
	unsigned char menu_Content_inactive_Text_blue;

	unsigned char infobar_alpha;
	unsigned char infobar_red;
	unsigned char infobar_green;
	unsigned char infobar_blue;

	unsigned char infobar_Text_alpha;
	unsigned char infobar_Text_red;
	unsigned char infobar_Text_green;
	unsigned char infobar_Text_blue;

	//network
#define NETWORK_NFS_NR_OF_ENTRIES 8
	std::string network_nfs_ip[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_mac[NETWORK_NFS_NR_OF_ENTRIES][31];
	char network_nfs_local_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	char network_nfs_dir[NETWORK_NFS_NR_OF_ENTRIES][100];
	int  network_nfs_automount[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_mount_options1[NETWORK_NFS_NR_OF_ENTRIES][31];
	char network_nfs_mount_options2[NETWORK_NFS_NR_OF_ENTRIES][31];
	int  network_nfs_type[NETWORK_NFS_NR_OF_ENTRIES];
	char network_nfs_username[NETWORK_NFS_NR_OF_ENTRIES][31];
	char network_nfs_password[NETWORK_NFS_NR_OF_ENTRIES][31];
	char network_nfs_audioplayerdir[100];
	char network_nfs_picturedir[100];
	char network_nfs_moviedir[100];
	char network_nfs_recordingdir[100];

	//personalization
	int personalize_pinstatus;
	int personalize_bluebutton;
	int personalize_redbutton;
	char personalize_pincode[5];

	int personalize_tvmode;
	int personalize_radiomode;
	int personalize_scartmode;
	int personalize_games;
	int personalize_audioplayer;
	int personalize_movieplayer;
	int personalize_pictureviewer;
	int personalize_upnpbrowser;
	int personalize_settings;
	int personalize_service;
	int personalize_sleeptimer;
	int personalize_reboot;
	int personalize_shutdown;

	int personalize_bouqueteditor;
	int personalize_scants;
	int personalize_reload;
	int personalize_getplugins;
	int personalize_restart;
	int personalize_ucodecheck;
	int personalize_imageinfo;
	int personalize_update;

	int personalize_audio;
	int personalize_video;
	int personalize_youth;
	int personalize_network;
	int personalize_recording;
	int personalize_streaming;
	int personalize_keybinding;
	int personalize_language;
	int personalize_colors;
	int personalize_lcd;
	int personalize_audpic;
	int personalize_driver;
	int personalize_misc;

	//recording
	int  recording_type;
	int  recording_stopplayback;
	int  recording_stopsectionsd;
	std::string recording_server_ip;
	char recording_server_port[10];
	int  recording_server_wakeup;
	char recording_server_mac[31];
	int  recording_vcr_no_scart;
	char recording_splitsize[10];
	int  recording_use_o_sync;
	int  recording_use_fdatasync;
	unsigned char recording_audio_pids_default;
	int recording_audio_pids_std;
	int recording_audio_pids_alt;
	int recording_audio_pids_ac3;
	int  recording_stream_vtxt_pid;
	int  recording_stream_subtitle_pid;
	char recording_ringbuffers[10];
	int recording_in_spts_mode;
	int recording_choose_direct_rec_dir;
	int recording_epg_for_filename;
#define REC_FILENAME_TEMPLATE_NR_OF_ENTRIES 4
	std::string recording_filename_template[REC_FILENAME_TEMPLATE_NR_OF_ENTRIES];
	char recording_dir_permissions[REC_FILENAME_TEMPLATE_NR_OF_ENTRIES][4];
	int  recording_zap_on_announce;

#define MAX_RECORDING_DIR 10	
	std::string recording_dir[MAX_RECORDING_DIR];
	//streaming
	int  streaming_type;
	std::string streaming_server_ip;
	char streaming_server_port[10];
	char streaming_server_cddrive[21];
	char streaming_videorate[6];
	char streaming_audiorate[6];
	char streaming_server_startdir[40];
	int streaming_transcode_audio;
	int streaming_force_avi_rawaudio;
	int streaming_force_transcode_video;
	int streaming_transcode_video_codec;
	int streaming_resolution;
	int streaming_use_buffer;
	int streaming_buffer_segment_size;
	int streaming_show_tv_in_browser;
	int streaming_allow_multiselect;

	int filesystem_is_utf8;
	// default plugin for ts-movieplayer (red button)
	std::string movieplayer_plugin;

	//key configuration
	int key_tvradio_mode;

	int key_channelList_pageup;
	int key_channelList_pagedown;
	int key_channelList_cancel;
	int key_channelList_sort;
	int key_channelList_addrecord;
	int key_channelList_addremind;
	int key_channelList_reload;

	int key_quickzap_up;
	int key_quickzap_down;
	int key_bouquet_up;
	int key_bouquet_down;
	int key_subchannel_up;
	int key_subchannel_down;
	int key_zaphistory;
	int key_lastchannel;

	char repeat_blocker[4];
	char repeat_genericblocker[4];
	int audiochannel_up_down_enable;
	int audio_left_right_selectable;

	//screen configuration
	int screen_StartX;
	int screen_StartY;
	int screen_EndX;
	int screen_EndY;

	//Software-update
	int softupdate_mode;
	char softupdate_url_file[31];
	char softupdate_proxyserver[31];
	char softupdate_proxyusername[31];
	char softupdate_proxypassword[31];

	//BouquetHandling
	int bouquetlist_mode;

	// parentallock
	int parentallock_prompt;
	int parentallock_lockage;
	char parentallock_pincode[5];


	// Font sizes
#define FONT_TYPE_COUNT 24
	enum FONT_TYPES {
		FONT_TYPE_MENU                =  0,
		FONT_TYPE_MENU_TITLE          =  1,
		FONT_TYPE_MENU_INFO           =  2,
		FONT_TYPE_EPG_TITLE           =  3,
		FONT_TYPE_EPG_INFO1           =  4,
		FONT_TYPE_EPG_INFO2           =  5,
		FONT_TYPE_EPG_DATE            =  6,
		FONT_TYPE_EVENTLIST_TITLE     =  7,
		FONT_TYPE_EVENTLIST_ITEMLARGE =  8,
		FONT_TYPE_EVENTLIST_ITEMSMALL =  9,
		FONT_TYPE_EVENTLIST_DATETIME  = 10,
		FONT_TYPE_GAMELIST_ITEMLARGE  = 11,
		FONT_TYPE_GAMELIST_ITEMSMALL  = 12,
		FONT_TYPE_CHANNELLIST         = 13,
		FONT_TYPE_CHANNELLIST_DESCR   = 14,
		FONT_TYPE_CHANNELLIST_NUMBER  = 15,
		FONT_TYPE_CHANNEL_NUM_ZAP     = 16,
		FONT_TYPE_INFOBAR_NUMBER      = 17,
		FONT_TYPE_INFOBAR_CHANNAME    = 18,
		FONT_TYPE_INFOBAR_INFO        = 19,
		FONT_TYPE_INFOBAR_SMALL       = 20,
		FONT_TYPE_FILEBROWSER_ITEM    = 21,
		FONT_TYPE_IMAGEINFO_INFO      = 22,
		FONT_TYPE_IMAGEINFO_SMALL     = 23
	};

	// lcdd
#define LCD_SETTING_COUNT 7
	enum LCD_SETTINGS {
		LCD_BRIGHTNESS         = 0,
		LCD_STANDBY_BRIGHTNESS = 1,
		LCD_CONTRAST           = 2,
		LCD_POWER              = 3,
		LCD_INVERSE            = 4,
		LCD_SHOW_VOLUME        = 5,
		LCD_AUTODIMM           = 6,
	};
	int lcd_setting[LCD_SETTING_COUNT];

	char lcd_setting_dim_time[4];
	char lcd_setting_dim_brightness[4];

#define FILESYSTEM_ENCODING_TO_UTF8(a) (g_settings.filesystem_is_utf8 ? (a) : ZapitTools::Latin1_to_UTF8(a).c_str())
#define UTF8_TO_FILESYSTEM_ENCODING(a) (g_settings.filesystem_is_utf8 ? (a) : ZapitTools::UTF8_to_Latin1(a).c_str())
#define FILESYSTEM_ENCODING_TO_UTF8_STRING(a) (g_settings.filesystem_is_utf8 ? (a) : Latin1_to_UTF8(a))

#ifdef HAVE_DREAMBOX_HARDWARE
#define DRIVER_SETTING_FILES_COUNT 2
#else
#if HAVE_DVB_API_VERSION == 1
#define DRIVER_SETTING_FILES_COUNT 6
#else
#define DRIVER_SETTING_FILES_COUNT 5
#endif
#endif

// #define MISC_SETTING_SPTS_MODE 0

	int misc_option[DRIVER_SETTING_FILES_COUNT];


	// pictureviewer
	char   picviewer_slide_time[3];
	int    picviewer_scaling;
	std::string picviewer_decode_server_ip;
	char    picviewer_decode_server_port[6];

	//audioplayer
	int   audioplayer_display;
	int   audioplayer_follow;
	char  audioplayer_screensaver[3];
	int   audioplayer_highprio;
	int   audioplayer_select_title_by_name;
	int   audioplayer_repeat_on;
	int   audioplayer_show_playlist;
	int   audioplayer_enable_sc_metadata;

	//Filebrowser
	int filebrowser_showrights;
	int filebrowser_sortmethod;
	int filebrowser_denydirectoryleave;

	//uboot
	int	uboot_lcd_inverse;
	int	uboot_lcd_contrast;
	int	uboot_console;
	int	uboot_console_bak;

	// USERMENU
	typedef enum
	{
		BUTTON_RED = 0,  // Do not change ordering of members, add new item just before BUTTON_MAX!!!
		BUTTON_GREEN = 1,
		BUTTON_YELLOW = 2,
		BUTTON_BLUE = 3,
		BUTTON_MAX   // MUST be always the last in the list
	}USER_BUTTON;
	typedef enum
	{
		ITEM_NONE = 0, // Do not change ordering of members, add new item just before ITEM_MAX!!!
		ITEM_BAR = 1,
		ITEM_EPG_LIST = 2,
		ITEM_EPG_SUPER = 3,
		ITEM_EPG_INFO = 4,
		ITEM_EPG_MISC = 5,
		ITEM_AUDIO_SELECT = 6,
		ITEM_SUBCHANNEL = 7,
		ITEM_PLUGIN = 8,  
		ITEM_VTXT = 9,
		ITEM_RECORD = 10, 
		ITEM_MOVIEPLAYER_TS = 11,
		ITEM_MOVIEPLAYER_MB = 12,
		ITEM_TIMERLIST = 13,
		ITEM_REMOTE = 14,
		ITEM_FAVORITS = 15,
		ITEM_TECHINFO = 16,
		ITEM_MAX   // MUST be always the last in the list
	}USER_ITEM;
	std::string usermenu_text[BUTTON_MAX];
	int usermenu[BUTTON_MAX][ITEM_MAX];  // (USER_ITEM)  [button][position in Menue] = feature item
};

/* some default Values */

extern const int               default_timing     [TIMING_SETTING_COUNT];
extern const neutrino_locale_t timing_setting_name[TIMING_SETTING_COUNT];

// lcdd
#define DEFAULT_LCD_BRIGHTNESS			0xff
#define DEFAULT_LCD_STANDBYBRIGHTNESS		0xaa
#define DEFAULT_LCD_CONTRAST			0x0F
#define DEFAULT_LCD_POWER			0x01
#define DEFAULT_LCD_INVERSE			0x00
#define DEFAULT_LCD_AUTODIMM			0x00
#define DEFAULT_LCD_SHOW_VOLUME			0x01

/* end default values */

struct SglobalInfo
{
	unsigned char     box_Type;
	delivery_system_t delivery_system;
};

const int RECORDING_OFF    = 0;
const int RECORDING_SERVER = 1;
const int RECORDING_VCR    = 2;
const int RECORDING_FILE   = 3;

const int PARENTALLOCK_PROMPT_NEVER          = 0;
const int PARENTALLOCK_PROMPT_ONSTART        = 1;
const int PARENTALLOCK_PROMPT_CHANGETOLOCKED = 2;
const int PARENTALLOCK_PROMPT_ONSIGNAL       = 3;

#ifdef HAVE_DREAMBOX_HARDWARE
#define MAX_SATELLITES 100
#else
#define MAX_SATELLITES 64
#endif
class CScanSettings
{
 public:
	CConfigFile               configfile;
	CZapitClient::bouquetMode bouquetMode;
	CZapitClient::scanType scanType;
	diseqc_t                  diseqcMode;
	uint32_t                  diseqcRepeat;
	int			  satCount;
	char                      satNameNoDiseqc[30];
	int                       satDiseqc[MAX_SATELLITES];
	int	                  satMotorPos[MAX_SATELLITES];
	t_satellite_position	  satPosition[MAX_SATELLITES];
	char	                  satName[MAX_SATELLITES][30];
	delivery_system_t         delivery_system;

	int		scanSectionsd;

	int		scan_mode;
	int		TP_scan;
	int		TP_fec;
	int		TP_pol;
	char		TP_freq[9];
	char		TP_rate[9];
	char		TP_satname[30];
	int		TP_diseqc;
	CMenuOptionStringChooser* TP_SatSelectMenu;

	CScanSettings();

	int* diseqscOfSat( char* satname);
	int* motorPosOfSat( char* satname);
	char* satOfDiseqc(int diseqc) const;
	char* satOfMotorPos(int32_t motorPos) const;
	void toSatList( CZapitClient::ScanSatelliteList& ) const;
	void toMotorPosList( CZapitClient::ScanMotorPosList& ) const;

	void useDefaults(const delivery_system_t _delivery_system);

	bool loadSettings(const char * const fileName, const delivery_system_t _delivery_system);
	bool saveSettings(const char * const fileName);
};


#endif
