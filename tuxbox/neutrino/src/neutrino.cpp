/*

        $Id: neutrino.cpp,v 1.82 2001/11/23 17:09:22 McClean Exp $

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

  $Log: neutrino.cpp,v $
  Revision 1.82  2001/11/23 17:09:22  McClean
  dont know

  Revision 1.81  2001/11/23 16:58:41  McClean
  update-functions

  Revision 1.80  2001/11/23 13:48:35  faralla
  check if card fits camalpha.bin

  Revision 1.79  2001/11/23 13:47:37  faralla
  check if card fits camalpha.bin

  Revision 1.78  2001/11/20 23:23:10  Simplex
  Fix for unassigned bouquetList

  Revision 1.77  2001/11/19 22:53:33  Simplex
  Neutrino can handle bouquets now.
  There are surely some bugs and todo's but it works :)

  Revision 1.76  2001/11/05 16:04:25  field
  nvods/subchannels ver"c++"ed

  Revision 1.75  2001/11/03 22:22:43  McClean
  radiomode backgound paint fix

  Revision 1.74  2001/11/03 15:43:17  field
  Perspektiven

  Revision 1.73  2001/10/31 12:35:39  field
  sectionsd stoppen waehrend scan

  Revision 1.72  2001/10/30 22:18:09  McClean
  add ts-scan mask

  Revision 1.71  2001/10/29 16:49:00  field
  Kleinere Bug-Fixes (key-input usw.)

  Revision 1.70  2001/10/22 21:48:22  McClean
  design-update

  Revision 1.69  2001/10/22 15:00:18  McClean
  icon update

  Revision 1.68  2001/10/22 11:40:37  field
  nvod-zeitanzeige

  Revision 1.67  2001/10/21 13:06:17  field
  nvod-zeiten funktionieren!

  Revision 1.66  2001/10/15 17:27:19  field
  nvods (fast) implementiert (umschalten funkt noch nicht)

  Revision 1.64  2001/10/14 23:39:55  McClean
  VCR-Mode prepared

  Revision 1.63  2001/10/14 23:32:15  McClean
  menu structure - prepared for VCR-Switching

  Revision 1.62  2001/10/14 14:30:47  rasc
  -- EventList Darstellung ueberarbeitet
  -- kleiner Aenderungen und kleinere Bugfixes
  -- locales erweitert..

  Revision 1.61  2001/10/11 20:59:35  rasc
  clearbuffer() fuer RC-Input bei Start

  Revision 1.60  2001/10/10 17:17:13  field
  zappen auf onid_sid umgestellt

  Revision 1.59  2001/10/10 01:20:09  McClean
  menue changed

  Revision 1.58  2001/10/09 21:48:37  McClean
  ucode-check

  Revision 1.57  2001/10/07 13:16:57  McClean
  rgb/svideo/composite__4:3/16:9 switch

  Revision 1.56  2001/10/07 12:17:22  McClean
  video mode setup (pre)

  Revision 1.55  2001/10/04 23:21:13  McClean
  cleanup

  Revision 1.54  2001/10/04 23:16:20  McClean
  get volume from controld

  Revision 1.53  2001/10/04 19:28:43  fnbrd
  Eventlist benutzt ID bei zapit und laesst sich per rot wieder schliessen.

  Revision 1.52  2001/10/02 23:16:48  McClean
  game interface

  Revision 1.51  2001/10/02 17:56:33  McClean
  time in infobar (thread probs?) and "0" quickzap added

  Revision 1.50  2001/10/01 20:41:08  McClean
  plugin interface for games - beta but nice.. :)

  Revision 1.49  2001/09/27 11:25:38  field
  Numzap gefixt, kleinere Bugfixes

  Revision 1.48  2001/09/26 16:24:17  rasc
  - kleinere Aenderungen: Channel Num Zap fuer >999 Channels (Eutelsat/Astra) und eigener Font

  Revision 1.47  2001/09/26 11:40:48  field
  Tontraegerauswahl haut hin (bei Kanaelen mit EPG)

  Revision 1.45  2001/09/23 21:34:07  rasc
  - LIFObuffer Module, pushbackKey fuer RCInput,
  - In einige Helper und widget-Module eingebracht
    ==> harmonischeres Menuehandling
  - Infoviewer Breite fuer Channelsdiplay angepasst (>1000 Channels)

  Revision 1.44  2001/09/22 13:18:07  field
  epg-anzeige bug gefixt

  Revision 1.43  2001/09/20 19:21:37  fnbrd
  Channellist mit IDs.

  Revision 1.42  2001/09/20 17:02:16  field
  event-liste zeigt jetzt auch epgs an...

  Revision 1.41  2001/09/20 14:10:10  field
  neues EPG-Handling abschaltbar

  Revision 1.40  2001/09/20 00:36:32  field
  epg mit zaopit zum grossteil auf onid & s_id umgestellt

  Revision 1.39  2001/09/19 20:48:26  field
  Sprachauswahl funktioniert... (zapit updaten!)

  Revision 1.37  2001/09/18 20:20:26  field
  Eventlist in den Infov. verschoben (gelber Knopf), Infov.-Anzeige auf Knoepfe
  vorbereitet

  Revision 1.36  2001/09/18 14:57:51  field
  tzset eingebaut, id wird beim starten ausgegeben

  Revision 1.35  2001/09/18 11:34:42  fnbrd
  Some changes.

  Revision 1.34  2001/09/18 10:49:49  fnbrd
  Eventlist, quick'n dirty

  Revision 1.33  2001/09/17 23:57:50  McClean
  increase shutdown-logo-loadspeed

  Revision 1.32  2001/09/17 18:36:56  fnbrd
  Fixed use of unwanted globals ;)

  Revision 1.31  2001/09/17 16:02:35  field
  Keyblocker einstellbar, String(Nummern)-Input verbessert

  Revision 1.29  2001/09/17 01:07:44  McClean
  i18n selectable from menue - call make install - the .locale-files are needed..

  Revision 1.28  2001/09/16 03:38:44  McClean
  i18n + small other fixes

  Revision 1.27  2001/09/16 02:27:22  McClean
  make neutrino i18n

  Revision 1.26  2001/09/15 17:16:23  McClean
  i18n-module added

  Revision 1.25  2001/09/14 16:18:46  field
  Umstellung auf globale Variablen...

  Revision 1.24  2001/09/13 10:12:41  field
  Major update! Beschleunigtes zappen & EPG uvm...

  Revision 1.23  2001/09/09 23:53:46  fnbrd
  Fixed some bugs, only shown compiling with -Os.
  Conclusion: use -Os ;)

  Revision 1.22  2001/09/07 00:21:39  McClean
  spezial shutdown (lcd) fix for GeOrG :))

  Revision 1.21  2001/09/03 03:34:04  tw-74
  cosmetic fixes, own "Mg" fontmetrics

  Revision 1.20  2001/08/22 07:40:09  faralla
  works with zapit again

  Revision 1.19  2001/08/22 07:39:12  faralla
  works with zapit again

  Revision 1.18  2001/08/22 00:03:24  ge0rg
  verst„ndliche Fehlermeldungen

  Revision 1.17  2001/08/21 18:30:15  ge0rg
  added power down LCD logo

  Revision 1.16  2001/08/21 00:30:38  tw-74
  more fontrendering (see comments there), screen cosmetics

  Revision 1.15  2001/08/20 13:13:38  tw-74
  cosmetic changes and changes for variable font size

  Revision 1.14  2001/08/20 01:51:12  McClean
  channellist bug fixed - faster channellist response

  Revision 1.13  2001/08/20 01:26:54  McClean
  stream info added

  Revision 1.12  2001/08/18 12:50:29  tw-74
  cosmetic fixes (umlaute, tabbing, style)

  Revision 1.11  2001/08/18 12:32:06  McClean
  settings menue - critcal crash bug fixed

  Revision 1.10  2001/08/17 10:06:26  McClean
  cleanup - settings menue is broken!

  Revision 1.9  2001/08/16 23:19:18  McClean
  epg-view and quickview changed

  Revision 1.8  2001/08/15 17:10:02  fnbrd
  Channellist with events.


*/
#include "neutrino.h"
#include "include/debug.h"

