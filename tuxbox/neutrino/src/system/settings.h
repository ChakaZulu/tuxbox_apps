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

#ifndef __settings__
#define __settings__

#include <configfile.h>
#include <zapit/client/zapitclient.h>

#include <string>

struct SNeutrinoSettings
{
	//video
	int video_Signal;
	int video_Format;
	unsigned char video_csync;

	//misc
	int shutdown_real;
	int shutdown_real_rcdelay;
	int shutdown_showclock;
	char record_safety_time_before[3];
	char record_safety_time_after[3];
	int infobar_sat_display;

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
	int timing_menu;
	int timing_chanlist;
	int timing_epg;
	int timing_infobar;
	int timing_filebrowser;
	char timing_menu_string[4];
	char timing_chanlist_string[4];
	char timing_epg_string[4];
	char timing_infobar_string[4];
	char timing_filebrowser_string[4];

	//widget settings
	int widget_fade;

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
	std::string network_nfs_ip[4];
	char network_nfs_local_dir[4][100];
	char network_nfs_dir[4][100];
	int  network_nfs_automount[4];
	char network_nfs_mount_options[2][31];
	int  network_nfs_type[4];
	char network_nfs_username[4][31];
	char network_nfs_password[4][31];
   char network_nfs_mp3dir[100];
   char network_nfs_picturedir[100];
   char network_nfs_moviedir[100];
   char network_nfs_recordingdir[100];

	//recording
	int  recording_type;
	int  recording_stopplayback;
	int  recording_stopsectionsd;
	std::string recording_server_ip;
	char recording_server_port[10];
	int  recording_server_wakeup;
	char recording_server_mac[31];
	int  recording_vcr_no_scart;

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
    
	//key configuration
	int key_tvradio_mode;

	int key_channelList_pageup;
	int key_channelList_pagedown;
	int key_channelList_cancel;
	int key_channelList_sort;
	int key_channelList_addrecord;
	int key_channelList_addremind;

	int key_quickzap_up;
	int key_quickzap_down;
	int key_bouquet_up;
	int key_bouquet_down;
	int key_subchannel_up;
	int key_subchannel_down;

	char repeat_blocker[4];
	char repeat_genericblocker[4];

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
#define FONT_TYPE_COUNT 22
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
		FONT_TYPE_FILEBROWSER_ITEM    = 21
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
		LCD_AUTODIMM           = 6
	};

	int lcd_setting[LCD_SETTING_COUNT];

	// pictureviewer
	char   picviewer_slide_time[3];
	int    picviewer_scaling;

   	//mp3player
	int   mp3player_display;
	int   mp3player_follow;
	char  mp3player_screensaver[3];
	int   mp3player_highprio;

	//Filebrowser
	int filebrowser_showrights;
	int filebrowser_sortmethod;
	
	//uboot
	int	uboot_lcd_inverse;
	int	uboot_lcd_contrast;
	int	uboot_console;
	int	uboot_console_bak;
};

/* some default Values */

// lcdd
#define DEFAULT_LCD_BRIGHTNESS			0xff
#define DEFAULT_LCD_STANDBYBRIGHTNESS		0xaa
#define DEFAULT_LCD_CONTRAST			0x0F
#define DEFAULT_LCD_POWER			0x01
#define DEFAULT_LCD_INVERSE			0x00
#define DEFAULT_LCD_AUTODIMM			0x00
#define DEFAULT_LCD_SHOW_VOLUME			1

//timing
#define DEFAULT_TIMING_MENU			60
#define DEFAULT_TIMING_CHANLIST			60
#define DEFAULT_TIMING_EPG			120
#define DEFAULT_TIMING_INFOBAR			6
#define DEFAULT_TIMING_FILEBROWSER		60

/* end default values */

struct SglobalInfo
{
	unsigned char     box_Type;
	delivery_system_t delivery_system;
};


const int PARENTALLOCK_PROMPT_NEVER          = 0;
const int PARENTALLOCK_PROMPT_ONSTART        = 1;
const int PARENTALLOCK_PROMPT_CHANGETOLOCKED = 2;
const int PARENTALLOCK_PROMPT_ONSIGNAL       = 3;

#define MAX_SATELLITES 64

class CScanSettings
{
 public:
	CConfigFile               configfile;
	CZapitClient::bouquetMode bouquetMode;
	diseqc_t                  diseqcMode;
	uint32_t                  diseqcRepeat;
	int			  satCount;
	char                      satNameNoDiseqc[30];
	int                       satDiseqc[MAX_SATELLITES];
	int	                  satMotorPos[MAX_SATELLITES];
	t_satellite_position	  satPosition[MAX_SATELLITES];
	char	                  satName[MAX_SATELLITES][30];
	delivery_system_t         delivery_system;

	CScanSettings();

	int* diseqscOfSat( char* satname);
	int* motorPosOfSat( char* satname);
	char* CScanSettings::satOfDiseqc(int diseqc) const;
	char* CScanSettings::satOfMotorPos(int32_t motorPos) const;
	void toSatList( CZapitClient::ScanSatelliteList& ) const;
	void toMotorPosList( CZapitClient::ScanMotorPosList& ) const;

	void useDefaults(const delivery_system_t _delivery_system);

	bool loadSettings(const char * const fileName, const delivery_system_t _delivery_system);
	bool saveSettings(const char * const fileName);
};


#endif

