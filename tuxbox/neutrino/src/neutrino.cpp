/*

        $Id: neutrino.cpp,v 1.18 2001/08/22 00:03:24 ge0rg Exp $

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

class CColorSetupNotifier : public CChangeObserver
{
	SNeutrinoSettings*	settings;
	CFrameBuffer*		frameBuffer;

	public:
		CColorSetupNotifier(CFrameBuffer* FrameBuffer, SNeutrinoSettings* Settings){settings=Settings;frameBuffer=FrameBuffer;}

		void changeNotify(string OptionName)
		{
			//setting colors-..
			frameBuffer->paletteGenFade(COL_MENUHEAD, 
				convertSetupColor2RGB(settings->menu_Head_red,settings->menu_Head_green,settings->menu_Head_blue),
				convertSetupColor2RGB(settings->menu_Head_Text_red,settings->menu_Head_Text_green,settings->menu_Head_Text_blue),
				8, convertSetupAlpha2Alpha(settings->menu_Head_alpha) );
			
			frameBuffer->paletteGenFade(COL_MENUCONTENT, 
				convertSetupColor2RGB(settings->menu_Content_red,settings->menu_Content_green,settings->menu_Content_blue),
				convertSetupColor2RGB(settings->menu_Content_Text_red,settings->menu_Content_Text_green,settings->menu_Content_Text_blue),
				8, convertSetupAlpha2Alpha(settings->menu_Content_alpha) );

			frameBuffer->paletteGenFade(COL_MENUCONTENTSELECTED, 
				convertSetupColor2RGB(settings->menu_Content_Selected_red,settings->menu_Content_Selected_green,settings->menu_Content_Selected_blue),
				convertSetupColor2RGB(settings->menu_Content_Selected_Text_red,settings->menu_Content_Selected_Text_green,settings->menu_Content_Selected_Text_blue),
				8, convertSetupAlpha2Alpha(settings->menu_Content_Selected_alpha) );

			frameBuffer->paletteGenFade(COL_MENUCONTENTINACTIVE, 
				convertSetupColor2RGB(settings->menu_Content_inactive_red,settings->menu_Content_inactive_green,settings->menu_Content_inactive_blue),
				convertSetupColor2RGB(settings->menu_Content_inactive_Text_red,settings->menu_Content_inactive_Text_green,settings->menu_Content_inactive_Text_blue),
				8, convertSetupAlpha2Alpha(settings->menu_Content_inactive_alpha) );

			frameBuffer->paletteGenFade(COL_INFOBAR, 
				convertSetupColor2RGB(settings->infobar_red,settings->infobar_green,settings->infobar_blue),
				convertSetupColor2RGB(settings->infobar_Text_red,settings->infobar_Text_green,settings->infobar_Text_blue),
				8, convertSetupAlpha2Alpha(settings->infobar_alpha) );

			frameBuffer->paletteSetColor( COL_INFOBAR_SHADOW, 
							convertSetupColor2RGB(
								int(settings->infobar_red*0.4),
								int(settings->infobar_green*0.4),
								int(settings->infobar_blue*0.4)),
							settings->infobar_alpha);

			frameBuffer->paletteSet();
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

void setNetworkAdress(char* ip, char* netmask, char* broadcast)
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
	SNeutrinoSettings*	settings;

	public:
		CNetworkSetupNotifier(SNeutrinoSettings* Settings){settings=Settings;}

		void changeNotify(string OptionName)
		{
			printf("notify: %s\n", OptionName.c_str() );
			if( (settings->networkSetOnStartup) && (OptionName=="initial"))
			{
				printf("doing network setup...\n");
				//setup network
				setNetworkAdress(settings->network_ip, settings->network_netmask, settings->network_broadcast);
				setDefaultGateway(settings->network_defaultgateway);

				FILE* fd = fopen("/etc/resolv.conf", "w");
				if(fd)
				{
					fprintf(fd, "# resolv.conf - generated by neutrino\n\n");
					fprintf(fd, "nameserver %s\n", settings->network_nameserver);
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
	frameBuffer.setIconBasePath("/usr/lib/icons/");
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

void CNeutrinoApp::setupNetwork(SNeutrinoSettings* settings, bool force)
{
	if((settings->networkSetOnStartup) || (force))
	{
		printf("doing network setup...\n");
		//setup network
		setNetworkAdress(settings->network_ip, settings->network_netmask, settings->network_broadcast);
		setDefaultGateway(settings->network_defaultgateway);

		FILE* fd = fopen("/etc/resolv.conf", "w");
		if(fd)
		{
			fprintf(fd, "# resolv.conf - generated by neutrino\n\n");
			fprintf(fd, "nameserver %s\n", settings->network_nameserver);
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
void CNeutrinoApp::setupColors_neutrino(SNeutrinoSettings* settings)
{
	settings->menu_Head_alpha = 0x00;
	settings->menu_Head_red   = 0x00;
	settings->menu_Head_green = 0x0A; 
	settings->menu_Head_blue  = 0x19;

	settings->menu_Head_Text_alpha = 0x00;
	settings->menu_Head_Text_red   = 0x5f;
	settings->menu_Head_Text_green = 0x41; 
	settings->menu_Head_Text_blue  = 0x00;

	settings->menu_Content_alpha = 0x14;
	settings->menu_Content_red   = 0x00;
	settings->menu_Content_green = 0x14; 
	settings->menu_Content_blue  = 0x23;

	settings->menu_Content_Text_alpha = 0x00;
	settings->menu_Content_Text_red   = 0x64;
	settings->menu_Content_Text_green = 0x64; 
	settings->menu_Content_Text_blue  = 0x64;

	settings->menu_Content_Selected_alpha = 0x14;
	settings->menu_Content_Selected_red   = 0x19;
	settings->menu_Content_Selected_green = 0x3c;
	settings->menu_Content_Selected_blue  = 0x64;

	settings->menu_Content_Selected_Text_alpha  = 0x00;
	settings->menu_Content_Selected_Text_red    = 0x00;
	settings->menu_Content_Selected_Text_green  = 0x00; 
	settings->menu_Content_Selected_Text_blue   = 0x00;

	settings->menu_Content_inactive_alpha = 0x14;
	settings->menu_Content_inactive_red   = 0x00;
	settings->menu_Content_inactive_green = 0x14;
	settings->menu_Content_inactive_blue  = 0x23;

	settings->menu_Content_inactive_Text_alpha  = 0x00;
	settings->menu_Content_inactive_Text_red    = 0x1e;
	settings->menu_Content_inactive_Text_green  = 0x28; 
	settings->menu_Content_inactive_Text_blue   = 0x3c;

	settings->infobar_alpha = 0x14;
	settings->infobar_red   = 0x00;
	settings->infobar_green = 0x14;
	settings->infobar_blue  = 0x23;

	settings->infobar_Text_alpha = 0x00;
	settings->infobar_Text_red   = 0x64;
	settings->infobar_Text_green = 0x64;
	settings->infobar_Text_blue  = 0x64;
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setup Color Sheme (classic)                                *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupColors_classic(SNeutrinoSettings* settings)
{
	settings->menu_Head_alpha = 20;
	settings->menu_Head_red   =  5;
	settings->menu_Head_green = 10; 
	settings->menu_Head_blue  = 60;

	settings->menu_Head_Text_alpha = 0;
	settings->menu_Head_Text_red   = 100;
	settings->menu_Head_Text_green = 100; 
	settings->menu_Head_Text_blue  = 100;

	settings->menu_Content_alpha = 20;
	settings->menu_Content_red   = 50;
	settings->menu_Content_green = 50; 
	settings->menu_Content_blue  = 50;

	settings->menu_Content_Text_alpha = 0;
	settings->menu_Content_Text_red   = 100;
	settings->menu_Content_Text_green = 100; 
	settings->menu_Content_Text_blue  = 100;

	settings->menu_Content_Selected_alpha = 20;
	settings->menu_Content_Selected_red   = 5;
	settings->menu_Content_Selected_green = 10;
	settings->menu_Content_Selected_blue  = 60;

	settings->menu_Content_Selected_Text_alpha  = 0;
	settings->menu_Content_Selected_Text_red    = 100;
	settings->menu_Content_Selected_Text_green  = 100; 
	settings->menu_Content_Selected_Text_blue   = 100;

	settings->menu_Content_inactive_alpha = 20;
	settings->menu_Content_inactive_red   = 50;
	settings->menu_Content_inactive_green = 50;
	settings->menu_Content_inactive_blue  = 50;

	settings->menu_Content_inactive_Text_alpha  = 0;
	settings->menu_Content_inactive_Text_red    = 80;
	settings->menu_Content_inactive_Text_green  = 80; 
	settings->menu_Content_inactive_Text_blue   = 80;

	settings->infobar_alpha = 20;
	settings->infobar_red   = 5;
	settings->infobar_green = 10;
	settings->infobar_blue  = 60;

	settings->infobar_Text_alpha = 0;
	settings->infobar_Text_red   = 100;
	settings->infobar_Text_green = 100;
	settings->infobar_Text_blue  = 100;
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  setupDefaults, set the application-defaults                *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::setupDefaults(SNeutrinoSettings* settings)
{
	//video
	settings->video_Signal = 0;
	settings->video_Format = 0;

	//audio
	settings->audio_Stereo = 1;
	settings->audio_DolbyDigital = 0;

	//colors
	setupColors_neutrino(settings);

	//network
	settings->networkSetOnStartup = 0;
	strcpy(settings->network_ip, "192.168.40.10");
	strcpy(settings->network_netmask, "255.255.255.000");
	strcpy(settings->network_broadcast, "192.168.40.255");
	strcpy(settings->network_defaultgateway, "192.168.40.1");
	strcpy(settings->network_nameserver, "192.168.40.1");

	//key bindings
	settings->key_tvradio_mode = CRCInput::RC_nokey;
	settings->key_channelList_pageup = CRCInput::RC_green;
	settings->key_channelList_pagedown = CRCInput::RC_red;
	settings->key_channelList_cancel = CRCInput::RC_home;
	settings->key_quickzap_up = CRCInput::RC_up;
	settings->key_quickzap_down = CRCInput::RC_down;

	//screen settings
	settings->screen_StartX=37;
	settings->screen_StartY=23;
	settings->screen_EndX=668;
	settings->screen_EndY=555;
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  loadSetup, load the application-settings                   *
*                                                                                     *
**************************************************************************************/
bool CNeutrinoApp::loadSetup(SNeutrinoSettings* settings)
{
	int fd;
	fd = open(settingsFile.c_str(), O_RDONLY );
	
	if (fd==-1)
	{
		printf("error while loading settings: %s\n", settingsFile.c_str() );
		return false;
	}
	if(read(fd, settings, sizeof(*settings))!=sizeof(*settings))
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
void CNeutrinoApp::saveSetup(SNeutrinoSettings* settings)
{
	int fd;
	fd = open(settingsFile.c_str(), O_WRONLY | O_CREAT );
	
	if (fd==-1)
	{
		printf("error while saving settings: %s\n", settingsFile.c_str() );
		return;
	}
	write(fd, settings,  sizeof(*settings) );
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
	memset(return_buf, 0, sizeof(return_buf));
	
	if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
		perror("recv(zapit)");
		exit(-1);
	}
	
	printf("That was returned: %s\n", return_buf);
	
	if (strncmp(return_buf,"00a",3))
	{
		printf("Wrong Command was sent for firstChannel(). Exiting.\n");
		return;
	}
	
	
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

//tw??	
	delete channelList;
	channelList = new CChannelList(&settings, 1,"All Services");
	
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
		memset(return_buf, 0, sizeof(return_buf));
		if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
			perror("recv(zapit)");
			exit(-1);
		}
	
		printf("That was returned: %s\n", return_buf);
	
		if (atoi(return_buf) != 5)
		{
			printf("Wrong Command was send for channelsInit(). Exiting.\n");
			return;
		}
	
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
	if (frameBuffer.setMode(720, 576, 8))
	{
		printf("Error while setting framebuffer mode\n");
		exit(-1);
	}

	//make 1..8 transparent for dummy painting
	for(int count =0;count<8;count++)
		frameBuffer.paletteSetColor(count, 0x000000, 0xffff); 
	frameBuffer.paletteSet();
}

