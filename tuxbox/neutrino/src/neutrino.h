/*
//  $Id: neutrino.h,v 1.41 2002/01/06 18:38:04 McClean Exp $
 
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
 
//  $Log: neutrino.h,v $
//  Revision 1.41  2002/01/06 18:38:04  McClean
//  save-optimize
//
//  Revision 1.40  2002/01/04 02:38:05  McClean
//  cleanup
//
//  Revision 1.39  2002/01/03 20:03:20  McClean
//  cleanup
//
//  Revision 1.38  2001/12/29 02:17:00  McClean
//  make some settings get from controld
//
//  Revision 1.37  2001/12/28 16:31:09  Simplex
//  libcontroldclient is now used
//
//  Revision 1.36  2001/12/19 18:41:25  McClean
//  change menue-structure
//
//  Revision 1.35  2001/12/14 16:56:42  faralla
//  better bouquet-key handling
//
//  Revision 1.34  2001/12/05 01:40:55  McClean
//  fixed bouquet-options bugs, soft-update-bug and add scartmode-support
//
//  Revision 1.33  2001/11/26 02:34:03  McClean
//  include (.../../stuff) changed - correct unix-formated files now
//
//  Revision 1.32  2001/11/23 16:58:41  McClean
//  update-functions
//
//  Revision 1.31  2001/11/23 13:47:37  faralla
//  check if card fits camalpha.bin
//
//  Revision 1.30  2001/11/19 22:53:33  Simplex
//  Neutrino can handle bouquets now.
//  There are surely some bugs and todo's but it works :)
//
//  Revision 1.29  2001/11/15 11:42:41  McClean
//  gpl-headers added
//
//  Revision 1.28  2001/10/31 12:35:39  field
//  sectionsd stoppen waehrend scan
//
//  Revision 1.27  2001/10/15 17:27:19  field
//  nvods (fast) implementiert (umschalten funkt noch nicht)
//
//  Revision 1.26  2001/10/14 23:32:15  McClean
//  menu structure - prepared for VCR-Switching
//
//  Revision 1.25  2001/10/10 01:20:09  McClean
//  menue changed
//
//  Revision 1.24  2001/10/08 00:17:28  McClean
//  ucode-check - not rumnning
//
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>

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

#include "zapit/getservices.h"

#include "daemonc/remotecontrol.h"
#include <controldclient.h>

#include "helpers/infoviewer.h"
#include "helpers/epgdata.h"
#include "helpers/settings.h"
#include "helpers/streaminfo.h"
#include "helpers/locale.h"
#include "helpers/ucodecheck.h"

#include <string>
#include <vector>
#include <map>

using namespace std;


#define SA struct sockaddr
#define SAI struct sockaddr_in


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  main run-class                                             *
*                                                                                     *
**************************************************************************************/

class CNeutrinoApp : public CMenuTarget, COnPaintNotifier, CChangeObserver
{
	private:
		enum
		{
		    mode_tv = 1,
		    mode_radio = 2,
		    mode_scart = 3
		};


		struct streaming_commandhead
		{
			char version;
			char command;
		};

		//    EventList *eventlist;
		string				settingsFile;

		bool				nRun;
		int				    mode;
		bool				mute;
		bool				softupdate;
		int					streamstatus;

		channel_msg         firstchannel;
		st_rmsg				sendmessage;

		CColorSetupNotifier		*colorSetupNotifier;
		CAudioSetupNotifier		*audioSetupNotifier;
		CVideoSetupNotifier		*videoSetupNotifier;
		CLanguageSetupNotifier  *languageSetupNotifier;
		CKeySetupNotifier       *keySetupNotifier;
		CAPIDChangeExec         *APIDChanger;
		CNVODChangeExec         *NVODChanger;

		CChannelList		*channelList;

		void PluginDemo(); //demo only --- remove!

		void isCamValid();
		void firstChannel();
		void setupDefaults();
		void setupColors_classic();
		void setupColors_neutrino();
		void setupNetwork( bool force= false );

		void saveSetup();
		bool loadSetup(SNeutrinoSettings* load2=NULL);

		void tvMode();
		void radioMode();
		void scartMode();
		void setVolume(int key);
		void AudioMuteToggle();

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
		void InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier* videoSetupNotifier);
		void InitMainMenu(CMenuWidget &mainMenu, CMenuWidget &mainSettings, CMenuWidget &audioSettings, CMenuWidget &networkSettings,
		                  CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings, CMenuWidget &languageSettings, CMenuWidget &miscSettings, CMenuWidget &service);
		void ClearFrameBuffer();
		void SetupFonts();
		void SetupFrameBuffer();
		void SelectAPID();
		void SelectNVOD();
		void CmdParser(int argc, char **argv);


	public:

		CNeutrinoApp();
		~CNeutrinoApp();

		void channelsInit();
		int run(int argc, char **argv);
		//callback stuff only....
		int exec(CMenuTarget* parent, string actionKey);
		//callback for menue
		bool onPaintNotify(string MenuName);
		//onchange
		bool changeNotify(string OptionName);

};

#endif



