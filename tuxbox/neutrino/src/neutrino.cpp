/*

        $Id: neutrino.cpp,v 1.199 2002/03/19 20:11:48 obi Exp $

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

#include <config.h>

#include "neutrino.h"

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
	g_Zapit = NULL;
	g_RemoteControl = NULL;

	g_EpgData = NULL;
	g_InfoViewer = NULL;
	g_EventList = NULL;
	g_StreamInfo = NULL;
	g_ScreenSetup = NULL;

	g_Locale = NULL;
	g_PluginList = NULL;
}
// Ende globale Variablen


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+                                                                                     +
+          CNeutrinoApp - Constructor, initialize g_fontRenderer                      +
+                                                                                     +
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
CNeutrinoApp::CNeutrinoApp()
{
	g_FrameBuffer = new CFrameBuffer;
	g_FrameBuffer->setIconBasePath(DATADIR "/neutrino/icons/");

	g_fontRenderer = new fontRenderClass;
	SetupFrameBuffer();

	settingsFile = CONFIGDIR "/neutrino.conf";

	mode = mode_unknown;
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
		setNameServer(g_settings.network_nameserver);
		setDefaultGateway(g_settings.network_defaultgateway);
	}
}
void CNeutrinoApp::testNetwork( )
{
	setupNetwork( true );

	printf("doing network test...\n");
	//test network
	testNetworkSettings(g_settings.network_ip, g_settings.network_netmask, g_settings.network_broadcast, g_settings.network_defaultgateway, g_settings.network_nameserver);
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

	strcpy(g_settings.language, "deutsch");

	//misc
	//g_settings.box_Type = 1;
	//g_settings.box_Type = g_Controld->getBoxType(); //passiert ohnehin immer...
	g_settings.shutdown_real = 1;
	g_settings.shutdown_showclock = 1;
	g_settings.show_camwarning = 1;

	//video
	g_settings.video_Signal = 0; //composite?
	g_settings.video_Format = 2; //4:3

	//audio
	g_settings.audio_Stereo = 1;
	g_settings.audio_DolbyDigital = 0;

	//vcr
	g_settings.vcr_AutoSwitch = 1;

	//colors
	setupColors_neutrino();

	//ts-scan
	g_settings.scan_astra = 1;
	g_settings.scan_eutel = 0;
	g_settings.scan_kopernikus = 0;
	g_settings.scan_digituerk = 0;
	g_settings.scan_bouquet = 1024; //erase bouquets


	//timing  (10 = 1 sec )
	g_settings.timing_menu = 1000;
	g_settings.timing_chanlist = 300;
	g_settings.timing_epg = 90000;
	g_settings.timing_infobar = 15; // 15 means 7,5 sec


	//network
	strcpy(g_settings.network_netmask, "255.255.255.0");
	strcpy(g_settings.network_defaultgateway, "");
	strcpy(g_settings.network_nameserver, "");

	FILE* fd = fopen("/var/tuxbox/config/ip", "r");
	if(fd)
	{
		char _ip[4];
		fread(_ip, 4, 4, fd);
		sprintf( g_settings.network_ip, "%d.%d.%d.%d", _ip[0], _ip[1], _ip[2], _ip[3] );
		sprintf( g_settings.network_broadcast, "%d.%d.%d.255", _ip[0], _ip[1], _ip[2] );
		fclose(fd);
		g_settings.networkSetOnStartup = 1;
	}
	else
	{
		strcpy(g_settings.network_ip, "10.10.10.100");
		strcpy(g_settings.network_broadcast, "10.10.10.255");
		g_settings.networkSetOnStartup = 0;
	}

	g_settings.network_streaming_use = 0;
	strcpy(g_settings.network_streamingserver, "10.10.10.10");
	strcpy(g_settings.network_streamingserverport, "4000");


	//key bindings
	//g_settings.key_tvradio_mode = CRCInput::RC_home;
	g_settings.key_tvradio_mode = CRCInput::RC_nokey;
	g_settings.key_channelList_pageup = CRCInput::RC_red;
	g_settings.key_channelList_pagedown = CRCInput::RC_green;
	g_settings.key_channelList_cancel = CRCInput::RC_home;
	g_settings.key_quickzap_up = CRCInput::RC_up;
	g_settings.key_quickzap_down = CRCInput::RC_down;
	g_settings.key_bouquet_up = CRCInput::RC_right;
	g_settings.key_bouquet_down = CRCInput::RC_left;
	g_settings.key_subchannel_up = CRCInput::RC_right;
	g_settings.key_subchannel_down = CRCInput::RC_left;

	if ( g_settings.box_Type == 2 )
	{
		// Sagem - andere Defaults...
		strcpy(g_settings.repeat_blocker, "150");
		strcpy(g_settings.repeat_genericblocker, "25");
	}
	else
	{
		strcpy(g_settings.repeat_blocker, "25");
		strcpy(g_settings.repeat_genericblocker, "0");
	}

	//screen settings
	g_settings.screen_StartX=37;
	g_settings.screen_StartY=23;
	g_settings.screen_EndX=668;
	g_settings.screen_EndY=555;

	//Soft-Update
	g_settings.softupdate_mode=1; //internet
	strcpy(g_settings.softupdate_proxyserver, "");
	strcpy(g_settings.softupdate_proxyusername, "");
	strcpy(g_settings.softupdate_proxypassword, "");

	//BouquetHandling
	g_settings.bouquetlist_mode=0; // show channellist of current bouquet

	// parentallock
	g_settings.parentallock_prompt = 0;
	g_settings.parentallock_lockage = 12;
	strcpy(g_settings.parentallock_pincode, "0000");

}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  loadSetup, load the application-settings                   *
*                                                                                     *
**************************************************************************************/
bool CNeutrinoApp::loadSetup(SNeutrinoSettings* load2)
{
	if(!load2)
	{
		load2 = &g_settings;
	}
	int fd;
	fd = open(settingsFile.c_str(), O_RDONLY );

	if (fd==-1)
	{
		printf("error while loading settings: %s\n", settingsFile.c_str() );
		return false;
	}
	if(read(fd, load2, sizeof(SNeutrinoSettings))!=sizeof(SNeutrinoSettings))
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
	bool tosave = false;

	SNeutrinoSettings tmp;
	if(loadSetup(&tmp)==1)
	{
		//compare...
		if(memcmp(&tmp, &g_settings, sizeof(SNeutrinoSettings))!=0)
		{
			tosave=true;
		}
	}
	else
	{
		tosave=true;
	}

	if(tosave)
	{
		int fd;
		fd = open(settingsFile.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR  |  S_IRGRP | S_IWGRP  |  S_IROTH | S_IWOTH);

		if (fd==-1)
		{
			printf("error while saving settings: %s\n", settingsFile.c_str() );
			return;
		}
		write(fd, &g_settings,  sizeof(SNeutrinoSettings) );
		close(fd);
		printf("[neutrino] settings saved\n");
	}
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

	//	printf("That was returned: %s\n", return_buf);

	if (strncmp(return_buf,"00a",3))
	{
		printf("Wrong Command was sent for firstChannel(). Exiting.\n");
		free(return_buf);
		return;
	}
	free(return_buf);


	memset(&firstchannel, 0, sizeof(firstchannel));
	if (recv(sock_fd, &firstchannel, sizeof(firstchannel),0) <= 0 )
	{
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

	//printf("That was returned: %s\n", return_buf);

	if (strncmp(return_buf,"00t",3))
	{
		printf("Wrong Command was sent for isCamValid(). Exiting.\n");
		free(return_buf);
		return;
	}
	free(return_buf);

	if (recv(sock_fd, &ca_verid, sizeof(int),0) <= 0 )
	{
		perror("Nothing could be received\n");
		exit(-1);
	}

	if  ( (ca_verid != 33 && ca_verid != 18 && ca_verid != 68) )
	{
		printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n!!\t\t\t\t\t\t\t!!\n!!\tATTENTION, YOUR CARD DOES NOT MATCH CAMALPHA.BIN!!\n!!\t\t\t\t\t\t\t!!\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");

		if (g_settings.show_camwarning)
		{
			if( ShowMsg ( "messagebox.error", g_Locale->getText("cam.wrong"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbNo ) == CMessageBox::mbrNo )
			{
				g_settings.show_camwarning = 0;
				saveSetup();
			}
		}
	}

}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit, get the Channellist from daemon              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit()
{
/*
	int sock_fd;
	SAI servaddr;
	char rip[]="127.0.0.1";
	char *return_buf;
	channel_msg_2   zapitchannel;
*/

	//deleting old channelList for mode-switching.
	delete channelList;
	channelList = new CChannelList( "channellist.head" );
/*
	sendmessage.version=1;
	// neu! war 5, mit neuem zapit holen wir uns auch die onid_tsid
	sendmessage.cmd = 'c';

	sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(1505);
	inet_pton(AF_INET, rip, &servaddr.sin_addr);

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
*/

	CZapitClient::BouquetChannelList zapitChannels;
	g_Zapit->getChannels( zapitChannels);
	for (uint i=0; i<zapitChannels.size(); i++)
	{
		channelList->addChannel( zapitChannels[i].nr, zapitChannels[i].nr, zapitChannels[i].name, zapitChannels[i].onid_sid );
	}

	delete bouquetList;
	bouquetList = new CBouquetList( "bouquetlist.head" );
	bouquetList->orgChannelList = channelList;
	CZapitClient::BouquetList zapitBouquets;
	g_Zapit->getBouquets(zapitBouquets, false);
	for (uint i=0; i<zapitBouquets.size(); i++)
	{
		bouquetList->addBouquet( zapitBouquets[i].name, zapitBouquets[i].bouquet_nr, zapitBouquets[i].locked);
	}

	for ( uint i=0; i< bouquetList->Bouquets.size(); i++ )
	{
		printf(".");
		CZapitClient::BouquetChannelList zapitChannels;
		g_Zapit->getBouquetChannels( bouquetList->Bouquets[i]->unique_key, zapitChannels);
		for (uint j=0; j<zapitChannels.size(); j++)
		{
			CChannelList::CChannel* channel = channelList->getChannel( zapitChannels[j].nr);

			bouquetList->Bouquets[i]->channelList->addChannel( channel);
			if ( bouquetList->Bouquets[i]->bLocked)
			{
				channel->bAlwaysLocked = true;
			}
		}
	}
	printf("\nAll bouquets-channels received\n");
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  run, the main runloop                                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::CmdParser(int argc, char **argv)
{
	softupdate = false;
	fromflash = false;
	g_settings.network_streaming_use = 0;

	for(int x=1; x<argc; x++)
	{
		if ( !strcmp(argv[x], "-su"))
		{
			printf("Software update enabled\n");
			softupdate = true;
		}
		else if ( !strcmp(argv[x], "-z"))
		{
			printf("zapitmode is default..\n");
		}
		else if ( !strcmp(argv[x], "-stream"))
		{
			printf("enable streaming-control\n");
			g_settings.network_streaming_use = 1;
		}
		else if ( !strcmp(argv[x], "-flash"))
		{
			printf("enable flash\n");
			fromflash = true;
		}
		else
		{
			printf("Usage: neutrino [-su]\n");
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
	g_Fonts->menu =         g_fontRenderer->getFont("Arial", "Bold", 20);
	g_Fonts->menu_title =   g_fontRenderer->getFont("Arial", "Bold", 30);
	g_Fonts->menu_info =    g_fontRenderer->getFont("Arial", "Regular", 16);

	g_Fonts->epg_title =    g_fontRenderer->getFont("Arial", "Regular", 25);

	g_Fonts->epg_info1 =	g_fontRenderer->getFont("Arial", "Italic", 17); // info1 must be same size as info2, but italic
	g_Fonts->epg_info2 =	g_fontRenderer->getFont("Arial", "Regular", 17);

	g_Fonts->epg_date =		g_fontRenderer->getFont("Arial", "Regular", 15);
	g_Fonts->alert =		g_fontRenderer->getFont("Arial", "Regular", 100);

	g_Fonts->eventlist_title =		g_fontRenderer->getFont("Arial", "Regular", 30);
	g_Fonts->eventlist_itemLarge =	g_fontRenderer->getFont("Arial", "Bold", 20);
	g_Fonts->eventlist_itemSmall =	g_fontRenderer->getFont("Arial", "Regular", 14);
	g_Fonts->eventlist_datetime =	g_fontRenderer->getFont("Arial", "Regular", 16);

	g_Fonts->gamelist_itemLarge =	g_fontRenderer->getFont("Arial", "Bold", 20);
	g_Fonts->gamelist_itemSmall =	g_fontRenderer->getFont("Arial", "Regular", 16);

	g_Fonts->channellist =			g_fontRenderer->getFont("Arial", "Bold", 20);
	g_Fonts->channellist_descr =	g_fontRenderer->getFont("Arial", "Regular", 20);
	g_Fonts->channellist_number =	g_fontRenderer->getFont("Arial", "Bold", 14);
	g_Fonts->channel_num_zap =		g_fontRenderer->getFont("Arial", "Bold", 40);

	g_Fonts->infobar_number =	g_fontRenderer->getFont("Arial", "Bold", 50);
	g_Fonts->infobar_channame =	g_fontRenderer->getFont("Arial", "Bold", 30);
	g_Fonts->infobar_info =		g_fontRenderer->getFont("Arial", "Regular", 20);
	g_Fonts->infobar_small =	g_fontRenderer->getFont("Arial", "Regular", 14);
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

void CNeutrinoApp::InitMainMenu(CMenuWidget &mainMenu, CMenuWidget &mainSettings,  CMenuWidget &audioSettings, CMenuWidget &parentallockSettings,
                                CMenuWidget &networkSettings, CMenuWidget &colorSettings, CMenuWidget &keySettings, CMenuWidget &videoSettings,
                                CMenuWidget &languageSettings, CMenuWidget &miscSettings, CMenuWidget &service)
{
	mainMenu.addItem( new CMenuSeparator() );
	mainMenu.addItem( new CMenuForwarder("mainmenu.tvmode", true, "", this, "tv", true, CRCInput::RC_red, "rot.raw"), true );
	mainMenu.addItem( new CMenuForwarder("mainmenu.radiomode", true, "", this, "radio", true, CRCInput::RC_green, "gruen.raw") );
	mainMenu.addItem( new CMenuForwarder("mainmenu.scartmode", true, "", this, "scart", true, CRCInput::RC_yellow, "gelb.raw") );
	mainMenu.addItem( new CMenuForwarder("mainmenu.games", true, "", new CGameList("mainmenu.games"), "", true, CRCInput::RC_blue, "blau.raw") );
	mainMenu.addItem( new CMenuForwarder("mainmenu.shutdown", true, "", this, "shutdown", true, CRCInput::RC_standby, "power.raw") );
	mainMenu.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	streamstatus = 0;
	if(g_settings.network_streaming_use)
	{
		CMenuOptionChooser* oj = new CMenuOptionChooser("mainmenu.streaming", &streamstatus, true, this );
		oj->addOption(0, "mainmenu.streaming_start");
		oj->addOption(1, "mainmenu.streaming_stop");
		mainMenu.addItem( oj );
		mainMenu.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	}
	mainMenu.addItem( new CMenuForwarder("mainmenu.settings", true, "", &mainSettings) );
	mainMenu.addItem( new CMenuForwarder("mainmenu.service", true, "", &service) );


	mainSettings.addItem( new CMenuSeparator() );
	mainSettings.addItem( new CMenuForwarder("menu.back") );
	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.savesettingsnow", true, "", this, "savesettings") );
	mainSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.video", true, "", &videoSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.audio", true, "", &audioSettings) );
	mainSettings.addItem( new CLockedMenuForwarder("parentallock.parentallock", g_settings.parentallock_pincode, true, "", &parentallockSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.network", true, "", &networkSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.language", true, "", &languageSettings ) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.colors", true,"", &colorSettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.keybinding", true,"", &keySettings) );
	mainSettings.addItem( new CMenuForwarder("mainsettings.misc", true, "", &miscSettings ) );
}

void CNeutrinoApp::InitServiceSettings(CMenuWidget &service)
{
	service.addItem( new CMenuSeparator() );
	service.addItem( new CMenuForwarder("menu.back") );
	service.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	//servicescan
	CMenuWidget* TSScan = new CMenuWidget("servicemenu.scants", "settings.raw");
	TSScan->addItem( new CMenuForwarder("menu.back") );
	TSScan->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	service.addItem( new CMenuForwarder("bouqueteditor.name", true, "", new CBEBouquetWidget( new CNeutrinoBouquetEditorEvents(this))));
	CMenuOptionChooser* oj = new CMenuOptionChooser("scants.bouquet", &g_settings.scan_bouquet, true );
	oj->addOption(256, "scants.bouquet_leave");
	oj->addOption(512, "scants.bouquet_create");
	oj->addOption(1024, "scants.bouquet_erase");
	TSScan->addItem( oj );

	if (atoi(getenv("fe"))==1)
	{// only sat-params....
		oj = new CMenuOptionChooser("scants.astra", &g_settings.scan_astra, true );
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		TSScan->addItem( oj );
		oj = new CMenuOptionChooser("scants.hotbird", &g_settings.scan_eutel, true );
		oj->addOption(0, "options.off");
		oj->addOption(2, "options.on");
		TSScan->addItem( oj );
		oj = new CMenuOptionChooser("scants.kopernikus", &g_settings.scan_kopernikus, true );
		oj->addOption(0, "options.off");
		oj->addOption(4, "options.on");
		TSScan->addItem( oj );
		oj = new CMenuOptionChooser("scants.digituerk", &g_settings.scan_digituerk, true );
		oj->addOption(0, "options.off");
		oj->addOption(8, "options.on");
		TSScan->addItem( oj );
	}
	TSScan->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	TSScan->addItem( new CMenuForwarder("scants.startnow", true, "", g_ScanTS) );
	service.addItem( new CMenuForwarder("servicemenu.scants", true, "", TSScan) );


	//kabel-lnb-settings
	if (atoi(getenv("fe"))==1)
	{// only sat-params....
		//todo
	}
	else
	{//kabel
		CMenuWidget* cableSettings = new CMenuWidget("servicemenu.cablesetup", "settings.raw");
		cableSettings->addItem( new CMenuSeparator() );
		cableSettings->addItem( new CMenuForwarder("menu.back") );
		cableSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

		static int dummy = 0;
		FILE* fd = fopen("/var/etc/.specinv", "r");
		if(fd)
		{
			dummy=1;
			fclose(fd);
		}
		oj = new CMenuOptionChooser("cablesetup.spectralInversion", &dummy, true, new CCableSpectalInversionNotifier );
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		cableSettings->addItem( oj );
		service.addItem( new CMenuForwarder("servicemenu.cablesetup", true, "", cableSettings) );
	}

	//ucodecheck
	service.addItem( new CMenuForwarder("servicemenu.ucodecheck", true, "", UCodeChecker ) );
	//	miscSettings.addItem( new CMenuForwarder("miscsettings.reload_services", true, "", g_ReloadServices ) );


	//softupdate
	if (softupdate)
	{
		printf("init soft-update-stuff\n");
		CMenuWidget* updateSettings = new CMenuWidget("servicemenu.update", "softupdate.raw", 450);
		updateSettings->addItem( new CMenuSeparator() );
		updateSettings->addItem( new CMenuForwarder("menu.back") );
		updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );

		//get current flash-version
		FILE* fd = fopen("/.version", "r");
		strcpy(g_settings.softupdate_currentversion, "1.0.0");

		if(!fd)
		{
			fd = fopen("/var/etc/version", "r");
			if(!fd)
			{
				perror("cannot read flash-versioninfo");
			}
			else
			{
				if(fgets(g_settings.softupdate_currentversion,90,fd)==NULL)
					fclose(fd);
			//printf("versiondata: ->%s<-\n", g_settings.softupdate_currentversion);
			for (unsigned int x=0;x<strlen(g_settings.softupdate_currentversion);x++)
			{
				if( (g_settings.softupdate_currentversion[x]!='.') && ((g_settings.softupdate_currentversion[x]>'9') || (g_settings.softupdate_currentversion[x]<'0') ) )
				{
					g_settings.softupdate_currentversion[x]=0;
				}
			}

			}
		}
		else
		{
			if(fgets(g_settings.softupdate_currentversion,90,fd)==NULL)
				fclose(fd);
		}


		printf("current flash-version: %s\n", g_settings.softupdate_currentversion);
		updateSettings->addItem( new CMenuForwarder("flashupdate.currentversion", false, (char*) &g_settings.softupdate_currentversion, NULL ));

		CMenuOptionChooser *oj = new CMenuOptionChooser("flashupdate.updatemode", &g_settings.softupdate_mode,true);
		oj->addOption(0, "flashupdate.updatemode_manual");
		oj->addOption(1, "flashupdate.updatemode_internet");
		updateSettings->addItem( oj );

		updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "flashupdate.proxyserver_sep") );

		CStringInputSMS*	updateSettings_proxy= new CStringInputSMS("flashupdate.proxyserver", g_settings.softupdate_proxyserver, 23,
		                                       "flashupdate.proxyserver_hint1", "flashupdate.proxyserver_hint2",
		                                       "abcdefghijklmnopqrstuvwxyz0123456789-.: ");
		updateSettings->addItem( new CMenuForwarder("flashupdate.proxyserver", true, g_settings.softupdate_proxyserver, updateSettings_proxy ) );

		CStringInputSMS*	updateSettings_proxyuser= new CStringInputSMS("flashupdate.proxyusername", g_settings.softupdate_proxyusername, 23,
		        "flashupdate.proxyusername_hint1", "flashupdate.proxyusername_hint2",
		        "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
		updateSettings->addItem( new CMenuForwarder("flashupdate.proxyusername", true, g_settings.softupdate_proxyusername, updateSettings_proxyuser ) );

		CStringInputSMS*	updateSettings_proxypass= new CStringInputSMS("flashupdate.proxypassword", g_settings.softupdate_proxypassword, 20,
		        "flashupdate.proxypassword_hint1", "flashupdate.proxypassword_hint2",
		        "abcdefghijklmnopqrstuvwxyz0123456789!""§$%&/()=?-. ");
		updateSettings->addItem( new CMenuForwarder("flashupdate.proxypassword", true, g_settings.softupdate_proxypassword, updateSettings_proxypass ) );

		updateSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		updateSettings->addItem( new CMenuForwarder("flashupdate.checkupdate", true, "", g_Update ) );

		service.addItem( new CMenuForwarder("servicemenu.update", true, "", updateSettings ) );
	}

}

void CNeutrinoApp::InitMiscSettings(CMenuWidget &miscSettings)
{
	miscSettings.addItem( new CMenuSeparator() );
	miscSettings.addItem( new CMenuForwarder("menu.back") );
	miscSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuOptionChooser *oj = new CMenuOptionChooser("miscsettings.boxtype", &g_settings.box_Type, false, NULL, false );
	oj->addOption(1, "Nokia");
	oj->addOption(2, "Sagem");
	oj->addOption(3, "Philips");
	miscSettings.addItem( oj );

	oj = new CMenuOptionChooser("miscsettings.shutdown_real", &g_settings.shutdown_real, true );
	oj->addOption(1, "options.off");
	oj->addOption(0, "options.on");
	miscSettings.addItem( oj );

	if (fromflash)
	{
		static int dummy = 0;
		FILE* fd = fopen("/var/etc/.neutrino", "r");
		if(fd)
		{
			dummy=1;
			fclose(fd);
		}
		oj = new CMenuOptionChooser("miscsettings.startneutrinodirect", &dummy, true, new CStartNeutrinoDirectNotifier );
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		miscSettings.addItem( oj );
	}
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

	n = scandir(DATADIR "/neutrino/locale", &namelist, 0, alphasort);
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



	CMenuOptionChooser* oj = new CMenuOptionChooser("videomenu.videosignal", &g_settings.video_Signal, true, videoSetupNotifier);
	oj->addOption(1, "videomenu.videosignal_rgb");
	oj->addOption(2, "videomenu.videosignal_svideo");
	oj->addOption(0, "videomenu.videosignal_composite");

	videoSettings.addItem( oj );

	oj = new CMenuOptionChooser("videomenu.videoformat", &g_settings.video_Format, true, videoSetupNotifier);
	oj->addOption(2, "videomenu.videoformat_43");
	oj->addOption(1, "videomenu.videoformat_169");
	oj->addOption(0, "videomenu.videoformat_autodetect");

	if (g_settings.video_Format==0) // autodetect has to be initialized
	{
		videoSetupNotifier->changeNotify("videomenu.videoformat", NULL);
	}

	videoSettings.addItem( oj );

	oj = new CMenuOptionChooser("videomenu.vcrswitch", &g_settings.vcr_AutoSwitch, true, NULL);
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	videoSettings.addItem( oj );

	videoSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	videoSettings.addItem( new CMenuForwarder("videomenu.screensetup", true, "", g_ScreenSetup ) );
}

void CNeutrinoApp::InitParentalLockSettings(CMenuWidget &parentallockSettings)
{
	parentallockSettings.addItem( new CMenuSeparator() );
	parentallockSettings.addItem( new CMenuForwarder("menu.back") );
	parentallockSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuOptionChooser* oj = new CMenuOptionChooser("parentallock.prompt", &g_settings.parentallock_prompt, true);
	oj->addOption(PARENTALLOCK_PROMPT_NEVER         , "parentallock.never");
	oj->addOption(PARENTALLOCK_PROMPT_ONSTART       , "parentallock.onstart");
	oj->addOption(PARENTALLOCK_PROMPT_CHANGETOLOCKED, "parentallock.changetolocked");
	oj->addOption(PARENTALLOCK_PROMPT_ONSIGNAL      , "parentallock.onsignal");
	parentallockSettings.addItem( oj );

	oj = new CMenuOptionChooser("parentallock.lockage", &g_settings.parentallock_lockage, true);
	oj->addOption(12, "parentallock.lockage12");
	oj->addOption(16, "parentallock.lockage16");
	oj->addOption(18, "parentallock.lockage18");
	parentallockSettings.addItem( oj );

	CPINChangeWidget* pinChangeWidget = new CPINChangeWidget("parentallock.changepin", g_settings.parentallock_pincode, 4, "parentallock.changepin_hint1", "");
	parentallockSettings.addItem( new CMenuForwarder("parentallock.changepin", true, g_settings.parentallock_pincode, pinChangeWidget));
}

void CNeutrinoApp::InitNetworkSettings(CMenuWidget &networkSettings)
{
	networkSettings.addItem( new CMenuSeparator() );
	networkSettings.addItem( new CMenuForwarder("menu.back") );
	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuOptionChooser* oj = new CMenuOptionChooser("networkmenu.setuponstartup", &g_settings.networkSetOnStartup, true);
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");

	networkSettings.addItem( oj );
	networkSettings.addItem( new CMenuForwarder("networkmenu.test", true, "", this, "networktest") );
	networkSettings.addItem( new CMenuForwarder("networkmenu.setupnow", true, "", this, "network") );

	networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CStringInput*	networkSettings_NetworkIP= new CStringInput("networkmenu.ipaddress", g_settings.network_ip, 3*4+3, "ipsetup.hint_1", "ipsetup.hint_2", "0123456789. ", MyIPChanger);
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


	if(g_settings.network_streaming_use)
	{
		networkSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		/*
		oj = new CMenuOptionChooser("networkmenu.usestreamserver", &g_settings.network_streaming_use, true);
		oj->addOption(0, "options.off");
		oj->addOption(1, "options.on");
		networkSettings.addItem( oj );
		*/
		CStringInput*	networkSettings_streamingserver= new CStringInput("networkmenu.streamingserver", g_settings.network_streamingserver, 24, "ipsetup.hint_1", "ipsetup.hint_2");
		CStringInput*	networkSettings_streamingserverport= new CStringInput("networkmenu.streamingserverport", g_settings.network_streamingserverport, 6, "ipsetup.hint_1", "ipsetup.hint_2","1234567890 ");

		networkSettings.addItem( new CMenuForwarder("networkmenu.streamingserver", true, g_settings.network_streamingserver,networkSettings_streamingserver));
		networkSettings.addItem( new CMenuForwarder("networkmenu.streamingserverport", true, g_settings.network_streamingserverport,networkSettings_streamingserverport));
	}
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
	CStringInput*	keySettings_repeat_genericblocker= new CStringInput("keybindingmenu.repeatblockgeneric", g_settings.repeat_genericblocker, 3, "repeatblocker.hint_1", "repeatblocker.hint_2", "0123456789 ", keySetupNotifier);
	keySetupNotifier->changeNotify("initial", NULL);

	CKeyChooser*	keySettings_tvradio_mode = new CKeyChooser(&g_settings.key_tvradio_mode, "keybindingmenu.tvradiomode_head", "settings.raw");
	CKeyChooser*	keySettings_channelList_pageup = new CKeyChooser(&g_settings.key_channelList_pageup, "keybindingmenu.pageup_head", "settings.raw");
	CKeyChooser*	keySettings_channelList_pagedown = new CKeyChooser(&g_settings.key_channelList_pagedown, "keybindingmenu.pagedown_head", "settings.raw");
	CKeyChooser*	keySettings_channelList_cancel = new CKeyChooser(&g_settings.key_channelList_cancel, "keybindingmenu.cancel_head", "settings.raw");
	CKeyChooser*	keySettings_quickzap_up = new CKeyChooser(&g_settings.key_quickzap_up, "keybindingmenu.channelup_head",   "settings.raw");
	CKeyChooser*	keySettings_quickzap_down = new CKeyChooser(&g_settings.key_quickzap_down, "keybindingmenu.channeldown_head", "settings.raw");
	CKeyChooser*	keySettings_bouquet_up = new CKeyChooser(&g_settings.key_bouquet_up, "keybindingmenu.bouquetup_head",   "settings.raw");
	CKeyChooser*	keySettings_bouquet_down = new CKeyChooser(&g_settings.key_bouquet_down, "keybindingmenu.bouquetdown_head", "settings.raw");
	CKeyChooser*	keySettings_subchannel_up = new CKeyChooser(&g_settings.key_subchannel_up, "keybindingmenu.subchannelup_head",   "settings.raw");
	CKeyChooser*	keySettings_subchannel_down = new CKeyChooser(&g_settings.key_subchannel_down, "keybindingmenu.subchanneldown_head", "settings.raw");


	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.RC") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.repeatblock", true, "", keySettings_repeatBlocker ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.repeatblockgeneric", true, "", keySettings_repeat_genericblocker ));

	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.modechange") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.tvradiomode", true, "", keySettings_tvradio_mode ));

	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.channellist") );
	CMenuOptionChooser *oj = new CMenuOptionChooser("keybindingmenu.bouquethandling" , &g_settings.bouquetlist_mode, true );
	oj->addOption(0, "keybindingmenu.bouquetchannels_on_ok");
	oj->addOption(1, "keybindingmenu.bouquetlist_on_ok");
	oj->addOption(2, "keybindingmenu.allchannels_on_ok");
	keySettings.addItem( oj );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.pageup", true, "", keySettings_channelList_pageup ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.pagedown", true, "", keySettings_channelList_pagedown ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.cancel", true, "", keySettings_channelList_cancel ));
	keySettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.quickzap") );
	keySettings.addItem( new CMenuForwarder("keybindingmenu.channelup", true, "", keySettings_quickzap_up ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.channeldown", true, "", keySettings_quickzap_down ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.bouquetup", true, "", keySettings_bouquet_up ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.bouquetdown", true, "", keySettings_bouquet_down ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.subchannelup", true, "", keySettings_subchannel_up ));
	keySettings.addItem( new CMenuForwarder("keybindingmenu.subchanneldown", true, "", keySettings_subchannel_down ));
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
				NVODSelector.addItem( new CMenuForwarder(subChannels.list[count].subservice_name, true, "", NVODChanger, nvod_id, false, (count<9)? (count+1) : CRCInput::RC_nokey ), (count == subChannels.selected) );
			}
		}
		NVODSelector.exec(NULL, "");
	}
}