void CNeutrinoApp::SetupFonts()
{
	fonts = new FontsDef();
	fonts->menu=fontRenderer->getFont("Arial", "Bold", 20);
	fonts->menu->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts->menu_title=fontRenderer->getFont("Arial Black", "Regular", 24);
	fonts->menu_title->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts->epg_title=fontRenderer->getFont("Arial", "Regular", 30);
	fonts->epg_title->RenderString( 10,100, 500, "DEMO!", 0 );

	// only one epg_info font instead of info1 and info2 (too complicated rendering calcs for epgviewer):
	fonts->epg_info=fontRenderer->getFont("Arial", "Regular", 17);
	fonts->epg_info->RenderString( 10,100, 500, "DEMO!", 0 );

	fonts->epg_date=fontRenderer->getFont("Arial", "Regular", 15);
	fonts->epg_date->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts->alert=fontRenderer->getFont("Arial", "Regular", 100);
	fonts->channellist=fontRenderer->getFont("Arial", "Regular", 20);
	fonts->channellist->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts->channellist_number=fontRenderer->getFont("Arial", "Regular", 14);
	fonts->channellist_number->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts->infobar_number=fontRenderer->getFont("Arial", "Regular", 50);
	fonts->infobar_number->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts->infobar_channame=fontRenderer->getFont("Arial", "Regular", 30);
	fonts->infobar_channame->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts->infobar_info=fontRenderer->getFont("Arial", "Regular", 20);
	fonts->infobar_info->RenderString( 10,100, 500, "DEMO!", 0 );
}

