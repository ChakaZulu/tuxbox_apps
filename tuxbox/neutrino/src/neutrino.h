/*
//  $Id: neutrino.h,v 1.23 2001/10/07 12:17:22 McClean Exp $

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

//  $Log: neutrino.h,v $
//  Revision 1.23  2001/10/07 12:17:22  McClean
//  video mode setup (pre)
//
//  Revision 1.22  2001/10/04 23:21:13  McClean
//  cleanup
//
//  Revision 1.21  2001/10/02 23:16:48  McClean
//  game interface
//
//  Revision 1.20  2001/10/01 20:41:08  McClean
//  plugin interface for games - beta but nice.. :)
//
//  Revision 1.19  2001/09/20 19:21:37  fnbrd
//  Channellist mit IDs.
//
//  Revision 1.18  2001/09/20 00:36:32  field
//  epg mit zaopit zum grossteil auf onid & s_id umgestellt
//
//  Revision 1.17  2001/09/19 18:03:14  field
//  Infobar, Sprachauswahl
//
//  Revision 1.16  2001/09/18 20:20:26  field
//  Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
//  vorbereitet
//
//  Revision 1.15  2001/09/18 10:49:49  fnbrd
//  Eventlist, quick'n dirty
//

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
#include <dirent.h>
#include <dlfcn.h>


#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"

#include "widget/eventlist.hpp"
#include "widget/color.h"
#include "widget/menue.h"
#include "widget/channellist.h"
#include "widget/colorchooser.h"
#include "widget/keychooser.h"
#include "widget/stringinput.h"
#include "widget/screensetup.h"
#include "widget/gamelist.h"
#include "helpers/setting_helpers.h"

#include "../zapit/getservices.h"

#include "daemonc/remotecontrol.h"
#include "daemonc/controld.h"

#include "helpers/infoviewer.h"
#include "helpers/epgdata.h"
#include "helpers/settings.h"
#include "helpers/streaminfo.h"
#include "helpers/locale.h"

#include <string>
#include <vector>
#include <map>

using namespace std;


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  main run-class                                             *
*                                                                                     *
**************************************************************************************/

class CNeutrinoApp : public CMenuTarget
{
  private:
	enum
	{
		mode_tv = 1,
		mode_radio = 2
	};

//    EventList *eventlist;
	string				settingsFile;

	bool				nRun;
	int				    mode;
	bool				mute;
		
    channel_msg         firstchannel;
	st_rmsg				sendmessage;

	CColorSetupNotifier		*colorSetupNotifier;
	CAudioSetupNotifier		*audioSetupNotifier;
	CVideoSetupNotifier		*videoSetupNotifier;
    CLanguageSetupNotifier  *languageSetupNotifier;
    CKeySetupNotifier       *keySetupNotifier;
    CAPIDChangeExec         *APIDChanger;

	CChannelList		*channelList;

	void PluginDemo(); //demo only --- remove!

	void firstChannel();
	void channelsInit();
	void setupDefaults();
	void setupColors_classic();
	void setupColors_neutrino();
	void setupNetwork( bool force= false );

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
	void InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier* audioSetupNotifier);
	void InitColorSettings(CMenuWidget &);
	void InitLanguageSettings(CMenuWidget &);
	void InitColorThemesSettings(CMenuWidget &);
	void InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_menuColors, CMenuWidget &);
	void InitNetworkSettings(CMenuWidget &networkSettings);
	void InitScreenSettings(CMenuWidget &);
	void InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier* videoSetupNotifier);
	void InitMainSettings(CMenuWidget &mainSettings, CMenuWidget &audioSettings, CMenuWidget &networkSettings,
			CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings, CMenuWidget &languageSettings);
	void ClearFrameBuffer();
	void SetupFonts();
	void SetupFrameBuffer();
    void SelectAPID();
	void CmdParser(int argc, char **argv);


	public:

	CNeutrinoApp();
	~CNeutrinoApp();

	int run(int argc, char **argv);
	//callback stuff only....
	int exec(CMenuTarget* parent, string actionKey);
};

#endif



