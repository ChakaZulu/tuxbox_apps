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


#ifndef __neutrino__
#define __neutrino__


#include "driver/framebuffer.h"
#include "helpers/setting_helpers.h"

#include <string>

using namespace std;


#define widest_number "1"


struct messages_return {
	enum
		{
			none 		= 0x00,
			handled		= 0x01,
			unhandled	= 0x02,
			cancel_all	= 0x04,
			cancel_info = 0x08
		};
};

struct NeutrinoMessages {
	enum
		{
			SHOW_EPG	= CRCInput::RC_Messages + 1,
			SHOW_INFOBAR= CRCInput::RC_Messages + 2,
			VCR_ON		= CRCInput::RC_Messages + 3,
			VCR_OFF		= CRCInput::RC_Messages + 4,
			STANDBY_ON	= CRCInput::RC_Messages + 5,
			STANDBY_OFF	= CRCInput::RC_Messages + 6,
			SHUTDOWN	= CRCInput::RC_Messages + 7,

			EVT_VOLCHANGED 	= 			CRCInput::RC_Events + 1,
			EVT_MUTECHANGED	=	 		CRCInput::RC_Events + 2,
			EVT_VCRCHANGED	= 			CRCInput::RC_Events + 3,
			EVT_MODECHANGED = 			CRCInput::RC_Events + 4,
			EVT_BOUQUETSCHANGED =		CRCInput::RC_Events + 6,
			EVT_SERVICESCHANGED =		CRCInput::RC_Events + 7,
			EVT_CURRENTNEXT_EPG =		CRCInput::RC_Events + 8,
			EVT_ZAP_GOT_SUBSERVICES =	CRCInput::RC_Events + 9,
			EVT_ZAP_GOTPIDS		= 		CRCInput::RC_Events + 10,
			EVT_ZAP_COMPLETE	= 		CRCInput::RC_Events + 11,
			EVT_ZAP_GOTAPIDS	= 		CRCInput::RC_Events + 12,
			EVT_ZAP_FAILED		= 		CRCInput::RC_Events + 13,
			EVT_ZAP_ISNVOD		=		CRCInput::RC_Events + 14,
			EVT_ZAP_SUB_COMPLETE=		CRCInput::RC_Events + 15,
			EVT_SCAN_COMPLETE	=		CRCInput::RC_Events + 16,
			EVT_SCAN_NUM_TRANSPONDERS =	CRCInput::RC_Events + 17,
			EVT_SCAN_NUM_CHANNELS =		CRCInput::RC_Events + 18,
			EVT_SHUTDOWN =				CRCInput::RC_Events + 19,
			EVT_TIMER =					CRCInput::RC_Events + 20,
			EVT_NEXTPROGRAM		=		CRCInput::RC_Events + 21,
			EVT_PROGRAMLOCKSTATUS	=	CRCInput::RC_Events + 22,
			EVT_NOEPG_YET		=		CRCInput::RC_Events + 23,

			EVT_CURRENTEPG 		=		CRCInput::RC_WithData + 1,
			EVT_SCAN_SATELLITE	=		CRCInput::RC_WithData + 2,
			EVT_SCAN_PROVIDER	=		CRCInput::RC_WithData + 3,
			EVT_NEXTEPG 		=		CRCInput::RC_WithData + 4,
			EVT_TIMESET 		= 		CRCInput::RC_WithData + 5
		};
};

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  main run-class                                             *
*                                                                                     *
**************************************************************************************/

class CNeutrinoApp : public CMenuTarget, COnPaintNotifier, CChangeObserver
{
	private:

		CFrameBuffer	*frameBuffer;

		enum
		{
			mode_unknown = -1,
		    mode_tv = 1,
		    mode_radio = 2,
		    mode_scart = 3,
		    mode_standby = 4
		};


		struct streaming_commandhead
		{
			char version;
			char command;
		};

		string				settingsFile;
		string				scanSettingsFile;
		CScanSettings		scanSettings;

		int				    mode;
		int					lastMode;
		bool				softupdate;
		bool				fromflash;
		int					streamstatus;
		long long 			standby_pressed_at;

		channel_msg         firstchannel;
		st_rmsg				sendmessage;

		char				current_volume;
		bool				current_muted;

		CColorSetupNotifier			*colorSetupNotifier;
		CAudioSetupNotifier			*audioSetupNotifier;
		CVideoSetupNotifier			*videoSetupNotifier;
		CLanguageSetupNotifier  	*languageSetupNotifier;
		CKeySetupNotifier       	*keySetupNotifier;
		CAPIDChangeExec         	*APIDChanger;
		CNVODChangeExec         	*NVODChanger;
		CUCodeCheckExec				*UCodeChecker;
		CStreamFeaturesChangeExec	*StreamFeaturesChanger;
		CIPChangeNotifier			*MyIPChanger;


		void doChecks();
		void firstChannel();
		void setupDefaults();
		void setupColors_classic();
		void setupColors_neutrino();
		void setupNetwork( bool force= false );
		void testNetwork();

		void saveSetup();
		bool loadSetup(SNeutrinoSettings* load2=NULL);

		void tvMode( bool rezap = true );
		void radioMode( bool rezap = true );
		void scartMode( bool bOnOff );
		void standbyMode( bool bOnOff );
		void setVolume(int key, bool bDoPaint = true);
		void AudioMute( bool newValue, bool isEvent= false );

		void ExitRun();
		void RealRun(CMenuWidget &mainSettings);
		void InitZapper();
		void InitKeySettings(CMenuWidget &);
		void InitServiceSettings(CMenuWidget &);
		void InitColorSettingsMenuColors(CMenuWidget &, CMenuWidget &);
		void InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier* audioSetupNotifier);
		void InitColorSettings(CMenuWidget &);
		void InitLanguageSettings(CMenuWidget &);
		void InitColorThemesSettings(CMenuWidget &);
		void InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_menuColors, CMenuWidget &);
		void InitNetworkSettings(CMenuWidget &networkSettings);
		void InitScreenSettings(CMenuWidget &);
		void InitMiscSettings(CMenuWidget &);
		void InitScanSettings(CMenuWidget &);
		void InitParentalLockSettings(CMenuWidget &);
		void InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier* videoSetupNotifier);
		void InitMainMenu(CMenuWidget &mainMenu, CMenuWidget &mainSettings, CMenuWidget &audioSettings,
		                  CMenuWidget &parentallockSettings, CMenuWidget &networkSettings,
		                  CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings,
		                  CMenuWidget &languageSettings, CMenuWidget &miscSettings, CMenuWidget &scanSettings,
		                  CMenuWidget &service);
		void ClearFrameBuffer();
		void SetupFonts();
		void SetupFrameBuffer();
		void SelectAPID();
		void SelectNVOD();
		void CmdParser(int argc, char **argv);
		void ShowStreamFeatures();

        long long last_profile_call;

		CNeutrinoApp();
		~CNeutrinoApp();

	public:
		const CScanSettings& getScanSettings(){ return scanSettings;};

		CChannelList				*channelList;

		static CNeutrinoApp* getInstance();

		void channelsInit();
		int run(int argc, char **argv);
		//callback stuff only....
		int exec(CMenuTarget* parent, string actionKey);
		//callback for menue
		bool onPaintNotify(string MenuName);
		//onchange
		bool changeNotify(string OptionName);

		int handleMsg(uint msg, uint data);
		void showProfiling( string text );

	friend class CNeutrinoBouquetEditorEvents;
};


#endif