void CNeutrinoApp::ClearFrameBuffer()
{
	memset(frameBuffer.lfb, 255, frameBuffer.Stride()*576);

	//backgroundmode
	frameBuffer.setBackgroundColor(COL_BACKGROUND);
	frameBuffer.useBackground(false);

	//background
	frameBuffer.paletteSetColor(COL_BACKGROUND, 0x000000, 0xffff); 
	//Windows Colors
	frameBuffer.paletteSetColor(0x0, 0x010101, 0);
	frameBuffer.paletteSetColor(0x1, 0x800000, 0);
	frameBuffer.paletteSetColor(0x2, 0x008000, 0);
	frameBuffer.paletteSetColor(0x3, 0x808000, 0);
	frameBuffer.paletteSetColor(0x4, 0x000080, 0);
	frameBuffer.paletteSetColor(0x5, 0x800080, 0);
	frameBuffer.paletteSetColor(0x6, 0x008080, 0);
//	frameBuffer.paletteSetColor(0x7, 0xC0C0C0, 0);
	frameBuffer.paletteSetColor(0x7, 0xA0A0A0, 0);

//	frameBuffer.paletteSetColor(0x8, 0x808080, 0);
	frameBuffer.paletteSetColor(0x8, 0x505050, 0);

	frameBuffer.paletteSetColor(0x9, 0xFF0000, 0);
	frameBuffer.paletteSetColor(0xA, 0x00FF00, 0);
	frameBuffer.paletteSetColor(0xB, 0xFFFF00, 0);
	frameBuffer.paletteSetColor(0xC, 0x0000FF, 0);
	frameBuffer.paletteSetColor(0xD, 0xFF00FF, 0);
	frameBuffer.paletteSetColor(0xE, 0x00FFFF, 0);
	frameBuffer.paletteSetColor(0xF, 0xFFFFFF, 0);

	frameBuffer.paletteSet();
}

