/*
	$Id: neutrino.h,v 1.228 2009/10/27 20:28:42 dbt Exp $

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


#ifndef __neutrino__
#define __neutrino__

#include <configfile.h>

#include <neutrinoMessages.h>
#include <driver/framebuffer.h>
#include <system/setting_helpers.h>
#include <system/configure_network.h>
#include <gui/timerlist.h>
#include <timerdclient/timerdtypes.h>
#include <gui/channellist.h>          /* CChannelList */
#include <gui/rc_lock.h>
#include <daemonc/remotecontrol.h>    /* st_rmsg      */

#include <zapit/client/zapitclient.h>

#include <string>

#define widest_number "2"

#define ANNOUNCETIME (1 * 60)
#define PLUGINDIR_VAR "/var/tuxbox/plugins"
#define LCDDIR_VAR "/var/share/tuxbox/neutrino/lcdd"

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  main run-class                                             *
*                                                                                     *
**************************************************************************************/

typedef struct neutrino_font_descr
{
	const char * name;
	const char * filename[3];
	int          size_offset;
} neutrino_font_descr_struct;

typedef struct font_sizes
{
	const neutrino_locale_t name;
	const unsigned int      defaultsize;
	const unsigned int      style;
	const unsigned int      size_offset;
} font_sizes_struct;

typedef struct font_sizes_groups
{
	const neutrino_locale_t                     groupname;
	const unsigned int                          count;
	const SNeutrinoSettings::FONT_TYPES * const content;
	const char * const                          actionkey;
} font_sizes_groups_struct;

#define FONT_STYLE_REGULAR 0
#define FONT_STYLE_BOLD    1
#define FONT_STYLE_ITALIC  2

extern const font_sizes_struct neutrino_font[];
extern const char * locale_real_names[]; /* #include <system/locals_intern.h> */

#define OPTIONS_OFF0_ON1_OPTION_COUNT 2
extern const CMenuOptionChooser::keyval OPTIONS_OFF0_ON1_OPTIONS[];

class CNeutrinoApp : public CMenuTarget, CChangeObserver
{
 public:
	enum
		{
			RECORDING_OFF    = 0,
			RECORDING_SERVER = 1,
			RECORDING_VCR    = 2,
			RECORDING_FILE   = 3
		};

	enum
		{
			DIRECTORMODE_PORTAL	= -3,
			DIRECTORMODE_TOGGLE	= -2
		};

		void saveSetup();

 private:
	CFrameBuffer * frameBuffer;

	enum
		{
			mode_unknown = -1,
			mode_tv = 1,
			mode_radio = 2,
			mode_scart = 3,
			mode_standby = 4,
			mode_audio = 5,
			mode_pic = 6,
			mode_ts = 7,
			mode_mask = 0xFF,
			norezap = 0x100
		};

	enum
		{
			init_mode_unknown 	= -1,
			init_mode_init 		= 1,
			init_mode_record 	= 2,
			init_mode_switch 	= 3,
		};

	enum
		{
			STARTMODE_UNKNOWN	= -1,
			STARTMODE_RESTORE	= 0,
			STARTMODE_TV		= 1,
			STARTMODE_RADIO		= 2,
			STARTMODE_SCART		= 3,
			STARTMODE_AUDIOPLAYER	= 4,
			STARTMODE_INETRADIO	= 5,
			STARTMODE_ESOUND	= 6,
			STARTMODE_STANDBY	= 7,
		};

		CConfigFile			configfile;
		CScanSettings			scanSettings;
		int				network_dhcp;
		int				network_automatic_start;

		neutrino_font_descr_struct	font;

		int				mode;
		int				lastMode;
		int				tunerMode;
		bool				wakeupfromScart;
		bool				standbyAfterRecord;
		bool				obeyStartMode;
		bool				softupdate;
		bool				fromflash;
		int				recording_id;
		CTimerd::RecordingInfo* 	nextRecordingInfo;
		//bool		record_mode;

		struct timeval			standby_pressed_at;

		CZapitClient::responseGetLastChannel    firstchannel;
		st_rmsg				sendmessage;

		bool				current_muted;

		bool				skipShutdownTimer;

		CColorSetupNotifier		*colorSetupNotifier;

		CKeySetupNotifier       	*keySetupNotifier;
		CShutdownCountNotifier		*shutdownCountNotifier;

		CNVODChangeExec         	*NVODChanger;
#ifdef HAVE_DBOX_HARDWARE
		CUCodeCheckExec			*UCodeChecker;
#endif
		CDVBInfoExec			*DVBInfo;
		CStreamFeaturesChangeExec	*StreamFeaturesChanger;
		CIPChangeNotifier		*MyIPChanger;
//		CVCRControl			*vcrControl;
		CConsoleDestChangeNotifier	*ConsoleDestinationChanger;
		CFdxChangeNotifier		*FdxSettingsChanger;
		CRCLock				*rcLock;
#ifdef ENABLE_MOVIEPLAYER
		CMenuTarget* 			moviePlayerGui;
#ifdef ENABLE_MOVIEBROWSER
		CMenuTarget*			movieBrowser;
#endif
#endif
		bool 				parentallocked;
		CFontSizeNotifier 		*fontsizenotifier;
		bool 				waitforshutdown;
		bool				volumeBarIsVisible;