#define NEUTRINO_CPP
#include "global.h"

// Globale Variablen - to use import global.h

// I don't like globals, I would have hidden them in classes,
// but if you wanna do it so... ;)

static void initGlobals(void)
{
  neutrino = NULL;

  g_FrameBuffer = NULL;
  g_fontRenderer = NULL;
  g_Fonts = NULL;

  g_RCInput = NULL;
  g_lcdd = NULL;
  g_Controld = NULL;
  g_RemoteControl = NULL;

  g_EpgData = NULL;
  g_InfoViewer = NULL;
  g_EventList = NULL;
  g_StreamInfo = NULL;
  g_ScreenSetup = NULL;
  g_UcodeCheck = NULL;

  g_Locale = NULL;

}
// Ende globale Variablen


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                                                     +
+          CNeutrinoApp - Constructor, initialize g_fontRenderer                        +
+                                                                                     +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
CNeutrinoApp::CNeutrinoApp()
{
    g_FrameBuffer = new CFrameBuffer;
	g_FrameBuffer->setIconBasePath("/usr/lib/icons/");

	g_fontRenderer = new fontRenderClass;
	SetupFrameBuffer();

	settingsFile = "/var/neutrino.conf";

	mode = 0;
	channelList = NULL;
	bouquetList = NULL;
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

void CNeutrinoApp::setupNetwork(bool force)
{
	if((g_settings.networkSetOnStartup) || (force))
	{
		printf("doing network setup...\n");
		//setup network
		setNetworkAddress(g_settings.network_ip, g_settings.network_netmask, g_settings.network_broadcast);
		setDefaultGateway(g_settings.network_defaultgateway);
        setNameServer(g_settings.network_nameserver);
	}
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
	g_settings.menu_Content_inactive_Text_red    = 0x1e;
	g_settings.menu_Content_inactive_Text_green  = 0x28;
	g_settings.menu_Content_inactive_Text_blue   = 0x3c;

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
*          CNeutrinoApp -  setupDefaults, set the application-defaults                *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupDefaults()
{
	//language
	strcpy(g_settings.language, "english");

	//misc
	g_settings.box_Type = 1;
	g_settings.epg_byname = 0;
    if ( !zapit )
        g_settings.epg_byname = 1;

	//video
	g_settings.video_Signal = 0; //composite?
	g_settings.video_Format = 2; //4:3
    g_settings.epg_byname   = 0;

	//audio
	g_settings.audio_Stereo = 1;
	g_settings.audio_DolbyDigital = 0;

	//colors
	setupColors_neutrino();

	//network
	g_settings.networkSetOnStartup = 0;
	strcpy(g_settings.network_ip, "192.168.40.10");
	strcpy(g_settings.network_netmask, "255.255.255.000");
	strcpy(g_settings.network_broadcast, "192.168.40.255");
	strcpy(g_settings.network_defaultgateway, "192.168.40.1");
	strcpy(g_settings.network_nameserver, "192.168.40.1");

	//key bindings
	g_settings.key_tvradio_mode = CRCInput::RC_nokey;
	g_settings.key_channelList_pageup = CRCInput::RC_red;
	g_settings.key_channelList_pagedown = CRCInput::RC_green;
	g_settings.key_channelList_cancel = CRCInput::RC_home;
	g_settings.key_quickzap_up = CRCInput::RC_up;
	g_settings.key_quickzap_down = CRCInput::RC_down;

    strcpy(g_settings.repeat_blocker, "0");

	//screen settings
	g_settings.screen_StartX=37;
	g_settings.screen_StartY=23;
	g_settings.screen_EndX=668;
	g_settings.screen_EndY=555;

	//Soft-Update
	g_settings.softupdate_mode=1; //internet

}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  loadSetup, load the application-settings                   *
*                                                                                     *
**************************************************************************************/
bool CNeutrinoApp::loadSetup()
{
	int fd;
	fd = open(settingsFile.c_str(), O_RDONLY );

	if (fd==-1)
	{
		printf("error while loading settings: %s\n", settingsFile.c_str() );
		return false;
	}
	if(read(fd, &g_settings, sizeof(g_settings))!=sizeof(g_settings))
	{
		printf("error while loading settings: %s - config from old version?\n", settingsFile.c_str() );
		return false;
	}
	close(fd);

    if ( !zapit )
        g_settings.epg_byname = 1;

	return true;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  saveSetup, save the application-settings                   *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::saveSetup()
{
	int fd;
	fd = open(settingsFile.c_str(), O_WRONLY | O_CREAT );

	if (fd==-1)
	{
		printf("error while saving settings: %s\n", settingsFile.c_str() );
		return;
	}
	write(fd, &g_settings,  sizeof(g_settings) );
	close(fd);
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  firstChannel, get the initial channel                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::firstChannel()
{
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";
	char *return_buf;

	sendmessage.version=1;
	sendmessage.cmd = 'a';

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(1505);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	#ifdef HAS_SIN_LEN
 		servaddr.sin_len = sizeof(servaddr); // needed ???
	#endif


	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
  		perror("neutrino: connect(zapit)");
		exit(-1);
	}

	write(sock_fd, &sendmessage, sizeof(sendmessage));
	return_buf = (char*) malloc(4);
	memset(return_buf, 0, 4);

	if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
		perror("recv(zapit)");
		exit(-1);
	}

//	printf("That was returned: %s\n", return_buf);

	if (strncmp(return_buf,"00a",3))
	{
		printf("Wrong Command was sent for firstChannel(). Exiting.\n");
		free(return_buf);
		return;
	}
	free(return_buf);


	memset(&firstchannel, 0, sizeof(firstchannel));
	if (recv(sock_fd, &firstchannel, sizeof(firstchannel),0) <= 0 ) {
		perror("Nothing could be received\n");
		exit(-1);
	}
	//firstchannel.chan_nr = ((firstchannel.chan_nr & 0x00ff) << 8) | ((firstchannel.chan_nr & 0xff00) >> 8);
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  isCamValid, check if card fits cam		              *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::isCamValid()
{

	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";
	char *return_buf;
	int ca_verid = 0;
	
	sendmessage.version=1;
	sendmessage.cmd = 't';

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(1505);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

	#ifdef HAS_SIN_LEN
 		servaddr.sin_len = sizeof(servaddr); // needed ???
	#endif


	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
  		perror("neutrino: connect(zapit)");
		exit(-1);
	}

	write(sock_fd, &sendmessage, sizeof(sendmessage));
	return_buf = (char*) malloc(4);
	memset(return_buf, 0, 4);

	if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
		perror("recv(zapit)");
		exit(-1);
	}

	printf("That was returned: %s\n", return_buf);

	if (strncmp(return_buf,"00t",3))
	{
		printf("Wrong Command was sent for isCamValid(). Exiting.\n");
		free(return_buf);
		return;
	}
	free(return_buf);
	
	if (recv(sock_fd, &ca_verid, sizeof(int),0) <= 0 ) {
		perror("Nothing could be received\n");
		exit(-1);
	}
	
	if (ca_verid != 33 && ca_verid != 18 && ca_verid != 68)
	{
		printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n!!\t\t\t\t\t\t\t!!\n!!\tATTENTION, YOUR CARD DOES NOT MATCH CAMALPHA.BIN!!\n!!\t\t\t\t\t\t\t!!\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}

}

		
/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit, get the Channellist from daemon              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit()
{
	if (zapit)
	{
 		int sock_fd;
		SAI servaddr;
		char rip[]="127.0.0.1";
		char *return_buf;
        channel_msg_2   zapitchannel;


        //deleting old channelList for mode-switching.
    	delete channelList;
    	channelList = new CChannelList(1, "channellist.head");

		sendmessage.version=1;
        // neu! war 5, mit neuem zapit holen wir uns auch die onid_tsid
		sendmessage.cmd = 'c';

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(1505);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		#ifdef HAS_SIN_LEN
 			servaddr.sin_len = sizeof(servaddr); // needed ???
		#endif


		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
			perror("neutrino: connect(zapit)");
			exit(-1);
		}

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		return_buf = (char*) malloc(4);
		memset(return_buf, 0, 4);
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}

//		printf("That was returned: %s\n", return_buf);

		if ( strcmp(return_buf, "00c")!= 0 )
		{
            free(return_buf);
			printf("Wrong Command was send for channelsInit(). Exiting.\n");
			return;
		}
        free(return_buf);
		memset(&zapitchannel,0,sizeof(zapitchannel));
		while (recv(sock_fd, &zapitchannel, sizeof(zapitchannel),0)>0)
        {
			char channel_name[30];
			strncpy(channel_name, zapitchannel.name,30);
			channelList->addChannel( zapitchannel.chan_nr, zapitchannel.chan_nr, channel_name, zapitchannel.onid_tsid );
		}
		printf("All channels received\n");
		close(sock_fd);

		bouquet_msg   zapitbouquet;
		//deleting old bouquetList for mode-switching.
		delete bouquetList;
		bouquetList = new CBouquetList(1, "bouquetlist.head");
		bouquetList->orgChannelList = channelList;

		sendmessage.version=1;
		sendmessage.cmd = 'q';

		sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		memset(&servaddr,0,sizeof(servaddr));
		servaddr.sin_family=AF_INET;
		servaddr.sin_port=htons(1505);
		inet_pton(AF_INET, rip, &servaddr.sin_addr);

		#ifdef HAS_SIN_LEN
 			servaddr.sin_len = sizeof(servaddr); // needed ???
		#endif


		if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
		{
			perror("neutrino: connect(zapit)");
			exit(-1);
		}

		write(sock_fd, &sendmessage, sizeof(sendmessage));
		return_buf = (char*) malloc(4);
		memset(return_buf, 0, 4);
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}
		if ( strcmp(return_buf, "00q")!= 0 )
		{
            free(return_buf);
			printf("Wrong Command was send for channelsInit(). Exiting.\n");
			return;
		}
        free(return_buf);

		printf("receiving bouquets...\n");

		uint nBouquetCount = 0;
		recv(sock_fd, &nBouquetCount, sizeof(&nBouquetCount),0);

		memset(&zapitbouquet,0,sizeof(zapitbouquet));
		for (uint i=0; i<nBouquetCount; i++)
		{
			recv(sock_fd, &zapitbouquet, sizeof(zapitbouquet),0);
			CBouquet* bouquet;
			char bouquet_name[30];

			strncpy(bouquet_name, zapitbouquet.name,30);
			bouquet = bouquetList->addBouquet( zapitbouquet.name );
//			printf("%s\n", zapitbouquet.name);
		}
		printf("All bouquets received (%d). Receiving channels... \n", nBouquetCount);
		close(sock_fd);

		for ( uint i=0; i< bouquetList->Bouquets.size(); i++ )
		{
			sendmessage.version=1;
			sendmessage.cmd = 'r';
			sendmessage.param = i+1;

			sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			memset(&servaddr,0,sizeof(servaddr));
			servaddr.sin_family=AF_INET;
			servaddr.sin_port=htons(1505);
			inet_pton(AF_INET, rip, &servaddr.sin_addr);

			#ifdef HAS_SIN_LEN
				servaddr.sin_len = sizeof(servaddr); // needed ???
			#endif


			if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
			{
				perror("neutrino: connect(zapit)");
				exit(-1);
			}

			write(sock_fd, &sendmessage, sizeof(sendmessage));
			return_buf = (char*) malloc(4);
			memset(return_buf, 0, 4);
			if (recv(sock_fd, return_buf, 3,0) <= 0 )
			{
				perror("recv(zapit)");
				exit(-1);
			}
			if ( strcmp(return_buf, "00r")!= 0 )
			{
				free(return_buf);
				printf("Wrong Command was send for channelsInit(). Exiting.\n");
				return;
			}
			free(return_buf);

			memset(&zapitchannel,0,sizeof(zapitchannel));
			printf("receiving channels for %s\n", bouquetList->Bouquets[i]->name.c_str());
			while (recv(sock_fd, &zapitchannel, sizeof(zapitchannel),0)>0)
			{
				char channel_name[30];
				strncpy(channel_name, zapitchannel.name,30);
				bouquetList->Bouquets[i]->channelList->addChannel(zapitchannel.chan_nr, zapitchannel.chan_nr, channel_name, zapitchannel.onid_tsid);
			}
			close(sock_fd);
		}
		printf("All bouquets-channels received\n");

	}
	else
	{

		char buffer[128];
		FILE *fp=popen("pzap --dump", "r");

		if (!fp)
		{
			perror("pzap --dump");
			return;
		}

		int count = 0;
		while ( fgets(buffer, 128, fp) )
		{
			if (buffer[strlen(buffer)-1]=='\n')
				buffer[strlen(buffer)-1]=0;
			char longname[100];
			char shortname[100];
			strcpy(shortname, buffer+7);
			strcpy(longname, buffer+7);
			channelList->addChannel(count, count+1, longname);
			count++;
	  	}
		pclose(fp);
	}
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  run, the main runloop                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::CmdParser(int argc, char **argv)
{
 	zapit = false;

	for(int x=1; x<argc; x++)
	{
		if (! strcmp(argv[x], "-z")) {
			printf("Using zapit\n");
			zapit = true;
		}
		else if (! strcmp(argv[x], "-su"))
		{
			printf("Software update enabled\n");
			softupdate = true;
		}
		else {
			printf("Usage: neutrino [-z] [-su]\n");
			exit(0);
		}
	}
}