void CNeutrinoApp::InitMainSettings(CMenuWidget &mainSettings, CMenuWidget &audioSettings, CMenuWidget &networkSettings,
				     CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings)
{
	mainSettings.addItem( new CMenuSeparator() );
	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Run mode", fonts) );
	mainSettings.addItem( new CMenuForwarder("Shutdown", fonts, true, "", this, "shutdown") );
	mainSettings.addItem( new CMenuForwarder("TV-Mode", fonts, true, "", this, "tv"), true );
	mainSettings.addItem( new CMenuForwarder("Radio-Mode", fonts, true, "", this, "radio") );
	mainSettings.addItem( new CMenuForwarder("MP3-Player", fonts, false, "", this, "mp3") );
	mainSettings.addItem( new CMenuForwarder("Stream playback", fonts, false, "", this, "playback") );

	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Info", fonts) );
		CStreamInfo* StreamInfo = new CStreamInfo(fonts);
	mainSettings.addItem( new CMenuForwarder("Stream Info", fonts, true, "", StreamInfo) );

	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Settings", fonts) );
	mainSettings.addItem( new CMenuForwarder("Video", fonts, true, "", &videoSettings) );
		CScreenSetup* screenSettings= new CScreenSetup("Screen setup", fonts, &settings);
	mainSettings.addItem( new CMenuForwarder("Screen Setup", fonts, true, "", screenSettings) );
	mainSettings.addItem( new CMenuForwarder("Audio", fonts, true, "", &audioSettings) );
	mainSettings.addItem( new CMenuForwarder("Network", fonts, true, "", &networkSettings) );
	mainSettings.addItem( new CMenuForwarder("Colors", fonts, true,"", &colorSettings) );
	mainSettings.addItem( new CMenuForwarder("Key binding", fonts, true,"", &keySettings) );
}

void CNeutrinoApp::InitAudioSettings(CMenuWidget &audioSettings, CAudioSetupNotifier &audioSetupNotifier)
{
	audioSettings.addItem( new CMenuSeparator() );
	audioSettings.addItem( new CMenuForwarder("back", fonts) );
	audioSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CMenuOptionChooser* oj = new CMenuOptionChooser("Stereo", fonts, &settings.audio_Stereo, true, &audioSetupNotifier);
		oj->addOption(0, "off");
		oj->addOption(1, "on");
	audioSettings.addItem( oj );
		oj = new CMenuOptionChooser("Dolby Digital", fonts, &settings.audio_DolbyDigital, true, &audioSetupNotifier);
		oj->addOption(0, "off");
		oj->addOption(1, "on");
	audioSettings.addItem( oj );	
}

void CNeutrinoApp::InitVideoSettings(CMenuWidget &videoSettings, CVideoSetupNotifier &videoSetupNotifier)
{
	videoSettings.addItem( new CMenuSeparator() );
	videoSettings.addItem( new CMenuForwarder("back", fonts) );
	videoSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CMenuOptionChooser* oj = new CMenuOptionChooser("Output signal", fonts, &settings.video_Signal, true, &videoSetupNotifier);
		oj->addOption(0, "RGB");
		oj->addOption(1, "S-Video");
		oj->addOption(2, "FBAS");
	videoSettings.addItem( oj );
		oj = new CMenuOptionChooser("Format", fonts, &settings.video_Format, true, &videoSetupNotifier);
		oj->addOption(0, "4:3");
		oj->addOption(1, "16:9");
	videoSettings.addItem( oj );	
}

