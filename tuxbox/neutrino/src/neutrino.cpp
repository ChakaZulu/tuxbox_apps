/*

        $Id: neutrino.cpp,v 1.27 2001/09/16 02:27:22 McClean Exp $

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

  $Log: neutrino.cpp,v $
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
  verständliche Fehlermeldungen

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

//global variablen

SNeutrinoSettings   g_settings;

FontsDef        *g_Fonts = NULL;
CFrameBuffer    *g_FrameBuffer = NULL;
CLocaleManager	*g_Locale = NULL;
CRCInput        *g_RCInput = NULL;
CLCDD           *g_lcdd = NULL;
CControld       *g_Controld = NULL;
CRemoteControl  *g_RemoteControl = NULL;

CEpgData        *g_EpgData = NULL;
CInfoViewer     *g_InfoViewer = NULL;
CStreamInfo     *g_StreamInfo = NULL;
CScreenSetup    *g_ScreenSetup = NULL;


class CColorSetupNotifier : public CChangeObserver
{
	public:
		CColorSetupNotifier() {};

		void changeNotify(string OptionName)
		{
			//setting colors-..
			g_FrameBuffer->paletteGenFade(COL_MENUHEAD,
				convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue),
				convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue),
				8, convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ) );
			
			g_FrameBuffer->paletteGenFade(COL_MENUCONTENT,
				convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue),
				convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
				8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

			g_FrameBuffer->paletteGenFade(COL_MENUCONTENTSELECTED,
				convertSetupColor2RGB(g_settings.menu_Content_Selected_red, g_settings.menu_Content_Selected_green, g_settings.menu_Content_Selected_blue),
				convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue),
				8, convertSetupAlpha2Alpha(g_settings.menu_Content_Selected_alpha) );

			g_FrameBuffer->paletteGenFade(COL_MENUCONTENTINACTIVE,
				convertSetupColor2RGB(g_settings.menu_Content_inactive_red, g_settings.menu_Content_inactive_green, g_settings.menu_Content_inactive_blue),
				convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue),
				8, convertSetupAlpha2Alpha(g_settings.menu_Content_inactive_alpha) );

			g_FrameBuffer->paletteGenFade(COL_INFOBAR,
				convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue),
				convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
				8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

			g_FrameBuffer->paletteSetColor( COL_INFOBAR_SHADOW,
							convertSetupColor2RGB(
								int(g_settings.infobar_red*0.4),
								int(g_settings.infobar_green*0.4),
								int(g_settings.infobar_blue*0.4)),
                                    g_settings.infobar_alpha);

			g_FrameBuffer->paletteSet();
		}
};

class CAudioSetupNotifier : public CChangeObserver
{
	void changeNotify(string OptionName)
	{
		printf("notify: %s\n", OptionName.c_str() );
	};
};

class CVideoSetupNotifier : public CChangeObserver
{
	void changeNotify(string OptionName)
	{
		printf("notify: %s\n", OptionName.c_str() );
	};
};

void setNetworkAddress(char* ip, char* netmask, char* broadcast)
{
	printf("IP       : %s\n", ip);
	printf("Netmask  : %s\n", netmask);
	printf("Broadcast: %s\n", broadcast);
	if(fork()==0)
	{
		if (execlp("ifconfig", "ifconfig", "eth0", "up", ip, "netmask", netmask, "broadcast", broadcast, 0)<0)
		{
			perror("exec failed - ifconfig\n");
		}
	}
}

void setDefaultGateway(char* ip)
{
	if(fork()==0)
	{
	    if (execlp("route", "route", "add", "-net", "default", "gw", ip, 0)<0)
		{
			perror("exec failed - route\n");
		}
	}
}

class CNetworkSetupNotifier : public CChangeObserver
{
	public:
//		CNetworkSetupNotifier(){}

		void changeNotify(string OptionName)
		{
			printf("notify: %s\n", OptionName.c_str() );
			if( (g_settings.networkSetOnStartup) && (OptionName=="initial"))
			{
				printf("doing network setup...\n");
				//setup network
				setNetworkAddress(g_settings.network_ip, g_settings.network_netmask, g_settings.network_broadcast);
				setDefaultGateway(g_settings.network_defaultgateway);

				FILE* fd = fopen("/etc/resolv.conf", "w");
				if(fd)
				{
					fprintf(fd, "# resolv.conf - generated by neutrino\n\n");
					fprintf(fd, "nameserver %s\n", g_settings.network_nameserver);
					fclose(fd);
				}
				else
				{
					perror("cannot write /etc/resolv.conf");
				}
			}
		}
};

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                                                     +
+          CNeutrinoApp - Constructor, initialize fontrenderer                        +
+                                                                                     +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
CNeutrinoApp::CNeutrinoApp()
{
	nRun = true;
	channelList = NULL;
	fontRenderer = NULL;

    g_FrameBuffer = new CFrameBuffer;
	g_FrameBuffer->setIconBasePath("/usr/lib/icons/");
	settingsFile = "/var/neutrino.conf";
	mode = mode_tv;
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
	if (fontRenderer)
		delete fontRenderer;
}

void CNeutrinoApp::setupNetwork(bool force)
{
	if((g_settings.networkSetOnStartup) || (force))
	{
		printf("doing network setup...\n");
		//setup network
		setNetworkAddress(g_settings.network_ip, g_settings.network_netmask, g_settings.network_broadcast);
		setDefaultGateway(g_settings.network_defaultgateway);

		FILE* fd = fopen("/etc/resolv.conf", "w");
		if(fd)
		{
			fprintf(fd, "# resolv.conf - generated by neutrino\n\n");
			fprintf(fd, "nameserver %s\n", g_settings.network_nameserver);
			fclose(fd);
		}
		else
		{
			perror("cannot write /etc/resolv.conf");
		}
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
	g_settings.menu_Head_Text_green = 0x41;
	g_settings.menu_Head_Text_blue  = 0x00;

	g_settings.menu_Content_alpha = 0x14;
	g_settings.menu_Content_red   = 0x00;
	g_settings.menu_Content_green = 0x14;
	g_settings.menu_Content_blue  = 0x23;

	g_settings.menu_Content_Text_alpha = 0x00;
	g_settings.menu_Content_Text_red   = 0x64;
	g_settings.menu_Content_Text_green = 0x64;
	g_settings.menu_Content_Text_blue  = 0x64;

	g_settings.menu_Content_Selected_alpha = 0x14;
	g_settings.menu_Content_Selected_red   = 0x19;
	g_settings.menu_Content_Selected_green = 0x3c;
	g_settings.menu_Content_Selected_blue  = 0x64;

	g_settings.menu_Content_Selected_Text_alpha  = 0x00;
	g_settings.menu_Content_Selected_Text_red    = 0x00;
	g_settings.menu_Content_Selected_Text_green  = 0x00;
	g_settings.menu_Content_Selected_Text_blue   = 0x00;

	g_settings.menu_Content_inactive_alpha = 0x14;
	g_settings.menu_Content_inactive_red   = 0x00;
	g_settings.menu_Content_inactive_green = 0x14;
	g_settings.menu_Content_inactive_blue  = 0x23;

	g_settings.menu_Content_inactive_Text_alpha  = 0x00;
	g_settings.menu_Content_inactive_Text_red    = 0x1e;
	g_settings.menu_Content_inactive_Text_green  = 0x28;
	g_settings.menu_Content_inactive_Text_blue   = 0x3c;

	g_settings.infobar_alpha = 0x14;
	g_settings.infobar_red   = 0x00;
	g_settings.infobar_green = 0x14;
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
	//video
	g_settings.video_Signal = 0;
	g_settings.video_Format = 0;

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

	//screen settings
	g_settings.screen_StartX=37;
	g_settings.screen_StartY=23;
	g_settings.screen_EndX=668;
	g_settings.screen_EndY=555;
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
	
	printf("That was returned: %s\n", return_buf);
	
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

//deleting old channelList for mode-switching.	
	delete channelList;
	channelList = new CChannelList(1, g_Locale->getText("channellist.head"));
	
		
		sendmessage.version=1;
		sendmessage.cmd = 5;

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
	
		if (atoi(return_buf) != 5)
		{
         		free(return_buf);
			printf("Wrong Command was send for channelsInit(). Exiting.\n");
			return;
		}
	        free(return_buf);
		memset(&zapitchannel,0,sizeof(zapitchannel));
		while (recv(sock_fd, &zapitchannel, sizeof(zapitchannel),0)>0) {
			char channel_name[30];
			uint channel_nr = zapitchannel.chan_nr;
			strncpy(channel_name,zapitchannel.name,30);
		
			//printf("Name received: %s\n", channel_name);
			//printf("Channelnumber received: %d\n", channel_nr);
			channelList->addChannel(channel_nr, channel_nr, channel_name);
			memset(&zapitchannel,0,sizeof(zapitchannel));
		}
		printf("All channels received\n");
			
		close(sock_fd);
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
	if (argc > 1) {
		if (! strcmp(argv[1], "-z")) {
			printf("Using zapit\n");
			zapit = true;
		}
		else {
			printf("Usage: neutrino [-z]\n");
			exit(0);
		}
	}
	else {
		printf("Using nstreamzapd\n");
	 	zapit = false;
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
	g_Fonts->menu=fontRenderer->getFont("Arial", "Regular", 20); // was "Arial" "Bold" 20
	g_Fonts->menu->RenderString( 10, 100, 500, "DEMO!", 0 );
	g_Fonts->menu_title=fontRenderer->getFont("Arial", "Regular", 30); // was: "Arial Black", "Regular", 30
	                                                              // but this font has wrong metric! (getHeight())
	g_Fonts->menu_title->RenderString( 10, 100, 500, "DEMO!", 0 );
	g_Fonts->epg_title=fontRenderer->getFont("Arial", "Regular", 30);
	g_Fonts->epg_title->RenderString( 10, 100, 500, "DEMO!", 0 );
	
	g_Fonts->epg_info1=fontRenderer->getFont("Arial", "Italic", 17); // info1 must be same size as info2, but italic
	g_Fonts->epg_info1->RenderString( 10, 100, 500, "DEMO!", 0 );
	g_Fonts->epg_info2=fontRenderer->getFont("Arial", "Regular", 17);
	g_Fonts->epg_info2->RenderString( 10, 100, 500, "DEMO!", 0 );

	g_Fonts->epg_date=fontRenderer->getFont("Arial", "Regular", 15);
	g_Fonts->epg_date->RenderString( 10, 100, 500, "DEMO!", 0 );
	g_Fonts->alert=fontRenderer->getFont("Arial", "Regular", 100);

	g_Fonts->channellist=fontRenderer->getFont("Arial", "Regular", 20);
	g_Fonts->channellist->RenderString( 10, 100, 500, "DEMO!", 0 );
	g_Fonts->channellist_number=fontRenderer->getFont("Arial", "Regular", 14);
	g_Fonts->channellist_number->RenderString( 10, 100, 500, "DEMO!", 0);
	
	g_Fonts->infobar_number=fontRenderer->getFont("Arial", "Regular", 50);
	g_Fonts->infobar_number->RenderString( 10, 100, 500, "DEMO!", 0 );
	g_Fonts->infobar_channame=fontRenderer->getFont("Arial", "Regular", 30);
	g_Fonts->infobar_channame->RenderString( 10, 100, 500, "DEMO!", 0 );
	g_Fonts->infobar_info=fontRenderer->getFont("Arial", "Regular", 20);
	g_Fonts->infobar_info->RenderString( 10, 100, 500, "DEMO!", 0 );

    g_Fonts->infobar_small=fontRenderer->getFont("Arial", "Regular", 14);
	g_Fonts->infobar_small->RenderString( 10, 100, 500, "DEMO!", 0 );

	g_Fonts->fixedabr20=fontRenderer->getFont("Arial Black", "Regular", 20);
	g_Fonts->fixedabr20->RenderString( 10, 100, 500, "DEMO!", 0 );
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

void CNeutrinoApp::InitMainSettings(CMenuWidget &mainSettings, CMenuWidget &audioSettings, CMenuWidget &networkSettings,
				     CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings)
{
	mainSettings.addItem( new CMenuSeparator() );
	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("mainmenu.runmode")) );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.shutdown"), true, "", this, "shutdown") );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.tvmode"), true, "", this, "tv"), true );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.radiomode"), (zapit), "", this, "radio") );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.mp3player"), false, "", this, "mp3") );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.splayback"), false, "", this, "playback") );

	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING,g_Locale->getText("mainmenu.info")) );

	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.streaminfo"), true, "", g_StreamInfo ) );

	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("mainmenu.settings")) );
	mainSettings.addItem( new CMenuForwarder("Video", true, "", &videoSettings) );

	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.screensetup"), true, "", g_ScreenSetup ) );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.audio"), true, "", &audioSettings) );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.network"), true, "", &networkSettings) );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.colors"), true,"", &colorSettings) );
	mainSettings.addItem( new CMenuForwarder(g_Locale->getText("mainmenu.keybinding"), true,"", &keySettings) );
}

void CNeutrinoApp::InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier &audioSetupNotifier)
{
	audioSettings.addItem( new CMenuSeparator() );
	audioSettings.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );
	audioSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CMenuOptionChooser* oj = new CMenuOptionChooser(g_Locale->getText("audiomenu.stereo"), &g_settings.audio_Stereo, true, &audioSetupNotifier);
		oj->addOption(0, g_Locale->getText("options.off"));
		oj->addOption(1, g_Locale->getText("options.on"));
	audioSettings.addItem( oj );
		oj = new CMenuOptionChooser(g_Locale->getText("audiomenu.dolbydigital"), &g_settings.audio_DolbyDigital, true, &audioSetupNotifier);
		oj->addOption(0, g_Locale->getText("options.off"));
		oj->addOption(1, g_Locale->getText("options.on"));
	audioSettings.addItem( oj );
}

void CNeutrinoApp::InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier &videoSetupNotifier)
{
	videoSettings.addItem( new CMenuSeparator() );
	videoSettings.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );
	videoSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CMenuOptionChooser* oj = new CMenuOptionChooser(g_Locale->getText("videomenu.videosignal"), &g_settings.video_Signal, true, &videoSetupNotifier);
		oj->addOption(0, g_Locale->getText("videomenu.videosignal_rgb"));
		oj->addOption(1, g_Locale->getText("videomenu.videosignal_svideo"));
		oj->addOption(2, g_Locale->getText("videomenu.videosignal_composite"));
	videoSettings.addItem( oj );
		oj = new CMenuOptionChooser(g_Locale->getText("videomenu.videoformat"), &g_settings.video_Format, true, &videoSetupNotifier);
		oj->addOption(0, g_Locale->getText("videomenu.videoformat_43"));
		oj->addOption(1, g_Locale->getText("videomenu.videoformat_169"));
	videoSettings.addItem( oj );
}

void CNeutrinoApp::InitNetworkSettings(CMenuWidget &networkSettings, CNetworkSetupNotifier &networkSetupNotifier)
{
	networkSettings.addItem( new CMenuSeparator() );
	networkSettings.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );
	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	networkSettings.addItem( new CMenuForwarder(g_Locale->getText("networkmenu.setupnow"), true, "", this, "network") );

	CMenuOptionChooser* oj = new CMenuOptionChooser(g_Locale->getText("networkmenu.setuponstartup"), &g_settings.networkSetOnStartup, true, &networkSetupNotifier);
		oj->addOption(0, g_Locale->getText("options.off"));
		oj->addOption(1, g_Locale->getText("options.on"));
	networkSettings.addItem( oj );	

	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CStringInput*	networkSettings_NetworkIP= new CStringInput(g_Locale->getText("networkmenu.ipaddress"), g_settings.network_ip, 3*4+3);
		CStringInput*	networkSettings_NetMask= new CStringInput(g_Locale->getText("networkmenu.netmask"), g_settings.network_netmask, 3*4+3);
		CStringInput*	networkSettings_Broadcast= new CStringInput(g_Locale->getText("networkmenu.broadcast"), g_settings.network_broadcast, 3*4+3);
		CStringInput*	networkSettings_Gateway= new CStringInput(g_Locale->getText("networkmenu.gateway"), g_settings.network_defaultgateway, 3*4+3);
		CStringInput*	networkSettings_NameServer= new CStringInput(g_Locale->getText("networkmenu.nameserver"), g_settings.network_nameserver, 3*4+3);
	networkSettings.addItem( new CMenuForwarder(g_Locale->getText("networkmenu.ipaddress"), true, g_settings.network_ip, networkSettings_NetworkIP ));
	networkSettings.addItem( new CMenuForwarder(g_Locale->getText("networkmenu.netmask"), true, g_settings.network_netmask, networkSettings_NetMask ));
	networkSettings.addItem( new CMenuForwarder(g_Locale->getText("networkmenu.broadcast"), true, g_settings.network_broadcast, networkSettings_Broadcast ));
	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	networkSettings.addItem( new CMenuForwarder(g_Locale->getText("networkmenu.gateway"), true, g_settings.network_defaultgateway, networkSettings_Gateway ));
	networkSettings.addItem( new CMenuForwarder(g_Locale->getText("networkmenu.nameserver"), true, g_settings.network_nameserver, networkSettings_NameServer ));
}

void CNeutrinoApp::InitColorSettings(CMenuWidget &colorSettings)
{
	colorSettings.addItem( new CMenuSeparator() );
	colorSettings.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );
	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
}

void CNeutrinoApp::InitColorThemesSettings(CMenuWidget &audioSettings_Themes)
{
	audioSettings_Themes.addItem( new CMenuSeparator() );
	audioSettings_Themes.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );
	audioSettings_Themes.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	audioSettings_Themes.addItem( new CMenuForwarder(g_Locale->getText("colorthememenu.neutrino_theme"), true, "", this, "theme_neutrino") );
	audioSettings_Themes.addItem( new CMenuForwarder(g_Locale->getText("colorthememenu.classic_theme"), true, "", this, "theme_classic") );
}

void CNeutrinoApp::InitColorSettingsMenuColors(CMenuWidget &colorSettings_menuColors, CMenuWidget &colorSettings)
{
	colorSettings_menuColors.addItem( new CMenuSeparator() );
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );

    CColorChooser* chHeadcolor = new CColorChooser(g_Locale->getText("colormenu.background_head"), &g_settings.menu_Head_red, &g_settings.menu_Head_green, &g_settings.menu_Head_blue,
					&g_settings.menu_Head_alpha, colorSetupNotifier);
    CColorChooser* chHeadTextcolor = new CColorChooser(g_Locale->getText("colormenu.textcolor_head"), &g_settings.menu_Head_Text_red, &g_settings.menu_Head_Text_green, &g_settings.menu_Head_Text_blue,
					NULL, colorSetupNotifier);
    CColorChooser* chContentcolor = new CColorChooser(g_Locale->getText("colormenu.background_head"), &g_settings.menu_Content_red, &g_settings.menu_Content_green, &g_settings.menu_Content_blue,
					&g_settings.menu_Content_alpha, colorSetupNotifier);
    CColorChooser* chContentTextcolor = new CColorChooser(g_Locale->getText("colormenu.textcolor_head"), &g_settings.menu_Content_Text_red, &g_settings.menu_Content_Text_green, &g_settings.menu_Content_Text_blue,
					NULL, colorSetupNotifier);
    CColorChooser* chContentSelectedcolor = new CColorChooser(g_Locale->getText("colormenu.background_head"), &g_settings.menu_Content_Selected_red, &g_settings.menu_Content_Selected_green, &g_settings.menu_Content_Selected_blue,
					&g_settings.menu_Content_Selected_alpha, colorSetupNotifier);
    CColorChooser* chContentSelectedTextcolor = new CColorChooser(g_Locale->getText("colormenu.textcolor_head"), &g_settings.menu_Content_Selected_Text_red, &g_settings.menu_Content_Selected_Text_green, &g_settings.menu_Content_Selected_Text_blue,
					NULL, colorSetupNotifier);
    CColorChooser* chContentInactivecolor = new CColorChooser(g_Locale->getText("colormenu.background_head"), &g_settings.menu_Content_inactive_red, &g_settings.menu_Content_inactive_green, &g_settings.menu_Content_inactive_blue,
					&g_settings.menu_Content_inactive_alpha, colorSetupNotifier);
    CColorChooser* chContentInactiveTextcolor = new CColorChooser(g_Locale->getText("colormenu.textcolor_head"), &g_settings.menu_Content_inactive_Text_red, &g_settings.menu_Content_inactive_Text_green, &g_settings.menu_Content_inactive_Text_blue,
					NULL, colorSetupNotifier);
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("colormenusetup.menuhead")) );
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.background"), true, "", chHeadcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.textcolor"), true, "", chHeadTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("colormenusetup.menucontent")) );
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.background"), true, "", chContentcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.textcolor"), true, "", chContentTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("colormenusetup.menucontent_inactive")) );
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.background"), true, "", chContentInactivecolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.textcolor"), true, "", chContentInactiveTextcolor));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("colormenusetup.menucontent_selected")) );
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.background"), true, "", chContentSelectedcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.textcolor"), true, "", chContentSelectedTextcolor ));

	colorSettings.addItem( new CMenuForwarder(g_Locale->getText("colormenu.menucolors"), true, "", &colorSettings_menuColors) );
}

void CNeutrinoApp::InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_statusbarColors, CMenuWidget &colorSettings)
{
	colorSettings_statusbarColors.addItem( new CMenuSeparator() );
	colorSettings_statusbarColors.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );
			CColorChooser* chInfobarcolor = new CColorChooser(g_Locale->getText("colormenu.background_head"), &g_settings.infobar_red, &g_settings.infobar_green, &g_settings.infobar_blue,
					&g_settings.infobar_alpha, colorSetupNotifier);
			CColorChooser* chInfobarTextcolor = new CColorChooser(g_Locale->getText("colormenu.textcolor_head"), &g_settings.infobar_Text_red, &g_settings.infobar_Text_green, &g_settings.infobar_Text_blue,
					NULL, colorSetupNotifier);
	colorSettings_statusbarColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("colorstatusbar.text")) );
	colorSettings_statusbarColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.background"), true, "", chInfobarcolor ));
	colorSettings_statusbarColors.addItem( new CMenuForwarder(g_Locale->getText("colormenu.textcolor"), true, "", chInfobarTextcolor ));
	colorSettings.addItem( new CMenuForwarder(g_Locale->getText("colorstatusbar.head"), true, "", &colorSettings_statusbarColors) );
}

void CNeutrinoApp::InitKeySettings(CMenuWidget &keySettings)
{
	keySettings.addItem( new CMenuSeparator() );
	keySettings.addItem( new CMenuForwarder(g_Locale->getText("menu.back")) );
		CKeyChooser*	keySettings_tvradio_mode = new CKeyChooser(&g_settings.key_tvradio_mode, g_Locale->getText("keybindingmenu.tvradiomode_head"), "settings.raw");
		CKeyChooser*	keySettings_channelList_pageup = new CKeyChooser(&g_settings.key_channelList_pageup, g_Locale->getText("keybindingmenu.pageup_head"), "settings.raw");
		CKeyChooser*	keySettings_channelList_pagedown = new CKeyChooser(&g_settings.key_channelList_pagedown, g_Locale->getText("keybindingmenu.pagedown_head"), "settings.raw");
		CKeyChooser*	keySettings_channelList_cancel = new CKeyChooser(&g_settings.key_channelList_cancel, g_Locale->getText("keybindingmenu.cancel_head"), "settings.raw");
		CKeyChooser*	keySettings_quickzap_up = new CKeyChooser(&g_settings.key_quickzap_up, g_Locale->getText("keybindingmenu.channelup_head"),   "settings.raw");
		CKeyChooser*	keySettings_quickzap_down = new CKeyChooser(&g_settings.key_quickzap_down, g_Locale->getText("keybindingmenu.channeldown_head"), "settings.raw");
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("keybindingmenu.modechange")) );
	keySettings.addItem( new CMenuForwarder(g_Locale->getText("keybindingmenu.tvradiomode"), true, "", keySettings_tvradio_mode ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("keybindingmenu.channellist")) );
	keySettings.addItem( new CMenuForwarder(g_Locale->getText("keybindingmenu.pageup"), true, "", keySettings_channelList_pageup ));
	keySettings.addItem( new CMenuForwarder(g_Locale->getText("keybindingmenu.pagedown"), true, "", keySettings_channelList_pagedown ));
	keySettings.addItem( new CMenuForwarder(g_Locale->getText("keybindingmenu.cancel"), true, "", keySettings_channelList_cancel ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, g_Locale->getText("keybindingmenu.quickzap")) );
	keySettings.addItem( new CMenuForwarder(g_Locale->getText("keybindingmenu.channelup"), true, "", keySettings_quickzap_up ));
	keySettings.addItem( new CMenuForwarder(g_Locale->getText("keybindingmenu.channeldown"), true, "", keySettings_quickzap_down ));
}

void CNeutrinoApp::InitZapper()
{
	g_RemoteControl->setZapper(zapit);
	volume = 100;
	if (!zapit)
		channelsInit();
		
	g_InfoViewer->start();
	g_EpgData->start();
		
	if (zapit) 
	{
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
		channelList->zapTo(0);
}

void CNeutrinoApp::RealRun(CMenuWidget &mainSettings)
{
	mute = false;
	while(nRun)
	{
		int key = g_RCInput->getKey();

		if (key==CRCInput::RC_setup)
		{
			mainSettings.exec(NULL, "");
		}
		else if (key==CRCInput::RC_standby)
		{
			nRun=false;
		}

		if ((mode==mode_tv) || ((mode==mode_radio) && (zapit)) )
		{
			if (key==CRCInput::RC_ok)
			{	//channellist
				channelList->exec();
			}
			else if ((key==g_settings.key_quickzap_up) || (key==g_settings.key_quickzap_down))
			{
				//quickzap
				channelList->quickZap( key );
			}
			else if (key==CRCInput::RC_help)
			{	//epg
				if ( g_InfoViewer->is_visible )
				{
                    g_InfoViewer->killTitle();
					g_EpgData->show( channelList->getActiveChannelName() );
				}
				else
				{
					g_InfoViewer->showTitle(  channelList->getActiveChannelNumber(),
					                         channelList->getActiveChannelName() );
				}
			}
			else if ((key>=0) && (key<=9))
			{ //numeric zap
				channelList->numericZap( key );
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
	g_FrameBuffer->paintIcon8("shutdown.raw",0,0);
	g_FrameBuffer->loadPal("shutdown.pal");

	g_Controld->shutdown();
	sleep(55555);
}

int CNeutrinoApp::run(int argc, char **argv)
{
	CmdParser(argc, argv);

	fontRenderer = new fontRenderClass;

	SetupFrameBuffer();

    g_Fonts = new FontsDef;	
	SetupFonts();

	ClearFrameBuffer();

	if(!loadSetup())
	{
		//setup default if configfile not exists
		setupDefaults();
		printf("using defaults...\n\n");
	}
	g_Locale = new CLocaleManager;
    g_RCInput = new CRCInput;
    g_lcdd = new CLCDD;
    g_Controld = new CControld;
    g_RemoteControl = new CRemoteControl;
    g_EpgData = new CEpgData;
    g_InfoViewer = new CInfoViewer;
    g_StreamInfo = new CStreamInfo;
    g_ScreenSetup = new CScreenSetup;

    printf("\nCNeutrinoApp::run - objects initialized...\n\n");
	g_Locale->loadLocale("deutsch");

	colorSetupNotifier = new CColorSetupNotifier();

	CAudioSetupNotifier        audioSetupNotifier;
	CVideoSetupNotifier        videoSetupNotifier;
	CNetworkSetupNotifier      networkSetupNotifier;
	
	colorSetupNotifier->changeNotify("initial");

	setupNetwork();

	channelList = new CChannelList( 1, g_Locale->getText("channellist.head") );

	//Main settings
	CMenuWidget mainSettings(g_Locale->getText("mainmenu.head"), "settings.raw");
	CMenuWidget videoSettings(g_Locale->getText("videomenu.head"), "video.raw");
	CMenuWidget audioSettings(g_Locale->getText("audiomenu.head"), "audio.raw");
	CMenuWidget networkSettings(g_Locale->getText("networkmenu.head"), "settings.raw");
	CMenuWidget colorSettings(g_Locale->getText("colormenu.head"), "settings.raw");
	CMenuWidget keySettings(g_Locale->getText("keybindingmenu.head"), "settings.raw");
//	CMenuWidget screenSettings("",fonts,"");

	InitMainSettings(mainSettings, audioSettings, networkSettings, colorSettings, keySettings, videoSettings);

	//audio Setup
	InitAudioSettings(audioSettings, audioSetupNotifier);

	//video Setup
	InitVideoSettings(videoSettings, videoSetupNotifier);

	//network Setup
	InitNetworkSettings(networkSettings, networkSetupNotifier);
	
	//color Setup
	InitColorSettings(colorSettings);

	CMenuWidget colorSettings_Themes(g_Locale->getText("colorthememenu.head"), "settings.raw");
	InitColorThemesSettings(colorSettings_Themes);

	// Hacking Shit
	colorSettings.addItem( new CMenuForwarder(g_Locale->getText("colormenu.themeselect"), true, "", &colorSettings_Themes) );
	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	// Meno

   	CMenuWidget colorSettings_menuColors(g_Locale->getText("colormenusetup.head"), "settings.raw");
	InitColorSettingsMenuColors(colorSettings_menuColors, colorSettings);

	CMenuWidget colorSettings_statusbarColors(g_Locale->getText("colormenu.statusbar"), "settings.raw");
	InitColorSettingsStatusBarColors(colorSettings_statusbarColors, colorSettings);

	//keySettings
	InitKeySettings(keySettings);

	//init programm
	InitZapper();

	RealRun(mainSettings);

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
        g_FrameBuffer->paintBoxRel(x, y, dx, dy, COL_BACKGROUND);
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
            g_RCInput->addKey2Buffer(key);
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

	g_FrameBuffer->paintBoxRel(x, y, dx, dy, COL_BACKGROUND);
}

void CNeutrinoApp::tvMode()
{
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
	printf("ac: %s\n", actionKey.c_str());
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
	CNeutrinoApp neutrino;
	return neutrino.run(argc, argv);
}