void CNeutrinoApp::SetupFrameBuffer()
{
	if (g_FrameBuffer->setMode(720, 576, 8))
	{
		printf("Error while setting framebuffer mode\n");
		exit(-1);
	}

	//make 1..8 transparent for dummy painting
	for(int count =0;count<8;count++)
		g_FrameBuffer->paletteSetColor(count, 0x000000, 0xffff);
	g_FrameBuffer->paletteSet();
}

void CNeutrinoApp::SetupFonts()
{
    g_Fonts->menu =         g_fontRenderer->getFont("Arial", "Regular", 20);
    g_Fonts->menu_title =   g_fontRenderer->getFont("Arial", "Regular", 30);
    g_Fonts->menu_info =    g_fontRenderer->getFont("Arial", "Regular", 16);

    g_Fonts->epg_title =    g_fontRenderer->getFont("Arial", "Regular", 30);

	g_Fonts->epg_info1=g_fontRenderer->getFont("Arial", "Italic", 17); // info1 must be same size as info2, but italic
	g_Fonts->epg_info2=g_fontRenderer->getFont("Arial", "Regular", 17);

	g_Fonts->epg_date=g_fontRenderer->getFont("Arial", "Regular", 15);
	g_Fonts->alert=g_fontRenderer->getFont("Arial", "Regular", 100);

      // -- Fonts for eventlist (why this not in Eventlist Class?)
	g_Fonts->eventlist_title=g_fontRenderer->getFont("Arial", "Regular", 30);
	g_Fonts->eventlist_itemLarge=g_fontRenderer->getFont("Arial", "Bold", 20);
	g_Fonts->eventlist_itemSmall=g_fontRenderer->getFont("Arial", "Regular", 14);
//	g_Fonts->eventlist_datetime=g_fontRenderer->getFont("Arial", "Italic", 16);
	g_Fonts->eventlist_datetime=g_fontRenderer->getFont("Arial", "Regular", 16);

	g_Fonts->channellist=g_fontRenderer->getFont("Arial", "Regular", 20);
	g_Fonts->channellist_number=g_fontRenderer->getFont("Arial", "Regular", 14);
	g_Fonts->channel_num_zap=g_fontRenderer->getFont("Arial", "Regular", 40);

	g_Fonts->infobar_number=g_fontRenderer->getFont("Arial", "Regular", 50);
	g_Fonts->infobar_channame=g_fontRenderer->getFont("Arial", "Regular", 30);
	g_Fonts->infobar_info=g_fontRenderer->getFont("Arial", "Regular", 20);
    g_Fonts->infobar_small=g_fontRenderer->getFont("Arial", "Regular", 14);
//    g_Fonts->info_symbols = g_fontRenderer->getFont("Marlett", "Regular", 18);
//    g_Fonts->info_symbols = g_fontRenderer->getFont("Arial", "Regular", 18);

	g_Fonts->fixedabr20=g_fontRenderer->getFont("Arial Black", "Regular", 20);
}

