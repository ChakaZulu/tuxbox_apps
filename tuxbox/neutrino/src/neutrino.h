/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://mcclean.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumöglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons übernommen.
	

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
class CNeutrinoApp : public CMenuTarget
{
	enum
	{
		mode_tv = 0,
		mode_radio = 1
	};

		struct SNeutrinoSettings	settings;
		string				settingsFile;

		bool				nRun;
		int				mode;
		char				volume;
		bool				mute;
		
		st_rmsg				sendmessage;
		channel_msg			zapitchannel;
		channel_msg			firstchannel;
		bool				zapit;

		CRCInput			rcInput;
		CFrameBuffer		frameBuffer;
		fontRenderClass		*fontRenderer;
		FontsDef			fonts;

		CColorSetupNotifier* colorSetupNotifier;

		CChannelList		*channelList;
		CRemoteControl		remoteControl;
		CControld			Controld;
		CInfoViewer			infoViewer;
		CEpgData			epgData;

		void firstChannel();
		void channelsInit();
		void setupDefaults(SNeutrinoSettings* settings);
		void setupColors_classic(SNeutrinoSettings* settings);
		void setupColors_neutrino(SNeutrinoSettings* settings);
		void saveSetup(SNeutrinoSettings* settings);
		bool loadSetup(SNeutrinoSettings* settings);

		void tvMode();
		void radioMode();
		void setVolume(int key);
		void AudioMute();
		void AudioUnMute();

	public:

		CNeutrinoApp();
		~CNeutrinoApp();

		int run(int argc, char **argv);
		//callback stuff only....
		int exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CMenuTarget* parent, string actionKey);
};



#endif