void CNeutrinoApp::SelectAPID()
{
	g_RemoteControl->CopyPIDs();

	char to_compare[50];
	snprintf( to_compare, 10, "%x", channelList->getActiveChannelOnid_sid() );

	if ( ( strcmp(g_RemoteControl->audio_chans.name, to_compare )== 0 ) &&
	        ( g_RemoteControl->audio_chans.count_apids> 1 ) )
	{
		// wir haben APIDs für diesen Kanal!

		CMenuWidget APIDSelector("apidselector.head", "audio.raw", 300);
		APIDSelector.addItem( new CMenuSeparator() );

		for(int count=0;count<g_RemoteControl->audio_chans.count_apids;count++)
		{
			char apid[5];
			sprintf(apid, "%d", count);
			APIDSelector.addItem( new CMenuForwarder(g_RemoteControl->audio_chans.apids[count].name, true,
								  "", APIDChanger, apid, false, (count<9)? (count+1) : CRCInput::RC_nokey ), (count == g_RemoteControl->audio_chans.selected) );
		}
		APIDSelector.exec(NULL, "");
	}
}

void CNeutrinoApp::ShowStreamFeatures()
{
	CMenuWidget StreamFeatureSelector("streamfeatures.head", "features.raw", 350);
	StreamFeatureSelector.addItem( new CMenuSeparator() );

	char id[5];
	int cnt = 0;
	int enabled_count = 0;

	g_RemoteControl->CopyPIDs();
    g_PluginList->loadPlugins();

	for(unsigned int count=0;count<g_PluginList->getNumberOfPlugins();count++)
	{
    	if ( g_PluginList->getType(count)== 2 )
    	{
    		// zB vtxt-plugins

			sprintf(id, "%d", count);

			bool enable_it = ( ( !g_PluginList->getVTXT(count) )  || (g_RemoteControl->vtxtpid!=0) );
			if ( enable_it )
				enabled_count++;

			StreamFeatureSelector.addItem( new CMenuForwarder(g_PluginList->getName(count), enable_it, "",
										   StreamFeaturesChanger, id, false, (cnt== 0) ? CRCInput::RC_blue : CRCInput::RC_nokey, (cnt== 0)?"blau.raw":""), (cnt == 0) && enable_it );
			cnt++;
		}
	}

	if (cnt>0)
	{
		StreamFeatureSelector.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	}

	sprintf(id, "%d", -1);
	StreamFeatureSelector.addItem( new CMenuForwarder("streamfeatures.info", true, "",
									   StreamFeaturesChanger, id, true, CRCInput::RC_help, "help_small.raw"), (enabled_count==0) );
	StreamFeatureSelector.exec(NULL, "");
}