void CNeutrinoApp::ClearFrameBuffer()
{
	memset(g_FrameBuffer->lfb, 255, g_FrameBuffer->Stride()*576);

	//backgroundmode
	g_FrameBuffer->setBackgroundColor(COL_BACKGROUND);
	g_FrameBuffer->useBackground(false);

	//background
	g_FrameBuffer->paletteSetColor(COL_BACKGROUND, 0x000000, 0xffff);
	//Windows Colors
	g_FrameBuffer->paletteSetColor(0x0, 0x010101, 0);
	g_FrameBuffer->paletteSetColor(0x1, 0x800000, 0);
	g_FrameBuffer->paletteSetColor(0x2, 0x008000, 0);
	g_FrameBuffer->paletteSetColor(0x3, 0x808000, 0);
	g_FrameBuffer->paletteSetColor(0x4, 0x000080, 0);
	g_FrameBuffer->paletteSetColor(0x5, 0x800080, 0);
	g_FrameBuffer->paletteSetColor(0x6, 0x008080, 0);
//	frameBuffer.paletteSetColor(0x7, 0xC0C0C0, 0);
	g_FrameBuffer->paletteSetColor(0x7, 0xA0A0A0, 0);

//	frameBuffer.paletteSetColor(0x8, 0x808080, 0);
	g_FrameBuffer->paletteSetColor(0x8, 0x505050, 0);

	g_FrameBuffer->paletteSetColor(0x9, 0xFF0000, 0);
	g_FrameBuffer->paletteSetColor(0xA, 0x00FF00, 0);
	g_FrameBuffer->paletteSetColor(0xB, 0xFFFF00, 0);
	g_FrameBuffer->paletteSetColor(0xC, 0x0000FF, 0);
	g_FrameBuffer->paletteSetColor(0xD, 0xFF00FF, 0);
	g_FrameBuffer->paletteSetColor(0xE, 0x00FFFF, 0);
	g_FrameBuffer->paletteSetColor(0xF, 0xFFFFFF, 0);

	g_FrameBuffer->paletteSet();
}

void CNeutrinoApp::InitMainMenu(CMenuWidget &mainMenu, CMenuWidget &mainSettings,  CMenuWidget &audioSettings, CMenuWidget &networkSettings,
				     CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings, CMenuWidget &languageSettings, CMenuWidget &miscSettings)
{
	mainMenu.addItem( new CMenuSeparator() );
	mainMenu.addItem( new CMenuForwarder("mainmenu.tvmode", true, "", this, "tv"), true );
	mainMenu.addItem( new CMenuForwarder("mainmenu.radiomode", (zapit), "", this, "radio") );
	mainMenu.addItem( new CMenuForwarder("mainmenu.vcrmode", false, "", this, "vcr") );
	mainMenu.addItem( new CMenuForwarder("mainmenu.games", true, "", new CGameList("mainmenu.games") ));
	mainMenu.addItem( new CMenuForwarder("mainmenu.shutdown", true, "", this, "shutdown") );
	mainMenu.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	mainMenu.addItem( new CMenuForwarder("mainmenu.settings", true, "", &mainSettings) );

	mainSettings.addItem( new CMenuSeparator() );
	mainSettings.addItem( new CMenuForwarder("menu.back") );
	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.scants", (zapit), "", g_ScanTS) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.video", true, "", &videoSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.audio", true, "", &audioSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.network", true, "", &networkSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.language", true, "", &languageSettings ) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.colors", true,"", &colorSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.keybinding", true,"", &keySettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.misc", true, "", &miscSettings ) );
	if (softupdate)
	{
		printf("init soft-update-stuff\n");
		CMenuWidget* updateSettings = new CMenuWidget("mainsettings.update", "mainmenue.raw");
		updateSettings->addItem( new CMenuSeparator() );
		updateSettings->addItem( new CMenuForwarder("menu.back") );
		updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		
		//get current flash-version
		FILE* fd = fopen("/var/etc/version", "r");
		strcpy(g_settings.softupdate_currentversion, "1.0.0");

		if(!fd)
		{
			perror("cannot read flash-versioninfo");
		}
		else
		{
			if(fgets(g_settings.softupdate_currentversion,90,fd)==NULL)
			fclose(fd);
			for (unsigned int x=0;x<strlen(g_settings.softupdate_currentversion);x++)
			{
				if( (g_settings.softupdate_currentversion[x]!='.') && ((g_settings.softupdate_currentversion[x]>'9') || (g_settings.softupdate_currentversion[x]<'0') ) )
				{
					g_settings.softupdate_currentversion[x]=0;
				}
			}
		}
		printf("current flash-version: %s\n", g_settings.softupdate_currentversion);

		updateSettings->addItem( new CMenuForwarder("flashupdate.currentversion", false, (char*) &g_settings.softupdate_currentversion, NULL ));

		CMenuOptionChooser *oj = new CMenuOptionChooser("flashupdate.updatemode", &g_settings.softupdate_mode,true);
			oj->addOption(0, "flashupdate.updatemode_manual");
			oj->addOption(1, "flashupdate.updatemode_internet");
		updateSettings->addItem( oj );
		updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		updateSettings->addItem( new CMenuForwarder("flashupdate.checkupdate", true, "", g_Update ) );		

		mainSettings.addItem( new CMenuForwarder("mainsettings.update", true, "", updateSettings ) );		
		printf("ready - soft-update-stuff\n");
	}

}


void CNeutrinoApp::InitMiscSettings(CMenuWidget &miscSettings)
{
	miscSettings.addItem( new CMenuSeparator() );
	miscSettings.addItem( new CMenuForwarder("menu.back") );
	miscSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuOptionChooser *oj = new CMenuOptionChooser("miscsettings.epgold", &g_settings.epg_byname, zapit);
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
	miscSettings.addItem( oj );

	oj = new CMenuOptionChooser("miscsettings.boxtype", &g_settings.box_Type, true);
		oj->addOption(1, "miscsettings.boxtype_nokia");
		oj->addOption(2, "miscsettings.boxtype_sagem");
		oj->addOption(3, "miscsettings.boxtype_philips");
	miscSettings.addItem( oj );

	miscSettings.addItem( new CMenuForwarder("miscsettings.ucodecheck", true, "", g_UcodeCheck ) );
//	miscSettings.addItem( new CMenuForwarder("miscsettings.reload_services", true, "", g_ReloadServices ) );
}


void CNeutrinoApp::InitLanguageSettings(CMenuWidget &languageSettings)
{
	languageSettings.addItem( new CMenuSeparator() );
	languageSettings.addItem( new CMenuForwarder("menu.back") );
	languageSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

    languageSetupNotifier = new CLanguageSetupNotifier;
	CMenuOptionStringChooser* oj = new CMenuOptionStringChooser("languagesetup.select", (char*)&g_settings.language, true, languageSetupNotifier, false);
		//search available languages....

		struct dirent **namelist;
		int n;
//		printf("scanning locale dir now....(perhaps)\n");

		n = scandir("/usr/lib/locale", &namelist, 0, alphasort);
		if (n < 0)
		{
			perror("scandir");
			//should be available...
			oj->addOption( "english" );
		}
		else
		{
			for(int count=0;count<n;count++)
			{
				string filen = namelist[count]->d_name;
				int pos = filen.find(".locale");
				if(pos!=-1)
				{
					string locale = filen.substr(0,pos);
//					printf("locale found: %s\n", locale.c_str() );
					oj->addOption( locale );
				}
				free(namelist[count]);
			}
			free(namelist);
		}
	languageSettings.addItem( oj );
}