void CNeutrinoApp::InitNetworkSettings(CMenuWidget &networkSettings, CNetworkSetupNotifier &networkSetupNotifier)
{
	networkSettings.addItem( new CMenuSeparator() );
	networkSettings.addItem( new CMenuForwarder("back", fonts) );
	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	networkSettings.addItem( new CMenuForwarder("setup network now", fonts, true, "", this, "network") );

	CMenuOptionChooser* oj = new CMenuOptionChooser("setup network on startup", fonts, &settings.networkSetOnStartup, true, &networkSetupNotifier);
		oj->addOption(0, "off");
		oj->addOption(1, "on");
	networkSettings.addItem( oj );	

	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CStringInput*	networkSettings_NetworkIP= new CStringInput("IP Adress", fonts, settings.network_ip, 3*4+3);
		CStringInput*	networkSettings_NetMask= new CStringInput("Network mask", fonts, settings.network_netmask, 3*4+3);
		CStringInput*	networkSettings_Broadcast= new CStringInput("Broadcast", fonts, settings.network_broadcast, 3*4+3);
		CStringInput*	networkSettings_Gateway= new CStringInput("Default gateway", fonts, settings.network_defaultgateway, 3*4+3);
		CStringInput*	networkSettings_NameServer= new CStringInput("Nameserver", fonts, settings.network_nameserver, 3*4+3);
	networkSettings.addItem( new CMenuForwarder("IP Adress", fonts, true, settings.network_ip, networkSettings_NetworkIP ));
	networkSettings.addItem( new CMenuForwarder("Netmask", fonts, true, settings.network_netmask, networkSettings_NetMask ));
	networkSettings.addItem( new CMenuForwarder("Broadcast", fonts, true, settings.network_broadcast, networkSettings_Broadcast ));
	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	networkSettings.addItem( new CMenuForwarder("Default gateway", fonts, true, settings.network_defaultgateway, networkSettings_Gateway ));
	networkSettings.addItem( new CMenuForwarder("Nameserver", fonts, true, settings.network_nameserver, networkSettings_NameServer ));
}

void CNeutrinoApp::InitColorSettings(CMenuWidget &colorSettings)
{
	colorSettings.addItem( new CMenuSeparator() );
	colorSettings.addItem( new CMenuForwarder("back", fonts) );
	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
}

void CNeutrinoApp::InitAudioThemesSettings(CMenuWidget &audioSettings_Themes)
{
	audioSettings_Themes.addItem( new CMenuSeparator() );
	audioSettings_Themes.addItem( new CMenuForwarder("back", fonts) );
	audioSettings_Themes.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	audioSettings_Themes.addItem( new CMenuForwarder("Neutrino default theme", fonts, true, "", this, "theme_neutrino") );
	audioSettings_Themes.addItem( new CMenuForwarder("Classic theme", fonts, true, "", this, "theme_classic") );
}