void CNeutrinoApp::InitZapper()
{

	g_InfoViewer->start();
	g_EpgData->start();

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

int CNeutrinoApp::run(int argc, char **argv)
{
	g_Controld = new CControldClient;

	if(!loadSetup())
	{
		//setup default if configfile not exists
		setupDefaults();
		printf("using defaults...\n\n");
	}
	//get dbox-type everytime!
	g_settings.box_Type = g_Controld->getBoxType();
	//printf("got boxtype from controld: %d\n", g_settings.box_Type);

	CmdParser(argc, argv);

	g_Fonts = new FontsDef;
	SetupFonts();

	ClearFrameBuffer();

	g_Locale = new CLocaleManager;
	g_RCInput = new CRCInput;
	g_lcdd = new CLcddClient;
	g_Zapit = new CZapitClient;
	g_Sectionsd = new CSectionsdClient;

	g_RemoteControl = new CRemoteControl;
	g_EpgData = new CEpgData;
	g_InfoViewer = new CInfoViewer;
	g_StreamInfo = new CStreamInfo;
	g_ScanTS = new CScanTs;
	g_ScreenSetup = new CScreenSetup;
	g_EventList = new EventList;
	g_Update = new CFlashUpdate;

	g_PluginList = new CPlugins;
	g_PluginList->setPluginDir(PLUGINDIR);

	#ifdef USEACTIONLOG
		g_ActionLog = new CActionLog;
		g_ActionLog->println("neutrino startup");
	#endif

	//printf("\nCNeutrinoApp::run - objects initialized...\n\n");
	g_Locale->loadLocale(g_settings.language);

	colorSetupNotifier	= new CColorSetupNotifier;
	audioSetupNotifier	= new CAudioSetupNotifier;
	videoSetupNotifier	= new CVideoSetupNotifier;
	APIDChanger			= new CAPIDChangeExec;
	UCodeChecker		= new CUCodeCheckExec;
	NVODChanger			= new CNVODChangeExec;
	StreamFeaturesChanger = new CStreamFeaturesChangeExec;
	MyIPChanger			= new CIPChangeNotifier;

	colorSetupNotifier->changeNotify("initial", NULL);

	setupNetwork();

	channelList = new CChannelList( "channellist.head" );


	//Main settings
	CMenuWidget mainMenu("mainmenu.head", "mainmenue.raw");
	CMenuWidget mainSettings("mainsettings.head", "settings.raw");
	CMenuWidget languageSettings("languagesetup.head", "language.raw");
	CMenuWidget videoSettings("videomenu.head", "video.raw");
	CMenuWidget audioSettings("audiomenu.head", "audio.raw");
	CMenuWidget parentallockSettings("parentallock.parentallock", "lock.raw", 500);
	CMenuWidget networkSettings("networkmenu.head", "network.raw");
	CMenuWidget colorSettings("colormenu.head", "colors.raw");
	CMenuWidget keySettings("keybindingmenu.head", "keybinding.raw", 400, 520);
	CMenuWidget miscSettings("miscsettings.head", "settings.raw");
	CMenuWidget service("servicemenu.head", "settings.raw");

	InitMainMenu(mainMenu, mainSettings, audioSettings, parentallockSettings, networkSettings,
	             colorSettings, keySettings, videoSettings, languageSettings, miscSettings, service);

	//service
	InitServiceSettings(service);

	//language Setup
	InitLanguageSettings(languageSettings);

	//misc Setup
	InitMiscSettings(miscSettings);
	miscSettings.setOnPaintNotifier(this);

	//audio Setup
	InitAudioSettings(audioSettings, audioSetupNotifier);

	//video Setup
	InitVideoSettings(videoSettings, videoSetupNotifier);
	videoSettings.setOnPaintNotifier(this);

	// Parentallock settings
	InitParentalLockSettings( parentallockSettings);

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

	current_volume= g_Controld->getVolume();
	g_Controld->registerEvent(CControldClient::EVT_VOLUMECHANGED, 222, NEUTRINO_UDS_NAME);

	AudioMute( g_Controld->getMute(), true );
	g_Controld->registerEvent(CControldClient::EVT_MUTECHANGED, 222, NEUTRINO_UDS_NAME);

	g_Controld->registerEvent(CControldClient::EVT_MODECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VCRCHANGED, 222, NEUTRINO_UDS_NAME);

	g_Sectionsd->registerEvent(CSectionsdClient::EVT_TIMESET, 222, NEUTRINO_UDS_NAME);

	RealRun(mainMenu);

	ExitRun();
	return 0;
}

void CNeutrinoApp::RealRun(CMenuWidget &mainMenu)
{
	while( true )
	{
		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data );

		if ( msg == messages::STANDBY_ON )
		{
			if ( mode != mode_standby )
			{
				// noch nicht im Standby-Mode...
				standbyMode( true );
			}
			g_RCInput->clearMsg();
		}

		if ( msg == messages::STANDBY_OFF )
		{
			if ( mode == mode_standby )
			{
				// WAKEUP
				standbyMode( false );
			}
			g_RCInput->clearMsg();
		}

		else if ( msg == messages::SHUTDOWN )
		{
			// AUSSCHALTEN...
			ExitRun();
		}

		else if ( msg == messages::VCR_ON )
		{
			if  ( mode != mode_scart )
			{
				// noch nicht im Scart-Mode...
				scartMode( true );
			}
		}

		else if ( msg == messages::VCR_OFF )
		{
			if ( mode == mode_scart )
			{
				// noch nicht im Scart-Mode...
				scartMode( false );
			}
		}
		else
		{
			if ( ( mode == mode_tv ) || ( ( mode == mode_radio ) ) )
			{
				if ( msg == messages::SHOW_EPG )
				{
					// show EPG

					g_EpgData->show( channelList->getActiveChannelName(),
			        		         channelList->getActiveChannelOnid_sid() );

				}
				else if ( msg == g_settings.key_tvradio_mode )
				{
					if ( mode == mode_tv )
					{
						radioMode();
					}
					else if ( mode == mode_radio )
					{
						tvMode();
					}
				}
				else if ( msg == CRCInput::RC_setup )
				{
					mainMenu.exec(NULL, "");
				}
				if ( msg == CRCInput::RC_ok )
				{
					int bouqMode = g_settings.bouquetlist_mode;//bsmChannels;

					if ((bouquetList!=NULL) && (bouquetList->Bouquets.size() == 0 ))
					{
						printf("bouquets are empty\n");
						bouqMode = bsmAllChannels;
					}
					if ((bouquetList!=NULL) && (bouqMode == 1/*bsmBouquets*/))
					{
						bouquetList->exec(true);
					}
					else if ((bouquetList!=NULL) && (bouqMode == 0/*bsmChannels*/))
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
				else if ( msg == CRCInput::RC_red )
				{	// eventlist
					g_EventList->exec(channelList->getActiveChannelOnid_sid(), channelList->getActiveChannelName());
				}
				else if ( msg == CRCInput::RC_blue )
				{	// streaminfo
					ShowStreamFeatures();
				}
				else if ( msg == CRCInput::RC_green )
				{	// APID
					SelectAPID();
				}
				else if ( msg == CRCInput::RC_yellow )
				{	// NVODs
					SelectNVOD();
				}
				else if ( ( msg == g_settings.key_quickzap_up ) || ( msg == g_settings.key_quickzap_down ) )
				{
					//quickzap
					channelList->quickZap( msg );
				}
				else if ( ( msg == CRCInput::RC_help ) ||
						  ( msg == messages::SHOW_INFOBAR ) )
				{
					// show Infoviewer
					g_InfoViewer->showTitle( channelList->getActiveChannelNumber(),
					                         channelList->getActiveChannelName(),
				    	                     channelList->getActiveChannelOnid_sid() );
				}
				else if ( ( msg >= CRCInput::RC_0 ) && ( msg <= CRCInput::RC_9 ))
				{ //numeric zap
					channelList->numericZap( msg );
				}
				else if ( msg == g_settings.key_subchannel_up )
				{
					string sc = g_RemoteControl->subChannelUp();
					showSubchan(sc);
				}
				else if ( msg == g_settings.key_subchannel_down )
				{
					string sc = g_RemoteControl->subChannelDown();
					showSubchan(sc);
				}
				else
				{
					handleMsg( msg, data );
				}

			}
			else
			{
				// mode == mode_scart
				if ( msg == CRCInput::RC_home )
				{
					if ( mode == mode_scart )
					{
						// noch nicht im Scart-Mode...
						scartMode( false );
					}
				}
				else
				{
					handleMsg( msg, data );
				}
			}
		}
	}
}

int CNeutrinoApp::handleMsg(uint msg, uint data)
{
	int res;

	res = g_InfoViewer->handleMsg(msg, data);

	if ( res != messages_return::unhandled )
		return res;

    if ( msg == messages::EVT_VCRCHANGED )
	{
		if ( g_settings.vcr_AutoSwitch == 1 )
		{
			if ( data != VCR_STATUS_OFF )
				g_RCInput->pushbackMsg( messages::VCR_ON, 0 );
			else
				g_RCInput->pushbackMsg( messages::VCR_OFF, 0 );
		}
		return messages_return::handled | messages_return::cancel_info;
	}
	else
	if ( msg == CRCInput::RC_standby )
	{
		// trigger StandBy
		struct timeval tv;
		gettimeofday( &tv, NULL );
		standby_pressed_at = (tv.tv_sec*1000000) + tv.tv_usec;

		if ( mode == mode_standby )
		{
        	g_RCInput->insertMsgAtTop( messages::STANDBY_OFF, 0 );
		}
		else if ( !g_settings.shutdown_real )
		{
			int timeout = 5;
			int timeout1 = 5;

			sscanf(g_settings.repeat_blocker, "%d", &timeout);
			timeout = int(timeout/100.0) + 5;
			sscanf(g_settings.repeat_genericblocker, "%d", &timeout1);
			timeout1 = int(timeout1/100.0) + 5;
			if(timeout1>timeout)
				timeout=timeout1;

			uint msg; uint data;
			int diff = 0;
			long long endtime;

			do
			{
				g_RCInput->getMsg( &msg, &data, timeout );

				if ( msg != CRCInput::RC_timeout )
				{
					gettimeofday( &tv, NULL );
					endtime = (tv.tv_sec*1000000) + tv.tv_usec;
					diff = int((endtime - standby_pressed_at)/100000. );
				}

			} while ( ( msg != CRCInput::RC_timeout ) && ( diff < 10 ) );

			g_RCInput->insertMsgAtTop( ( diff >= 10 ) ? messages::SHUTDOWN : messages::STANDBY_ON, 0 );
        }
        else
        {
        	g_RCInput->insertMsgAtTop( messages::SHUTDOWN, 0 );
		}
		return messages_return::cancel_all | messages_return::handled;
	}
	else if ( msg == CRCInput::RC_standby_release )
	{
		struct timeval tv;
		gettimeofday( &tv, NULL );
		long long endtime = (tv.tv_sec*1000000) + tv.tv_usec;
		int diff = int((endtime - standby_pressed_at)/100000. );
		if ( diff >= 10 )
		{
        	g_RCInput->insertMsgAtTop( messages::SHUTDOWN, 0 );
        	return messages_return::cancel_all | messages_return::handled;
        }
	}
	else if ( ( msg == CRCInput::RC_plus ) ||
			  ( msg == CRCInput::RC_minus ) )
	{
		//volume
		setVolume( msg, ( mode != mode_scart ) );
		return messages_return::handled;
	}
	else if ( msg == CRCInput::RC_spkr )
	{
		//mute
		AudioMute( !current_muted );
		return messages_return::handled;
	}
	else if ( msg == messages::EVT_VOLCHANGED )
	{
		current_volume = data;
		return messages_return::handled;
	}
	else if ( msg == messages::EVT_MUTECHANGED )
	{
		AudioMute( (bool)data, true );
		return messages_return::handled;
	}

	return messages_return::unhandled;
}



void CNeutrinoApp::ExitRun()
{
	#ifdef USEACTIONLOG
		g_ActionLog->println("neutrino shutdown");
	#endif

	printf("neutrino exit\n");
	//shutdown screen
	g_lcdd->shutdown();

	//memset(frameBuffer.lfb, 255, frameBuffer.Stride()*576);
	for(int x=0;x<256;x++)
		g_FrameBuffer->paletteSetColor(x, 0x000000, 0xffff);
	g_FrameBuffer->paletteSet();

	g_FrameBuffer->loadPicture2Mem("shutdown.raw", g_FrameBuffer->lfb );
	g_FrameBuffer->loadPal("shutdown.pal");

	saveSetup();
	g_Controld->shutdown();
	sleep(55555);
}

bool CNeutrinoApp::onPaintNotify(string MenuName)
{
	if(MenuName == "videomenu.head")
	{//aktuelle werte vom controld holen...
		g_settings.video_Signal = g_Controld->getVideoOutput();
		g_settings.video_Format = g_Controld->getVideoFormat();
	}

	return false;
}

void CNeutrinoApp::AudioMute( bool newValue, bool isEvent )
{
	int dx = 40;
	int dy = 40;
	int x = g_settings.screen_EndX-dx;
	int y = g_settings.screen_StartY;

	if ( newValue != current_muted )
	{
		current_muted = newValue;

		if ( !isEvent )
		{
			if ( current_muted )
				g_Controld->Mute();
			else
				g_Controld->UnMute();
		}
	}

	if ( isEvent && ( mode != mode_scart ) )
	{
		// anzeigen NUR, wenn es vom Event kommt
		if ( current_muted )
		{
			g_FrameBuffer->paintBoxRel(x, y, dx, dy, COL_INFOBAR);
			g_FrameBuffer->paintIcon("mute.raw", x+5, y+5);
		}
		else
			g_FrameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
	}
}

void CNeutrinoApp::setVolume(int key, bool bDoPaint)
{
	int dx = 256;
	int dy = 40;
	int x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
	int y = g_settings.screen_EndY- 100;

	unsigned char* pixbuf;

	if (bDoPaint)
	{
		pixbuf= new unsigned char[ dx * dy ];
		if (pixbuf!= NULL)
			g_FrameBuffer->SaveScreen(x, y, dx, dy, pixbuf);
		g_FrameBuffer->paintIcon("volume.raw",x,y, COL_INFOBAR);
	}

	uint msg = key;
	uint data;

	do
	{
		if (msg==CRCInput::RC_plus)
		{
			if (current_volume<100)
			{
				current_volume += 5;
			}
			g_Controld->setVolume(current_volume);
		}
		else if (msg==CRCInput::RC_minus)
		{
			if (current_volume>0)
			{
				current_volume -= 5;
			}
			g_Controld->setVolume(current_volume);
		}
		else
		{
			if ( (msg!=CRCInput::RC_ok) || (msg!=CRCInput::RC_home) )
			{
				if ( neutrino->handleMsg( msg, data ) & messages_return::unhandled )
				{
					g_RCInput->pushbackMsg( msg, data );

					msg= CRCInput::RC_timeout;
				}
			}
		}

		if (bDoPaint)
		{
			int vol = current_volume<<1;
			g_FrameBuffer->paintBoxRel(x+40, y+12, 200, 15, COL_INFOBAR+1);
			g_FrameBuffer->paintBoxRel(x+40, y+12, vol, 15, COL_INFOBAR+3);
        }

		if ( msg != CRCInput::RC_timeout )
		{
			g_RCInput->getMsg( &msg, &data, 30 );
		}

	}
	while ( msg != CRCInput::RC_timeout );

    if ( (bDoPaint) && (pixbuf!= NULL) )
		g_FrameBuffer->RestoreScreen(x, y, dx, dy, pixbuf);
}

void CNeutrinoApp::tvMode( bool rezap )
{
	if( mode == mode_tv )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		g_lcdd->setMode(CLcddClient::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}

	mode = mode_tv;
	NeutrinoMode = mode_tv;
	#ifdef USEACTIONLOG
		g_ActionLog->println("mode: tv");
	#endif

	//printf( "tv-mode\n" );

	memset(g_FrameBuffer->lfb, 255, g_FrameBuffer->Stride()*576);
	g_FrameBuffer->useBackground(false);

    if ( rezap )
	{
		g_RemoteControl->tvMode();
		firstChannel();
		channelsInit();
		channelList->zapTo( firstchannel.chan_nr -1 );
	}
}

void CNeutrinoApp::scartMode( bool bOnOff )
{
	#ifdef USEACTIONLOG
		g_ActionLog->println( ( bOnOff ) ? "mode: scart on" : "mode: scart off" );
	#endif

	//printf( ( bOnOff ) ? "mode: scart on\n" : "mode: scart off\n" );

	if ( bOnOff )
	{
		// SCART AN

		memset(g_FrameBuffer->lfb, 255, g_FrameBuffer->Stride()*576);
		g_Controld->setScartMode( 1 );

		lastMode = mode;
		mode = mode_scart;
	}
	else
	{
	    // SCART AUS
		g_Controld->setScartMode( 0 );

        mode = mode_unknown;

		//re-set mode
		if ( lastMode == mode_radio )
		{
			radioMode( false );
		}
		else if ( lastMode == mode_tv )
		{
			tvMode( false );
		}
		else if ( lastMode == mode_standby )
		{
			standbyMode( true );
		}
	}
}

void CNeutrinoApp::standbyMode( bool bOnOff )
{
	#ifdef USEACTIONLOG
		g_ActionLog->println( ( bOnOff ) ? "mode: standby on" : "mode: standby off" );
	#endif

	//printf( ( bOnOff ) ? "mode: standby on\n" : "mode: standby off\n" );

	if ( bOnOff )
	{
		// STANDBY AN

		if( mode == mode_scart )
		{
			g_Controld->setScartMode( 0 );
		}

		memset(g_FrameBuffer->lfb, 255, g_FrameBuffer->Stride()*576);

		g_lcdd->setMode(CLcddClient::MODE_STANDBY);
		g_Controld->videoPowerDown(true);

		lastMode = mode;
		mode = mode_standby;
	}
	else
	{
	    // STANDBY AUS

		g_lcdd->setMode(CLcddClient::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);

		mode = mode_unknown;

		//re-set mode
		if ( lastMode == mode_radio )
		{
			radioMode( false );
		}
		else
		{
			tvMode( false );
		}
	}
}

void CNeutrinoApp::radioMode( bool rezap = true )
{
	if ( mode==mode_radio )
	{
		return;
	}
	else if( mode == mode_scart )
	{
		g_Controld->setScartMode( 0 );
	}
	else if( mode == mode_standby )
	{
		g_lcdd->setMode(CLcddClient::MODE_TVRADIO);
		g_Controld->videoPowerDown(false);
	}

	mode = mode_radio;
	NeutrinoMode = mode_radio;
	#ifdef USEACTIONLOG
		g_ActionLog->println("mode: radio");
	#endif

	g_FrameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	g_FrameBuffer->loadBackground("radiomode.raw");
	g_FrameBuffer->useBackground(true);
	g_FrameBuffer->paintBackground();

	if ( rezap )
	{
		firstChannel();
		g_RemoteControl->radioMode();
		firstChannel();
		channelsInit();
		channelList->zapTo( firstchannel.chan_nr -1 );
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
	int returnval = menu_return::RETURN_REPAINT;

	if(actionKey=="theme_neutrino")
	{
		setupColors_neutrino();
		colorSetupNotifier->changeNotify("initial", NULL);
	}
	else if(actionKey=="theme_classic")
	{
		setupColors_classic();
		colorSetupNotifier->changeNotify("initial", NULL);
	}
	else if(actionKey=="shutdown")
	{
		ExitRun();
	}
	else if(actionKey=="tv")
	{
		tvMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="radio")
	{
		radioMode();
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="scart")
	{
		g_RCInput->pushbackMsg( messages::VCR_ON, 0 );
		returnval = menu_return::RETURN_EXIT_ALL;
	}
	else if(actionKey=="network")
	{
		setupNetwork( true );
	}
	else if(actionKey=="networktest")
	{
		 testNetwork( );
	}

	else if(actionKey=="savesettings")
	{
		g_Controld->saveSettings();
		saveSetup();
	}

	return returnval;
}

bool CNeutrinoApp::changeNotify(string OptionName)
{
	int port = 0;
	sscanf(g_settings.network_streamingserverport, "%d", &port);

	printf("streaming : %d\n", streamstatus);
	printf("port : %d\n", port);

	int sock_fd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SAI servaddr;
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(port);
	inet_pton(AF_INET, g_settings.network_streamingserver, &servaddr.sin_addr);

	if(connect(sock_fd, (SA *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("[neutrino] -  cannot connect to streamingserver\n");
		return false;
	}
	streaming_commandhead rmsg;
	rmsg.version=1;
	rmsg.command = streamstatus +1;
	write(sock_fd, &rmsg, sizeof(rmsg));
	close(sock_fd);

	return false;
}

	/* This class should be a temporarily work around             */
	/* and should be replaced by standard neutrino event handlers */
	/* (libevent) */
void CNeutrinoBouquetEditorEvents::onBouquetsChanged()
{
//	neutrino->firstChannel();
	neutrino->channelsInit();
//	neutrino->channelList->zapTo( 0 );
}


/**************************************************************************************
*                                                                                     *
*          Main programm - no function here                                           *
*                                                                                     *
**************************************************************************************/
int main(int argc, char **argv)
{
	printf("NeutrinoNG $Id: neutrino.cpp,v 1.199 2002/03/19 20:11:48 obi Exp $\n\n");
	tzset();
	initGlobals();
	neutrino = new CNeutrinoApp;
	return neutrino->run(argc, argv);
}