void CNeutrinoApp::InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier* audioSetupNotifier)
{
	audioSettings.addItem( new CMenuSeparator() );
	audioSettings.addItem( new CMenuForwarder("menu.back") );
	audioSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuOptionChooser* oj = new CMenuOptionChooser("audiomenu.stereo", &g_settings.audio_Stereo, true, audioSetupNotifier);
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
	audioSettings.addItem( oj );
		oj = new CMenuOptionChooser("audiomenu.dolbydigital", &g_settings.audio_DolbyDigital, true, audioSetupNotifier);
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
	audioSettings.addItem( oj );
}

void CNeutrinoApp::InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier* videoSetupNotifier)
{
	videoSettings.addItem( new CMenuSeparator() );
	videoSettings.addItem( new CMenuForwarder("menu.back") );
	videoSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	videoSettings.addItem( new CMenuForwarder("videomenu.screensetup", true, "", g_ScreenSetup ) );

	CMenuOptionChooser* oj = new CMenuOptionChooser("videomenu.videosignal", &g_settings.video_Signal, true, videoSetupNotifier);
		oj->addOption(1, "videomenu.videosignal_rgb");
		oj->addOption(2, "videomenu.videosignal_svideo");
		oj->addOption(0, "videomenu.videosignal_composite");

	videoSettings.addItem( oj );

	oj = new CMenuOptionChooser("videomenu.videoformat", &g_settings.video_Format, true, videoSetupNotifier);
		oj->addOption(2, "videomenu.videoformat_43");
		oj->addOption(1, "videomenu.videoformat_169");

	videoSettings.addItem( oj );
}

void CNeutrinoApp::InitNetworkSettings(CMenuWidget &networkSettings)
{
	networkSettings.addItem( new CMenuSeparator() );
	networkSettings.addItem( new CMenuForwarder("menu.back") );
	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	networkSettings.addItem( new CMenuForwarder("networkmenu.setupnow", true, "", this, "network") );

	CMenuOptionChooser* oj = new CMenuOptionChooser("networkmenu.setuponstartup", &g_settings.networkSetOnStartup, true);
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");

	networkSettings.addItem( oj );

	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CStringInput*	networkSettings_NetworkIP= new CStringInput("networkmenu.ipaddress", g_settings.network_ip, 3*4+3, "ipsetup.hint_1", "ipsetup.hint_2");
	CStringInput*	networkSettings_NetMask= new CStringInput("networkmenu.netmask", g_settings.network_netmask, 3*4+3, "ipsetup.hint_1", "ipsetup.hint_2");
	CStringInput*	networkSettings_Broadcast= new CStringInput("networkmenu.broadcast", g_settings.network_broadcast, 3*4+3, "ipsetup.hint_1", "ipsetup.hint_2");
	CStringInput*	networkSettings_Gateway= new CStringInput("networkmenu.gateway", g_settings.network_defaultgateway, 3*4+3, "ipsetup.hint_1", "ipsetup.hint_2");
	CStringInput*	networkSettings_NameServer= new CStringInput("networkmenu.nameserver", g_settings.network_nameserver, 3*4+3, "ipsetup.hint_1", "ipsetup.hint_2");

	networkSettings.addItem( new CMenuForwarder("networkmenu.ipaddress", true, g_settings.network_ip, networkSettings_NetworkIP ));
	networkSettings.addItem( new CMenuForwarder("networkmenu.netmask", true, g_settings.network_netmask, networkSettings_NetMask ));
	networkSettings.addItem( new CMenuForwarder("networkmenu.broadcast", true, g_settings.network_broadcast, networkSettings_Broadcast ));

	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	networkSettings.addItem( new CMenuForwarder("networkmenu.gateway", true, g_settings.network_defaultgateway, networkSettings_Gateway ));
	networkSettings.addItem( new CMenuForwarder("networkmenu.nameserver", true, g_settings.network_nameserver, networkSettings_NameServer ));
}

void CNeutrinoApp::InitColorSettings(CMenuWidget &colorSettings)
{
	colorSettings.addItem( new CMenuSeparator() );
	colorSettings.addItem( new CMenuForwarder("menu.back") );
	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
}

void CNeutrinoApp::InitColorThemesSettings(CMenuWidget &audioSettings_Themes)
{
	audioSettings_Themes.addItem( new CMenuSeparator() );
	audioSettings_Themes.addItem( new CMenuForwarder("menu.back") );
	audioSettings_Themes.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	audioSettings_Themes.addItem( new CMenuForwarder("colorthememenu.neutrino_theme", true, "", this, "theme_neutrino") );
	audioSettings_Themes.addItem( new CMenuForwarder("colorthememenu.classic_theme", true, "", this, "theme_classic") );
}

void CNeutrinoApp::InitColorSettingsMenuColors(CMenuWidget &colorSettings_menuColors, CMenuWidget &colorSettings)
{
	colorSettings_menuColors.addItem( new CMenuSeparator() );
	colorSettings_menuColors.addItem( new CMenuForwarder("menu.back") );

    CColorChooser* chHeadcolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue,
					&g_settings.menu_Head_alpha, colorSetupNotifier);
    CColorChooser* chHeadTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Head_Text_red, &g_settings.menu_Head_Text_green, &g_settings.menu_Head_Text_blue,
					NULL, colorSetupNotifier);
    CColorChooser* chContentcolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Content_red, &g_settings.menu_Content_green, &g_settings.menu_Content_blue,
					&g_settings.menu_Content_alpha, colorSetupNotifier);
    CColorChooser* chContentTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Content_Text_red, &g_settings.menu_Content_Text_green, &g_settings.menu_Content_Text_blue,
					NULL, colorSetupNotifier);
    CColorChooser* chContentSelectedcolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Content_Selected_red, &g_settings.menu_Content_Selected_green, &g_settings.menu_Content_Selected_blue,
					&g_settings.menu_Content_Selected_alpha, colorSetupNotifier);
    CColorChooser* chContentSelectedTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Content_Selected_Text_red, &g_settings.menu_Content_Selected_Text_green, &g_settings.menu_Content_Selected_Text_blue,
					NULL, colorSetupNotifier);
    CColorChooser* chContentInactivecolor = new CColorChooser("colormenu.background_head", &g_settings.menu_Content_inactive_red, &g_settings.menu_Content_inactive_green, &g_settings.menu_Content_inactive_blue,
					&g_settings.menu_Content_inactive_alpha, colorSetupNotifier);
    CColorChooser* chContentInactiveTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.menu_Content_inactive_Text_red, &g_settings.menu_Content_inactive_Text_green, &g_settings.menu_Content_inactive_Text_blue,
					NULL, colorSetupNotifier);
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menuhead") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, "", chHeadcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, "", chHeadTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menucontent") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, "", chContentcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, "", chContentTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menucontent_inactive") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, "", chContentInactivecolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, "", chContentInactiveTextcolor));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colormenusetup.menucontent_selected") );
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.background", true, "", chContentSelectedcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("colormenu.textcolor", true, "", chContentSelectedTextcolor ));

	colorSettings.addItem( new CMenuForwarder("colormenu.menucolors", true, "", &colorSettings_menuColors) );
}

