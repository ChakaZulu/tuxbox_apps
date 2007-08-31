/*
	$Id: neutrino.h,v 1.189 2007/08/31 11:27:25 nitr8 Exp $

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

		CConfigFile			configfile;
		CScanSettings			scanSettings;
		int                             network_dhcp;
		int                             network_automatic_start;

		neutrino_font_descr_struct      font;

		int				mode;
		int				lastMode;
		bool				softupdate;
		bool				fromflash;
		int				recording_id;
		CTimerd::RecordingInfo* nextRecordingInfo;
		//bool				record_mode;

		struct timeval                  standby_pressed_at;

		CZapitClient::responseGetLastChannel    firstchannel;
		st_rmsg				sendmessage;

		bool				current_muted;

		bool				skipShutdownTimer;

		CColorSetupNotifier		*colorSetupNotifier;

		CKeySetupNotifier       	*keySetupNotifier;
		CShutdownCountNotifier		*shutdownCountNotifier;

		CNVODChangeExec         	*NVODChanger;
#ifndef HAVE_DREAMBOX_HARDWARE
		CUCodeCheckExec			*UCodeChecker;
#endif
		CStreamFeaturesChangeExec	*StreamFeaturesChanger;
		CMoviePluginChangeExec 		*MoviePluginChanger;
		CIPChangeNotifier		*MyIPChanger;
//		CVCRControl			*vcrControl;
		CConsoleDestChangeNotifier	*ConsoleDestinationChanger;
		CRCLock                         *rcLock;
		CMenuTarget* 			moviePlayerGui;

		// USERMENU
		CTimerList			*Timerlist;
		bool showUserMenu(int button);
		bool getNVODMenu(CMenuWidget* menu);

#ifndef HAVE_DREAMBOX_HARDWARE
		bool ucodes_available(void);
#endif
		void firstChannel();
		void setupColors_classic();
		void setupColors_neutrino();
		void setupColors_dblue();
		void setupColors_dvb2k();
		void setupColors_virginmedia();
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


		void ExitRun(const bool write_si);
		void RealRun(CMenuWidget &mainSettings);
		void InitZapper();
		void InitKeySettings(CMenuWidget &);
		void InitServiceSettings(CMenuWidget &, CMenuWidget &);
		void InitColorSettingsMenuColors(CMenuWidget &);
		void InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier* audioSetupNotifier);
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
		void InitMiscSettings(CMenuWidget &);
		void InitScanSettings(CMenuWidget &);
		void InitParentalLockSettings(CMenuWidget &);
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
								CMenuWidget &moviePlayer);

		void SetupTiming();
		void SetupFrameBuffer();
		void SelectAPID();
		void SelectNVOD();
		void CmdParser(int argc, char **argv);
		void ShowStreamFeatures();
		bool doGuiRecord(char * preselectedDir, bool addTimer = false);
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
		void SendSectionsdConfig(void);
};


#endif