void CNeutrinoApp::InitColorSettingsMenuColors(CMenuWidget &colorSettings_menuColors, CMenuWidget &colorSettings)
{
	colorSettings_menuColors.addItem( new CMenuSeparator() );
	colorSettings_menuColors.addItem( new CMenuForwarder("back", fonts) );
			CColorChooser* chHeadcolor = new CColorChooser("background color", fonts, &settings.menu_Head_red, &settings.menu_Head_green, &settings.menu_Head_blue, 
					&settings.menu_Head_alpha, colorSetupNotifier);
			CColorChooser* chHeadTextcolor = new CColorChooser("text color", fonts, &settings.menu_Head_Text_red, &settings.menu_Head_Text_green, &settings.menu_Head_Text_blue, 
					NULL, colorSetupNotifier);
			CColorChooser* chContentcolor = new CColorChooser("background color", fonts, &settings.menu_Content_red, &settings.menu_Content_green, &settings.menu_Content_blue, 
					&settings.menu_Content_alpha, colorSetupNotifier);
			CColorChooser* chContentTextcolor = new CColorChooser("text color", fonts, &settings.menu_Content_Text_red, &settings.menu_Content_Text_green, &settings.menu_Content_Text_blue, 
					NULL, colorSetupNotifier);
			CColorChooser* chContentSelectedcolor = new CColorChooser("background color", fonts, &settings.menu_Content_Selected_red, &settings.menu_Content_Selected_green, &settings.menu_Content_Selected_blue, 
					&settings.menu_Content_Selected_alpha, colorSetupNotifier);
			CColorChooser* chContentSelectedTextcolor = new CColorChooser("text color", fonts, &settings.menu_Content_Selected_Text_red, &settings.menu_Content_Selected_Text_green, &settings.menu_Content_Selected_Text_blue, 
					NULL, colorSetupNotifier);
			CColorChooser* chContentInactivecolor = new CColorChooser("background color", fonts, &settings.menu_Content_inactive_red, &settings.menu_Content_inactive_green, &settings.menu_Content_inactive_blue, 
					&settings.menu_Content_inactive_alpha, colorSetupNotifier);
			CColorChooser* chContentInactiveTextcolor = new CColorChooser("text color", fonts, &settings.menu_Content_inactive_Text_red, &settings.menu_Content_inactive_Text_green, &settings.menu_Content_inactive_Text_blue, 
					NULL, colorSetupNotifier);
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu heads", fonts) );
	colorSettings_menuColors.addItem( new CMenuForwarder("Background", fonts, true, "", chHeadcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", fonts, true, "", chHeadTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu body", fonts) );
	colorSettings_menuColors.addItem( new CMenuForwarder("Background", fonts, true, "", chContentcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", fonts, true, "", chContentTextcolor ));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu body - inactive", fonts) );
	colorSettings_menuColors.addItem( new CMenuForwarder("Background", fonts, true, "", chContentInactivecolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", fonts, true, "", chContentInactiveTextcolor));
	colorSettings_menuColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu body - selected", fonts) );
	colorSettings_menuColors.addItem( new CMenuForwarder("Background", fonts, true, "", chContentSelectedcolor ));
	colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", fonts, true, "", chContentSelectedTextcolor ));

	colorSettings.addItem( new CMenuForwarder("menu colors", fonts, true, "", &colorSettings_menuColors) );
}

void CNeutrinoApp::InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_statusbarColors, CMenuWidget &colorSettings)
{
	colorSettings_statusbarColors.addItem( new CMenuSeparator() );
	colorSettings_statusbarColors.addItem( new CMenuForwarder("back", fonts) );
			CColorChooser* chInfobarcolor = new CColorChooser("background color", fonts, &settings.infobar_red, &settings.infobar_green, &settings.infobar_blue, 
					&settings.infobar_alpha, colorSetupNotifier);
			CColorChooser* chInfobarTextcolor = new CColorChooser("text color", fonts, &settings.infobar_Text_red, &settings.infobar_Text_green, &settings.infobar_Text_blue, 
					NULL, colorSetupNotifier);
	colorSettings_statusbarColors.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Status bars", fonts) );
	colorSettings_statusbarColors.addItem( new CMenuForwarder("Background", fonts, true, "", chInfobarcolor ));
	colorSettings_statusbarColors.addItem( new CMenuForwarder("Textcolor", fonts, true, "", chInfobarTextcolor ));
	colorSettings.addItem( new CMenuForwarder("statusbar colors", fonts, true, "", &colorSettings_statusbarColors) );
}

void CNeutrinoApp::InitKeySettings(CMenuWidget &keySettings)
{
	keySettings.addItem( new CMenuSeparator() );
	keySettings.addItem( new CMenuForwarder("back", fonts) );
		CKeyChooser*	keySettings_tvradio_mode = new CKeyChooser(&settings.key_tvradio_mode, "tv/radio mode key setup", fonts, "settings.raw");
		CKeyChooser*	keySettings_channelList_pageup = new CKeyChooser(&settings.key_channelList_pageup, "page up key setup", fonts, "settings.raw");
		CKeyChooser*	keySettings_channelList_pagedown = new CKeyChooser(&settings.key_channelList_pagedown, "page down key setup", fonts, "settings.raw");
		CKeyChooser*	keySettings_channelList_cancel = new CKeyChooser(&settings.key_channelList_cancel, "cancel key setup", fonts, "settings.raw");
		CKeyChooser*	keySettings_quickzap_up = new CKeyChooser(&settings.key_quickzap_up, "up key setup", fonts, "settings.raw");
		CKeyChooser*	keySettings_quickzap_down = new CKeyChooser(&settings.key_quickzap_down, "down key setup", fonts, "settings.raw");
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "mode change", fonts) );
	keySettings.addItem( new CMenuForwarder("tv/radio mode", fonts, true, "", keySettings_tvradio_mode ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "channellist", fonts) );
	keySettings.addItem( new CMenuForwarder("page up", fonts, true, "", keySettings_channelList_pageup ));
	keySettings.addItem( new CMenuForwarder("page down", fonts, true, "", keySettings_channelList_pagedown ));
	keySettings.addItem( new CMenuForwarder("cancel", fonts, true, "", keySettings_channelList_cancel ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "quickzap", fonts) );
	keySettings.addItem( new CMenuForwarder("channel up", fonts, true, "", keySettings_quickzap_up ));
	keySettings.addItem( new CMenuForwarder("channel down", fonts, true, "", keySettings_quickzap_down ));
}