void CNeutrinoApp::InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_statusbarColors, CMenuWidget &colorSettings)
{
	colorSettings_statusbarColors.addItem( new CMenuSeparator() );

	colorSettings_statusbarColors.addItem( new CMenuForwarder("menu.back") );

	CColorChooser* chInfobarcolor = new CColorChooser("colormenu.background_head", &g_settings.infobar_red, &g_settings.infobar_green, &g_settings.infobar_blue,
					&g_settings.infobar_alpha, colorSetupNotifier);
	CColorChooser* chInfobarTextcolor = new CColorChooser("colormenu.textcolor_head", &g_settings.infobar_Text_red, &g_settings.infobar_Text_green, &g_settings.infobar_Text_blue,
					NULL, colorSetupNotifier);

	colorSettings_statusbarColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "colorstatusbar.text") );
	colorSettings_statusbarColors.addItem( new CMenuForwarder("colormenu.background", true, "", chInfobarcolor ));
	colorSettings_statusbarColors.addItem( new CMenuForwarder("colormenu.textcolor", true, "", chInfobarTextcolor ));
	colorSettings.addItem( new CMenuForwarder("colorstatusbar.head", true, "", &colorSettings_statusbarColors) );

}

void CNeutrinoApp::InitKeySettings(CMenuWidget &keySettings)
{
	keySettings.addItem( new CMenuSeparator() );

	keySettings.addItem( new CMenuForwarder("menu.back") );

    keySetupNotifier = new CKeySetupNotifier;
	CStringInput*	keySettings_repeatBlocker= new CStringInput("keybindingmenu.repeatblock", g_settings.repeat_blocker, 3, "repeatblocker.hint_1", "repeatblocker.hint_2", "0123456789 ", keySetupNotifier);
    keySetupNotifier->changeNotify("initial");

	CKeyChooser*	keySettings_tvradio_mode = new CKeyChooser(&g_settings.key_tvradio_mode, "keybindingmenu.tvradiomode_head", "settings.raw");
	CKeyChooser*	keySettings_channelList_pageup = new CKeyChooser(&g_settings.key_channelList_pageup, "keybindingmenu.pageup_head", "settings.raw");
	CKeyChooser*	keySettings_channelList_pagedown = new CKeyChooser(&g_settings.key_channelList_pagedown, "keybindingmenu.pagedown_head", "settings.raw");
	CKeyChooser*	keySettings_channelList_cancel = new CKeyChooser(&g_settings.key_channelList_cancel, "keybindingmenu.cancel_head", "settings.raw");
	CKeyChooser*	keySettings_quickzap_up = new CKeyChooser(&g_settings.key_quickzap_up, "keybindingmenu.channelup_head",   "settings.raw");
	CKeyChooser*	keySettings_quickzap_down = new CKeyChooser(&g_settings.key_quickzap_down, "keybindingmenu.channeldown_head", "settings.raw");


    keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.RC") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.repeatblock", true, "", keySettings_repeatBlocker ));

	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.modechange") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.tvradiomode", true, "", keySettings_tvradio_mode ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.channellist") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.pageup", true, "", keySettings_channelList_pageup ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.pagedown", true, "", keySettings_channelList_pagedown ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.cancel", true, "", keySettings_channelList_cancel ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.quickzap") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.channelup", true, "", keySettings_quickzap_up ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.channeldown", true, "", keySettings_quickzap_down ));

}

static char* copyStringto(const char* from, char* to, int len, char delim)
{
	const char *fromend=from+len;
	while(*from!=delim && from<fromend && *from)
	{
		*(to++)=*(from++);
	}
	*to=0;
	return (char *)++from;
}