		// USERMENU
		CTimerList			*Timerlist;

		bool showUserMenu(int button);
		bool getNVODMenu(CMenuWidget* menu);

#ifdef HAVE_DBOX_HARDWARE
		bool ucodes_available(void);
#endif
		void firstChannel();
		void setupColors_neutrino();
		void setupNetwork( bool force= false );
		void setupNFS();
		void setupRecordingDevice(void);

		void startNextRecording();

		int loadSetup();

		void tvMode( bool rezap = true );
		void radioMode( bool rezap = true );
		void scartMode( bool bOnOff );
		void standbyMode( bool bOnOff );
		void setVolume(const neutrino_msg_t key, const bool bDoPaint = true);
		void AudioMute( bool newValue, bool isEvent= false );
		void paintMuteIcon( bool is_visible = true );
		
		void ExitRun(const bool write_si);
		
		void RealRun(CMenuWidget &mainSettings);
		void InitZapper();
		void InitKeySettings(CMenuWidget &);
		void InitServiceSettings(CMenuWidget &);
		void InitColorSettingsMenuColors(CMenuWidget &);
		void InitColorSettings(CMenuWidget &, CMenuWidget &);
		void InitLanguageSettings(CMenuWidget &);
		void InitColorThemesSettings(CMenuWidget &);
		void InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_menuColors);
		void InitColorSettingsTiming(CMenuWidget &colorSettings_timing);
		void InitLcdSettings(CMenuWidget &lcdSettings);
		void InitNetworkSettings(CMenuWidget &networkSettings);
		void AddFontSettingItem(CMenuWidget &fontSettings, const SNeutrinoSettings::FONT_TYPES number_of_fontsize_entry);
		void InitFontSettings(CMenuWidget &fontSettings);
		void InitRecordingSettings(CMenuWidget &recordingSettings);
		void InitStreamingSettings(CMenuWidget &streamingSettings);
		void InitScreenSettings(CMenuWidget &);
		void InitAudioplPicSettings(CMenuWidget &);
		void InitDriverSettings(CMenuWidget &);
		void InitZapitSettings(CMenuWidget &miscSettingsZapitSettings);
		void InitMiscSettings(CMenuWidget &miscSettings,
								CMenuWidget &miscSettingsGeneral,
								CMenuWidget &miscSettingsOSDExtras,
								CMenuWidget &miscSettingsInfobar,
								CMenuWidget &miscSettingsChannellist,
								CMenuWidget &miscSettingsEPGSettings,
								CMenuWidget &miscSettingsZapitSettings,
								CMenuWidget &miscSettingsRemoteControl,
								CMenuWidget &miscSettingsFilebrowser);
		void InitScanSettings(CMenuWidget &);
		void InitParentalLockSettings(CMenuWidget &);
		void InitMainMenu(CMenuWidget &mainMenu,
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
				  CMenuWidget &service);
		void addMenueIntroItems(CMenuWidget &item);

		void SetupTiming();
		void SetupFrameBuffer();
		void SelectAPID();
		void SelectNVOD();
		void CmdParser(int argc, char **argv);
		void ShowStreamFeatures();
		bool doGuiRecord(char * preselectedDir, bool addTimer = false, char * filename = NULL);
		bool doShowMuteIcon(void);
		void prepareEnviroment();
		CNeutrinoApp();

	public:
		void SetupFonts();
		~CNeutrinoApp();
		const CScanSettings& getScanSettings(){ return scanSettings;};
		CScanSettings& ScanSettings(){ return scanSettings;};

		CChannelList			*channelList;
		CChannelList			*channelListTV;
		CChannelList			*channelListRADIO;
		CChannelList			*channelListRecord;
		CNetworkConfig                  networkConfig;

		static CNeutrinoApp* getInstance();

		void channelsInit(int init_mode, int mode = -1);
		void channelsInit4Record();
		int run(int argc, char **argv);
		//callback stuff only....
		int exec(CMenuTarget* parent, const std::string & actionKey);

		//onchange
		bool changeNotify(const neutrino_locale_t OptionName, void *);

		int handleMsg(const neutrino_msg_t msg, neutrino_msg_data_t data);

		int getMode() {return mode;}
		int getLastMode() {return lastMode;}
		bool isMuted() {return current_muted;}
		int recordingstatus;
		bool zapto_tv_on_init_done;
		bool zapto_radio_on_init_done;
		void SendSectionsdConfig(void);

		bool execute_start_file(const char *filename, const bool blocking = true, const bool verbose = false);
		int execute_sys_command(const char *command);
};


#endif