void CNeutrinoApp::InitZapper()
{
	remoteControl.setZapper(zapit);
	volume = 100;
	if (!zapit)
		channelsInit();
		
	infoViewer.start(&frameBuffer, fonts, &settings);
	epgData.start(&frameBuffer, fonts, &rcInput, &settings);	
		
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
		channelList->zapTo(&remoteControl, &infoViewer,  0);
}

void CNeutrinoApp::RealRun(CMenuWidget &mainSettings)
{
	mute = false;
	while(nRun)
	{
		int key = rcInput.getKey(); 

		if (key==CRCInput::RC_setup)
		{
			infoViewer.killTitle();
			mainSettings.exec(&frameBuffer, &rcInput, NULL, "");
		}
		else if (key==CRCInput::RC_standby)
		{
			//exit
			infoViewer.killTitle();
			nRun=false;
		}

		if ((mode==mode_tv) || ((mode==mode_radio) && (zapit)) )
		{
			if (key==CRCInput::RC_ok)
			{	//channellist
				infoViewer.killTitle();
				channelList->exec(&frameBuffer, &rcInput, &remoteControl, &infoViewer, &settings);
			}
			else if ((key==settings.key_quickzap_up) || (key==settings.key_quickzap_down))
			{
				//quickzap
				channelList->quickZap(&frameBuffer, &rcInput, &remoteControl, &infoViewer, &settings, key);
			}
			else if (key==CRCInput::RC_help)
			{	//epg
				if(infoViewer.isActive())
				{
					infoViewer.killTitle();
					epgData.show( channelList->getActiveChannelName() );
				}
				else
				{
					infoViewer.showTitle(   channelList->getActiveChannelNumber(), 
							channelList->getActiveChannelName(), true );
				}
			}
			else if ((key>=0) && (key<=9))
			{ //numeric zap
				channelList->numericZap( &frameBuffer, &rcInput, &remoteControl, &infoViewer, key);
			}
			else if (key==CRCInput::RC_spkr)
			{	//mute
				if(mute)
				{
					AudioUnMute();
				}
				else
				{
					AudioMute();
				}
			}
			else if ((key==CRCInput::RC_plus) || (key==CRCInput::RC_minus))
			{	//volume
				infoViewer.killTitle();
				setVolume( key );
			}
		}
	}
}

void CNeutrinoApp::ExitRun()
{
	saveSetup(&settings);
	printf("neutrino exit\n");
	//shutdown screen
	infoViewer.killTitle();
	memset(frameBuffer.lfb, 255, frameBuffer.Stride()*576);
	for(int x=0;x<256;x++)
		frameBuffer.paletteSetColor(x, 0x000000, 0xffff); 
	frameBuffer.paletteSet();	
	frameBuffer.paintIcon8("shutdown.raw",0,0);
	frameBuffer.loadPal("shutdown.pal");
	Controld.shutdown();
	sleep(5);
	remoteControl.shutdown();
	sleep(55555);
}

int CNeutrinoApp::run(int argc, char **argv)
{
	printf("neutrino2\n");

	CmdParser(argc, argv);

	fontRenderer = new fontRenderClass( &frameBuffer);

	SetupFrameBuffer();
	
	SetupFonts();

	ClearFrameBuffer();

	if(!loadSetup(&settings))
	{
		//setup default if configfile not exists
		setupDefaults( &settings);
		printf("using defaults...\n\n");
	}

	colorSetupNotifier = new CColorSetupNotifier(&frameBuffer, &settings);
	CAudioSetupNotifier		audioSetupNotifier;
	CVideoSetupNotifier		videoSetupNotifier;
	CNetworkSetupNotifier		networkSetupNotifier(&settings);
	
	colorSetupNotifier->changeNotify("initial");

	setupNetwork(&settings);

	channelList = new CChannelList(&settings, 1,"All Services",fonts);

	//Main settings
	CMenuWidget mainSettings("Neutrino Setup",fonts,"settings.raw");
	CMenuWidget videoSettings("Video Setup",fonts,"video.raw");
	CMenuWidget audioSettings("Audio Setup",fonts,"audio.raw");
	CMenuWidget networkSettings("Network Setups",fonts,"settings.raw");
	CMenuWidget colorSettings("Color Setup",fonts,"settings.raw");
	CMenuWidget keySettings("Key Setup",fonts,"settings.raw");
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

	CMenuWidget colorSettings_statusbarColors("Statusbar Colors",fonts,"settings.raw");
	InitColorSettingsStatusBarColors(colorSettings_statusbarColors, colorSettings);

	CMenuWidget audioSettings_Themes("Select a theme",fonts,"settings.raw");
	InitAudioThemesSettings(audioSettings_Themes);

	// Hacking Shit
	colorSettings.addItem( new CMenuForwarder("select color theme", fonts, true, "", &audioSettings_Themes) );
	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	// Meno

   	CMenuWidget colorSettings_menuColors("menu colors",fonts,"settings.raw");
	InitColorSettingsMenuColors(colorSettings_menuColors, colorSettings);

	//keySettings
	InitKeySettings(keySettings);

	//init programm
	InitZapper();

	RealRun(mainSettings);

	ExitRun();
	return 0;
}