void CNeutrinoApp::SelectNVOD()
{
    CSubChannel_Infos subChannels= g_RemoteControl->getSubChannels();

    if ( subChannels.has_subChannels_for( channelList->getActiveChannelID() ) )
    {
        // NVOD/SubService- Kanal!
      	CMenuWidget NVODSelector( subChannels.are_subchannels?"nvodselector.subservice":"nvodselector.head",
                                  "video.raw", 400 );

        NVODSelector.addItem( new CMenuSeparator() );

        for(unsigned count=0;count<subChannels.list.size();count++)
        {
            char nvod_id[5];
            sprintf(nvod_id, "%d", count);

            if (!subChannels.are_subchannels)
            {
                char nvod_time_a[50], nvod_time_e[50], nvod_time_x[50];
                char nvod_s[100];
                struct  tm *tmZeit;

                tmZeit= localtime(&subChannels.list[count].startzeit);
                sprintf(nvod_time_a, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

                time_t endtime=subChannels.list[count].startzeit+ subChannels.list[count].dauer;
                tmZeit= localtime(&endtime);
                sprintf(nvod_time_e, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

                time_t jetzt=time(NULL);
                if (subChannels.list[count].startzeit > jetzt)
                {
                    int mins=(subChannels.list[count].startzeit- jetzt)/ 60;
                    sprintf(nvod_time_x, g_Locale->getText("nvod.starting").c_str(), mins);
                }
                else
                if ( (subChannels.list[count].startzeit<= jetzt) && (jetzt < endtime) )
                {
                    int proz=(jetzt- subChannels.list[count].startzeit)*100/subChannels.list[count].dauer;
                    sprintf(nvod_time_x, g_Locale->getText("nvod.proz").c_str(), proz);
                }
                else
                    nvod_time_x[0]= 0;

                //string nvod_s= nvod_time_a+ " - "+ nvod_time_e+ " "+ nvod_time_x;
                sprintf(nvod_s, "%s - %s %s", nvod_time_a, nvod_time_e, nvod_time_x);
                NVODSelector.addItem( new CMenuForwarder(nvod_s, true, "", NVODChanger, nvod_id, false), (count == subChannels.selected) );
            }
            else
            {
                NVODSelector.addItem( new CMenuForwarder(subChannels.list[count].subservice_name, true, "", NVODChanger, nvod_id, false), (count == subChannels.selected) );
            }
        }
        NVODSelector.exec(NULL, "");
    }
}

void CNeutrinoApp::SelectAPID()
{
    g_RemoteControl->CopyAPIDs();

    char to_compare[50];
    if ( g_settings.epg_byname == 0 )
        snprintf( to_compare, 10, "%x", channelList->getActiveChannelOnid_sid() );
    else
        strcpy( to_compare, channelList->getActiveChannelName().c_str() );

    if ( ( strcmp(g_RemoteControl->audio_chans.name, to_compare )== 0 ) &&
         ( g_RemoteControl->audio_chans.count_apids> 1 ) )
    {
        // wir haben APIDs für diesen Kanal!

    	CMenuWidget APIDSelector("apidselector.head", "audio.raw", 300);
        APIDSelector.addItem( new CMenuSeparator() );

//        APIDSelector.addItem( new CMenuSeparator(CMenuSeparator::STRING, "apidselector.hint") );
//        APIDSelector.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

//        APIDSelector.addItem( new CMenuForwarder("menu.back") );
//	    APIDSelector.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

        bool has_unresolved_ctags= false;
        for(int count=0;count<g_RemoteControl->audio_chans.count_apids;count++)
        {
            if ( g_RemoteControl->audio_chans.apids[count].ctag != -1 )
                has_unresolved_ctags= true;

            if ( strlen( g_RemoteControl->audio_chans.apids[count].name ) == 3 )
            {
                // unaufgel÷ste Sprache...
                strcpy( g_RemoteControl->audio_chans.apids[count].name, getISO639Description( g_RemoteControl->audio_chans.apids[count].name ) );
            }

            if ( g_RemoteControl->audio_chans.apids[count].is_ac3 )
                strcat(g_RemoteControl->audio_chans.apids[count].name, " (AC3)");
        }

        unsigned int onid_tsid = channelList->getActiveChannelOnid_sid();

        if ( has_unresolved_ctags && ( onid_tsid != 0 ) )
        {

            int sock_fd;
        	SAI servaddr;
        	char rip[]="127.0.0.1";
        	bool retval = false;

        	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        	memset(&servaddr,0,sizeof(servaddr));
        	servaddr.sin_family=AF_INET;
        	servaddr.sin_port=htons(sectionsd::portNumber);
        	inet_pton(AF_INET, rip, &servaddr.sin_addr);

        	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
        	{
        		perror("Couldn't connect to server!");
        	}
            else
            {
                sectionsd::msgRequestHeader req;
                req.version = 2;
                req.command = sectionsd::CurrentComponentTagsChannelID;
                req.dataLength = 4;
                write(sock_fd,&req,sizeof(req));

                write(sock_fd, &onid_tsid, sizeof(onid_tsid));
                printf("query ComponentTags for onid_tsid >%x<\n", onid_tsid );

                sectionsd::msgResponseHeader resp;
                memset(&resp, 0, sizeof(resp));

                read(sock_fd, &resp, sizeof(sectionsd::msgResponseHeader));

                int nBufSize = resp.dataLength;
                if(nBufSize>0)
                {
                    char TagIDs[10];
                    char TagText[100];
                    unsigned char componentTag, componentType, streamContent;

                    char* pData = new char[nBufSize+1] ;
                    read(sock_fd, pData, nBufSize);

                    char *actPos=pData;

                    while(*actPos && actPos<pData+resp.dataLength)
                    {
                        *TagIDs=0;
                        actPos = copyStringto( actPos, TagIDs, sizeof(TagIDs), '\n');
                        *TagText=0;
                        actPos = copyStringto( actPos, TagText, sizeof(TagText), '\n');


                        sscanf(TagIDs, "%02hhx %02hhx %02hhx", &componentTag, &componentType, &streamContent);
                        // printf("%s - %d - %s\n", TagIDs, componentTag, TagText);

                        for(int count=0;count<g_RemoteControl->audio_chans.count_apids;count++)
                        {
                            if ( g_RemoteControl->audio_chans.apids[count].ctag == componentTag )
                            {
                                strcpy(g_RemoteControl->audio_chans.apids[count].name, TagText);
                                if ( g_RemoteControl->audio_chans.apids[count].is_ac3 )
                                    strcat(g_RemoteControl->audio_chans.apids[count].name, " (AC3)");
                                g_RemoteControl->audio_chans.apids[count].ctag = -1;
                                break;
                            }
                        }
                    }

                    delete[] pData;
                    retval = true;
                }
                close(sock_fd);
            }
        }

        for(int count=0;count<g_RemoteControl->audio_chans.count_apids;count++)
        {
            char apid[5];
            sprintf(apid, "%d", count);
            APIDSelector.addItem( new CMenuForwarder(g_RemoteControl->audio_chans.apids[count].name, true, "", APIDChanger, apid, false), (count == g_RemoteControl->audio_chans.selected) );
        }
        APIDSelector.exec(NULL, "");
    }
}

void CNeutrinoApp::InitZapper()
{
	g_RemoteControl->setZapper(zapit);

	if (!zapit)
		channelsInit();

	g_InfoViewer->start();
	g_EpgData->start();

	if (zapit)
	{
		isCamValid();
		firstChannel();
		if (firstchannel.mode == 't')
		{
			//remoteControl.tvMode();
			tvMode();
		}
		else
		{
			//remoteControl.radioMode();
			radioMode();
		}
	}
	if (!zapit)
    {
		channelList->zapTo(0);
        mode = mode_tv;
    }
	if (bouquetList!=NULL)
		bouquetList->adjustToChannel( channelList->getActiveChannelNumber());
}

void CNeutrinoApp::RealRun(CMenuWidget &mainMenu)
{
    // display volume...
    char volume = g_Controld->getVolume();
    g_Controld->setVolume(volume);

	while(nRun)
	{
		int key = g_RCInput->getKey();

		if (key==CRCInput::RC_setup)
		{
			mainMenu.exec(NULL, "");
		}
		else if (key==CRCInput::RC_standby)
		{
			nRun=false;
		}

		if ((mode==mode_tv) || ((mode==mode_radio) && (zapit)) )
		{
			if (key==CRCInput::RC_ok)
			{	//channellist
//				channelList->exec();
//				BouquetSwitchMode bouqMode = bsmBouquets;
				BouquetSwitchMode bouqMode = bsmChannels;
//				BouquetSwitchMode bouqMode = bsmAllChannels;
//				TODO: get Value from options, whether to show bouquetlist
//				or channellist

				if ((bouquetList!=NULL) && (bouquetList->Bouquets.size() == 0 ))
				{
					printf("bouquets are empty\n");
					bouqMode = bsmAllChannels;
                }
				if ((bouquetList!=NULL) && (bouqMode == bsmBouquets))
				{
					bouquetList->exec(true);
				}
				else if ((bouquetList!=NULL) && (bouqMode == bsmChannels))
				{
					int nNewChannel = bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->show();
					if (nNewChannel>-1)
					{
						channelList->zapTo(bouquetList->Bouquets[bouquetList->getActiveBouquetNumber()]->channelList->getKey(nNewChannel)-1);
					}
				}
				else
				{
					printf("using all channels\n");
					channelList->exec();
				}
			}
			else if ((key==CRCInput::RC_right) && (bouquetList!=NULL))
			{
				if (bouquetList->Bouquets.size() > 0)
				{
					int nNext = (bouquetList->getActiveBouquetNumber()+1) % bouquetList->Bouquets.size();
					bouquetList->activateBouquet(nNext);
					if ( bouquetList->showChannelList())
						bouquetList->adjustToChannel( channelList->getActiveChannelNumber());

				}
			}
			else if ((key==CRCInput::RC_left) && (bouquetList!=NULL))
			{
				if (bouquetList->Bouquets.size() > 0)
				{
					int nNext = (bouquetList->getActiveBouquetNumber()+bouquetList->Bouquets.size()-1) % bouquetList->Bouquets.size();
					bouquetList->activateBouquet(nNext);
					if ( bouquetList->showChannelList())
						bouquetList->adjustToChannel( channelList->getActiveChannelNumber());
				}
			}
			else if (key==CRCInput::RC_red)
			{	// eventlist
                g_InfoViewer->killTitle();
                g_EventList->exec(channelList->getActiveChannelOnid_sid(), channelList->getActiveChannelName());
			}
			else if (key==CRCInput::RC_blue)
			{	// streaminfo
                g_StreamInfo->exec(NULL, "");
			}
            else if (key==CRCInput::RC_green)
			{	// APID
                SelectAPID();
			}
            else if (key==CRCInput::RC_yellow)
			{	// NVODs
                SelectNVOD();
			}
			else if ((key==g_settings.key_quickzap_up) || (key==g_settings.key_quickzap_down))
			{
				//quickzap
				channelList->quickZap( key );
				if (bouquetList!=NULL)
					bouquetList->adjustToChannel( channelList->getActiveChannelNumber());
			}
			else if (key==CRCInput::RC_help)
			{	//epg
				if ( g_InfoViewer->is_visible )
				{
                    g_InfoViewer->killTitle();
					g_EpgData->show( channelList->getActiveChannelName(),
                                     channelList->getActiveChannelOnid_sid() );
				}
				else
				{
					g_InfoViewer->showTitle( channelList->getActiveChannelNumber(),
					                         channelList->getActiveChannelName(),
                                             channelList->getActiveChannelOnid_sid(),
                                             false
                                            );
				}
			}
			else if ((key>=0) && (key<=9))
			{ //numeric zap
				channelList->numericZap( key );
				if (bouquetList!=NULL)
					bouquetList->adjustToChannel( channelList->getActiveChannelNumber());
			}
			else if (key==CRCInput::RC_spkr)
			{	//mute
				AudioMuteToggle();
			}
			else if ((key==CRCInput::RC_plus) || (key==CRCInput::RC_minus))
			{	//volume
				setVolume( key );
			}
		}
	}
}

void CNeutrinoApp::ExitRun()
{
	saveSetup();
	printf("neutrino exit\n");
	//shutdown screen

	//memset(frameBuffer.lfb, 255, frameBuffer.Stride()*576);
	for(int x=0;x<256;x++)
		g_FrameBuffer->paletteSetColor(x, 0x000000, 0xffff);

	g_FrameBuffer->paletteSet();

	g_FrameBuffer->loadPicture2Mem("shutdown.raw", g_FrameBuffer->lfb );
//	g_FrameBuffer->paintIcon8("shutdown.raw",0,0);
	g_FrameBuffer->loadPal("shutdown.pal");

	g_Controld->shutdown();
	sleep(55555);
}

int CNeutrinoApp::run(int argc, char **argv)
{
	CmdParser(argc, argv);

	if(!loadSetup())
	{
		//setup default if configfile not exists
		setupDefaults();
		printf("using defaults...\n\n");
	}

    g_Fonts = new FontsDef;
	SetupFonts();

	ClearFrameBuffer();

	g_Locale = new CLocaleManager;
    g_RCInput = new CRCInput;
    g_lcdd = new CLCDD;
    g_Controld = new CControld;
    g_RemoteControl = new CRemoteControl;
    g_EpgData = new CEpgData;
    g_InfoViewer = new CInfoViewer;
    g_StreamInfo = new CStreamInfo;
	g_ScanTS = new CScanTs;
	g_UcodeCheck = new CUCodeCheck;
    g_ScreenSetup = new CScreenSetup;
    g_EventList = new EventList;
	g_Update = new CFlashUpdate;


//    printf("\nCNeutrinoApp::run - objects initialized...\n\n");
	g_Locale->loadLocale(g_settings.language);

	colorSetupNotifier = new CColorSetupNotifier;
	audioSetupNotifier = new CAudioSetupNotifier;
	videoSetupNotifier = new CVideoSetupNotifier;
	APIDChanger        = new CAPIDChangeExec;
	NVODChanger        = new CNVODChangeExec;

    colorSetupNotifier->changeNotify("initial");

	setupNetwork();

	channelList = new CChannelList( 1, "channellist.head" );


	//Main settings
	CMenuWidget mainMenu("mainmenu.head", "mainmenue.raw");
	CMenuWidget mainSettings("mainsettings.head", "settings.raw");
	CMenuWidget languageSettings("languagesetup.head", "language.raw");
	CMenuWidget videoSettings("videomenu.head", "video.raw");
	CMenuWidget audioSettings("audiomenu.head", "audio.raw");
	CMenuWidget networkSettings("networkmenu.head", "network.raw");
	CMenuWidget colorSettings("colormenu.head", "colors.raw");
	CMenuWidget keySettings("keybindingmenu.head", "keybinding.raw");
	CMenuWidget miscSettings("miscsettings.head", "settings.raw");

	InitMainMenu(mainMenu, mainSettings, audioSettings, networkSettings, colorSettings, keySettings, videoSettings, languageSettings, miscSettings);

	//language Setup
	InitLanguageSettings(languageSettings);

	//misc Setup
	InitMiscSettings(miscSettings);

	//audio Setup
	InitAudioSettings(audioSettings, audioSetupNotifier);

	//video Setup
	InitVideoSettings(videoSettings, videoSetupNotifier);

	//network Setup
	InitNetworkSettings(networkSettings);

	//color Setup
	InitColorSettings(colorSettings);

	CMenuWidget colorSettings_Themes("colorthememenu.head", "settings.raw");
	InitColorThemesSettings(colorSettings_Themes);

	// Hacking Shit
	colorSettings.addItem( new CMenuForwarder("colormenu.themeselect", true, "", &colorSettings_Themes) );
	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	// Meno

   	CMenuWidget colorSettings_menuColors("colormenusetup.head", "settings.raw");
	InitColorSettingsMenuColors(colorSettings_menuColors, colorSettings);

	CMenuWidget colorSettings_statusbarColors("colormenu.statusbar", "settings.raw");
	InitColorSettingsStatusBarColors(colorSettings_statusbarColors, colorSettings);

	//keySettings
	InitKeySettings(keySettings);

	//init programm
	InitZapper();

    mute = false;
	nRun = true;

	RealRun(mainMenu);

	ExitRun();
	return 0;
}

void CNeutrinoApp::AudioMuteToggle()
{
	int dx = 40;
	int dy = 40;
	int x = g_settings.screen_EndX-dx;
	int y = g_settings.screen_StartY;
    if ( !mute )
    {
    	g_FrameBuffer->paintBoxRel(x, y, dx, dy, COL_INFOBAR);
    	g_FrameBuffer->paintIcon("mute.raw", x+5, y+5);
    	g_Controld->Mute();
    }
    else
    {
        g_FrameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
    	g_Controld->UnMute();
    }
	mute = !mute;
}

void CNeutrinoApp::setVolume(int key)
{
	int dx = 256;
	int dy = 40;
	int x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
	int y = g_settings.screen_EndY- 100;

	g_FrameBuffer->paintIcon("volume.raw",x,y, COL_INFOBAR);

	char volume = g_Controld->getVolume();

    do
	{
        if (key==CRCInput::RC_plus)
		{
			if (volume<100)
			{
				volume += 5;
			}
		}
		else if (key==CRCInput::RC_minus)
		{
			if (volume>0)
			{
				volume -= 5;
			}
		}
        else
        {
            if (key!=CRCInput::RC_ok)
              g_RCInput->pushbackKey(key);

            key= CRCInput::RC_timeout;
        }

		g_Controld->setVolume(volume);

		int vol = volume<<1;
		g_FrameBuffer->paintBoxRel(x+40, y+12, 200, 15, COL_INFOBAR+1);
		g_FrameBuffer->paintBoxRel(x+40, y+12, vol, 15, COL_INFOBAR+3);

        if ( key != CRCInput::RC_timeout )
        {
    		key = g_RCInput->getKey(30);
        }

	} while ( key != CRCInput::RC_timeout );

	g_FrameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
}

void CNeutrinoApp::tvMode()
{
	if(mode==mode_tv)
	{
		return;
	}
	mode = mode_tv;

	memset(g_FrameBuffer->lfb, 255, g_FrameBuffer->Stride()*576);
	g_FrameBuffer->useBackground(false);

	if (zapit)
	{
		g_RemoteControl->tvMode();
		channelsInit();
		channelList->zapTo( firstchannel.chan_nr -1 );
	}
}

void CNeutrinoApp::radioMode()
{
	if(mode==mode_radio)
	{
		return;
	}
	mode = mode_radio;

	g_FrameBuffer->loadPal("dboxradio.pal", 18, 199);
	g_FrameBuffer->paintIcon8("dboxradio.raw",0,0, 18);
	g_FrameBuffer->loadBackground("dboxradio.raw", 18);
	g_FrameBuffer->useBackground(true);

	if (zapit)
	{
		firstChannel();
		g_RemoteControl->radioMode();
		channelsInit();
//		bouquetList->activateBouquet(0,false);
		channelList->zapTo( 0 );
	}
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  exec, menuitem callback (shutdown)                         *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::exec( CMenuTarget* parent, string actionKey )
{
//	printf("ac: %s\n", actionKey.c_str());
	int returnval = CMenuTarget::RETURN_REPAINT;

	if(actionKey=="theme_neutrino")
	{
		setupColors_neutrino();
		colorSetupNotifier->changeNotify("initial");
	}
	else if(actionKey=="theme_classic")
	{
		setupColors_classic();
		colorSetupNotifier->changeNotify("initial");
	}
	else if(actionKey=="shutdown")
	{
		nRun=false;
		returnval = CMenuTarget::RETURN_EXIT_ALL;
	}
	else if(actionKey=="tv")
	{
		tvMode();
		returnval = CMenuTarget::RETURN_EXIT_ALL;
	}
	else if(actionKey=="radio")
	{
		radioMode();
		returnval = CMenuTarget::RETURN_EXIT_ALL;
	}
	else if(actionKey=="network")
	{
		setupNetwork( true );
	}

	return returnval;
}



/**************************************************************************************
*                                                                                     *
*          Main programm - no function here                                           *
*                                                                                     *
**************************************************************************************/
int main(int argc, char **argv)
{
    printf("NeutrinoNG $Id: neutrino.cpp,v 1.82 2001/11/23 17:09:22 McClean Exp $\n\n");
    tzset();
    initGlobals();
	neutrino = new CNeutrinoApp;
	return neutrino->run(argc, argv);
}

