/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://mcclean.cyberphoria.org/

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


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"

#include "widget/color.h"
#include "widget/menue.h"
#include "widget/channellist.h"
#include "widget/colorchooser.h"
#include "widget/keychooser.h"
#include "widget/stringinput.h"
#include "widget/screensetup.h"

#include "../zapit/getservices.h"

#include "daemonc/remotecontrol.h"
#include "daemonc/controld.h"
#include "helpers/infoviewer.h"
#include "helpers/epgdata.h"
#include "helpers/settings.h"
#include "helpers/streaminfo.h"

#include <string>
#include <vector>
#include <map>

using namespace std;


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  main run-class                                             *
*                                                                                     *
**************************************************************************************/
class CColorSetupNotifier;
class CAudioSetupNotifier;
class CVideoSetupNotifier;
class CNetworkSetupNotifier;
class CNeutrinoApp : public CMenuTarget
{
	enum
	{
		mode_tv = 0,
		mode_radio = 1
	};

	string				settingsFile;
//    struct SNeutrinoSettings	settings;

	bool				nRun;
	int				    mode;
	char				volume;
	bool				mute;
		
	st_rmsg				sendmessage;
	channel_msg			zapitchannel;
	channel_msg			firstchannel;
	bool				zapit;

//	CRCInput			rcInput;
//	CFrameBuffer		frameBuffer;
	fontRenderClass		*fontRenderer;
//	FontsDef			*fonts;

	CColorSetupNotifier* colorSetupNotifier;

	CChannelList		*channelList;
//	CRemoteControl		remoteControl;
//	CControld			Controld;
//	CInfoViewer			infoViewer;
//	CEpgData			epgData;

	void firstChannel();
	void channelsInit();
	void setupDefaults();
	void setupColors_classic();
	void setupColors_neutrino();
	void setupNetwork( bool force= false );
//	void saveSetup(SNeutrinoSettings* settings);
//	bool loadSetup(SNeutrinoSettings* settings);
    void saveSetup();
	bool loadSetup();

	void tvMode();
	void radioMode();
	void setVolume(int key);
	void AudioMuteToggle();

	void ExitRun();
	void RealRun(CMenuWidget &mainSettings);
	void InitZapper();
	void InitKeySettings(CMenuWidget &);
	void InitColorSettingsMenuColors(CMenuWidget &, CMenuWidget &);
	void InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier &audioSetupNotifier);
	void InitColorSettings(CMenuWidget &);
	void InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_menuColors, CMenuWidget &);
	void InitNetworkSettings(CMenuWidget &networkSettings, CNetworkSetupNotifier &networkSetupNotifier);
	void InitScreenSettings(CMenuWidget &);
	void InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier &videoSetupNotifier);
	void InitAudioThemesSettings(CMenuWidget &);
	void InitMainSettings(CMenuWidget &mainSettings, CMenuWidget &audioSettings, CMenuWidget &networkSettings, CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings);
	void ClearFrameBuffer();
	void SetupFonts();
	void SetupFrameBuffer();
	void CmdParser(int argc, char **argv);


	public:

	CNeutrinoApp();
	~CNeutrinoApp();

	int run(int argc, char **argv);
	//callback stuff only....
	int exec(CMenuTarget* parent, string actionKey);
};

#endif