void CNeutrinoApp::AudioUnMute()
{
	int dx = 40;
	int dy = 40;
	int x = settings.screen_EndX-dx;
	int y = settings.screen_StartY;
	frameBuffer.paintBoxRel(x, y, dx, dy, COL_BACKGROUND);
	Controld.UnMute();
	mute = false;
}

void CNeutrinoApp::AudioMute()
{
	int dx = 40;
	int dy = 40;
	int x = settings.screen_EndX-dx;
	int y = settings.screen_StartY;
	frameBuffer.paintBoxRel(x, y, dx, dy, COL_INFOBAR);
	frameBuffer.paintIcon("mute.raw",x+5,y+5);
	Controld.Mute();
	mute = true;
}

void CNeutrinoApp::setVolume(int key)
{
	int dx = 256;
	int dy = 40;
	int x = (((settings.screen_EndX-settings.screen_StartX)-dx) / 2) + settings.screen_StartX;
	int y = settings.screen_EndY-100;
	frameBuffer.paintIcon("volume.raw",x,y, COL_INFOBAR);
	frameBuffer.paintBoxRel(x+40, y+12, 200, 15, COL_INFOBAR+1);
	int vol = volume<<1;
	frameBuffer.paintBoxRel(x+40, y+12, vol, 15, COL_INFOBAR+3);
	bool nRun=true;
	bool paint = false;
	while(nRun)
	{
		paint = false;
		if (key==CRCInput::RC_timeout)
		{
			nRun=false;
			break;
		}
		else if (key==CRCInput::RC_plus)
		{
			if (volume<100)
			{
				volume += 5;
				paint = true;
			}
		}
		else if (key==CRCInput::RC_minus)
		{
			if (volume>0)
			{
				volume -= 5;
				paint = true;
			}
		}
		else if (key==CRCInput::RC_spkr)
		{	//mute
			if(mute)
			{
				AudioUnMute();
			}
			else
			{
				AudioMute();
			}
			break;
		}
		if (paint)
		{
			Controld.setVolume(volume);
			int vol = volume<<1;
			frameBuffer.paintBoxRel(x+40, y+12, 200, 15, COL_INFOBAR+1);
			frameBuffer.paintBoxRel(x+40, y+12, vol, 15, COL_INFOBAR+3);
		}

		key = rcInput.getKey(30); 
	}
	frameBuffer.paintBoxRel(x, y, dx, dy, COL_BACKGROUND);
}

void CNeutrinoApp::tvMode()
{
	mode = mode_tv;
	infoViewer.killTitle();
	memset(frameBuffer.lfb, 255, frameBuffer.Stride()*576);
	frameBuffer.useBackground(false);

	if (zapit)
	{
		remoteControl.tvMode();
		channelsInit();
		channelList->zapTo(&remoteControl, &infoViewer,  firstchannel.chan_nr -1);
	}
}

void CNeutrinoApp::radioMode()
{
	mode = mode_radio;
	infoViewer.killTitle();
	frameBuffer.loadPal("dboxradio.pal", 18, 199);
	frameBuffer.paintIcon8("dboxradio.raw",0,0, 18);
	frameBuffer.loadBackground("dboxradio.raw", 18);
	frameBuffer.useBackground(true);

	if (zapit)
	{
		firstChannel();
		remoteControl.radioMode();
		channelsInit();
		channelList->zapTo(&remoteControl, &infoViewer,  0);
	} 
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  exec, menuitem callback (shutdown)                         *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::exec(CFrameBuffer* frameBuffer, CRCInput* rcInput, CMenuTarget* parent, string actionKey)
{
	printf("ac: %s\n", actionKey.c_str());
	int returnval = CMenuTarget::RETURN_REPAINT;

	if(actionKey=="theme_neutrino")
	{
		setupColors_neutrino( &settings );
		colorSetupNotifier->changeNotify("initial");
	}
	else if(actionKey=="theme_classic")
	{
		setupColors_classic( &settings );
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
		setupNetwork(&settings, true);
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

