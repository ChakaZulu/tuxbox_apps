/*

        $Id: neutrino.cpp,v 1.253 2002/04/27 07:44:07 field Exp $

	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	                   and some other guys
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#include <config.h>

#define NEUTRINO_CPP

#include "global.h"
#include "neutrino.h"

#include "zapit/getservices.h"
#include "daemonc/remotecontrol.h"

#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"

#include "gui/widget/menue.h"
#include "gui/widget/messagebox.h"
#include "gui/widget/colorchooser.h"
#include "gui/widget/keychooser.h"
#include "gui/widget/stringinput.h"
#include "gui/widget/stringinput_ext.h"

#include "gui/color.h"

#include "gui/bedit/bouqueteditor_bouquets.h"
#include "gui/eventlist.h"
#include "gui/channellist.h"
#include "gui/screensetup.h"
#include "gui/gamelist.h"
#include "gui/infoviewer.h"
#include "gui/epgview.h"
#include "gui/update.h"
#include "gui/scan.h"
#include "gui/favorites.h"

#include "system/setting_helpers.h"
#include "system/settings.h"
#include "system/locale.h"

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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#define SA struct sockaddr
#define SAI struct sockaddr_in

using namespace std;


// Globale Variablen - to use import global.h

// I don't like globals, I would have hidden them in classes,
// but if you wanna do it so... ;)

static void initGlobals(void)
{
	g_fontRenderer = NULL;
	g_Fonts = NULL;

	g_RCInput = NULL;
	g_lcdd = NULL;
	g_Controld = NULL;
	g_Timerd = NULL;
	g_Zapit = NULL;
	g_RemoteControl = NULL;

	g_EpgData = NULL;
	g_InfoViewer = NULL;
	g_EventList = NULL;

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
	frameBuffer = CFrameBuffer::getInstance();
	frameBuffer->setIconBasePath(DATADIR "/neutrino/icons/");

	g_fontRenderer = new fontRenderClass;
	SetupFrameBuffer();

	settingsFile = CONFIGDIR "/neutrino.conf";
	scanSettingsFile = CONFIGDIR "/scan.conf";

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

CNeutrinoApp* CNeutrinoApp::getInstance()
{
	static CNeutrinoApp* neutrinoApp = NULL;

	if(!neutrinoApp)
	{
		neutrinoApp = new CNeutrinoApp();
		printf("[neutrino] main Instance created\n");
	}
	else
	{
		//printf("[neutrino] frameBuffer Instace requested\n");
	}
	return neutrinoApp;
}


void CNeutrinoApp::setupNetwork(bool force)
{
	if((g_settings.networkSetOnStartup) || (force))
	{
		printf("doing network setup...\n");
		//setup network
		setNetworkAddress(g_settings.network_ip, g_settings.network_netmask, g_settings.network_broadcast);
		if(strcmp(g_settings.network_nameserver, "000.000.000.000")!=0)
		{
			setNameServer(g_settings.network_nameserver);
		}
		if(strcmp(g_settings.network_defaultgateway, "000.000.000.000")!=0)
		{
			setDefaultGateway(g_settings.network_defaultgateway);
		}
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
	g_settings.shutdown_real = 1;
	g_settings.shutdown_showclock = 1;
	g_settings.show_camwarning = 1;

	//video
	g_settings.video_Signal = 0; //composite?
	g_settings.video_Format = 2; //4:3

	//audio
	g_settings.audio_AnalogMode = 0;
	g_settings.audio_DolbyDigital = 0;

	//vcr
	g_settings.vcr_AutoSwitch = 1;

	//colors
	setupColors_neutrino();

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

	if ( g_info.box_Type == 3 )
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

	g_settings.widget_fade = 1;
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  loadSetup, load the application-settings                   *
*                                                                                     *
**************************************************************************************/
bool CNeutrinoApp::loadSetup(SNeutrinoSettings* load2)
{
	char tmp[0xFFFF];
	bool loadSuccessfull = true;
	if(!load2)
	{
		load2 = &g_settings;
	}

	int fd = open(settingsFile.c_str(), O_RDONLY );

	if (fd==-1)
	{
		printf("error while loading settings: %s\n", settingsFile.c_str() );
		loadSuccessfull = false;
	}
	else if(read(fd, tmp, sizeof(tmp))!=sizeof(SNeutrinoSettings))
	{
		printf("error while loading settings: %s - config from old version?\n", settingsFile.c_str() );
		loadSuccessfull = false;
	}
	else
	{
		memcpy(load2, &tmp, sizeof(SNeutrinoSettings));

		close(fd);
	}

	ifstream is( scanSettingsFile.c_str());
	if ( !is.is_open())
	{
		cout << "error while loading scan-settings!" << endl;
		scanSettings.useDefaults();
	}
	else
	{
		is >> scanSettings;
	}

	return loadSuccessfull;
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

	ofstream os( scanSettingsFile.c_str(), ios::out | ios::trunc);
	try
	{
		os << scanSettings;
	}
	catch (...)
	{
		cout << "error while saving scan-settings!" << endl;
	}
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  firstChannel, get the initial channel                      *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::firstChannel()
{
	string tmp;
	g_Zapit->getLastChannel(tmp, firstchannel.chan_nr, firstchannel.mode);
	strcpy( firstchannel.name, tmp.c_str() );
}

/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  doChecks, check if card fits cam		              *
*                                                                                     *
**************************************************************************************/

void CNeutrinoApp::doChecks()
{
	FILE* fd;
	fd = fopen(UCODEDIR "/avia500.ux", "r");
	if(fd)
		fclose(fd);
	bool ucodes_ok= (fd);
	fd = fopen(UCODEDIR "/avia600.ux", "r");
	if(fd)
		fclose(fd);
	ucodes_ok= ucodes_ok||(fd);
	fd = fopen(UCODEDIR "/ucode.bin", "r");
	if(fd)
		fclose(fd);
	ucodes_ok= ucodes_ok&&(fd);
	fd = fopen(UCODEDIR "/cam-alpha.bin", "r");
	if(fd)
		fclose(fd);
	ucodes_ok= ucodes_ok&&(fd);

	if ( !ucodes_ok )
		ShowMsg ( "messagebox.error", g_Locale->getText("ucodes.failure"), CMessageBox::mbrCancel, CMessageBox::mbCancel, "error.raw" );
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  channelsInit, get the Channellist from daemon              *
*                                                                                     *
**************************************************************************************/
void CNeutrinoApp::channelsInit()
{
	//deleting old channelList for mode-switching.
	delete channelList;
	channelList = new CChannelList( "channellist.head" );

	CZapitClient::BouquetChannelList zapitChannels;
	g_Zapit->getChannels( zapitChannels );
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

	#define FONTNAME "Micron"
	#define FONTFILE "micron"

	fontName = FONTNAME;
	fontFile = FONTDIR "/" FONTFILE;
	fontsSizeOffset = 0;

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
		else if ( !strcmp(argv[x], "-font"))
		{
			if ( ( x + 3 ) < argc )
			{
				fontFile = argv[x+ 1];
				fontName = argv[x+ 2];
				fontsSizeOffset = atoi(argv[x+ 3]);
			}
            x=x+ 3;
		}
		else
		{
			printf("Usage: neutrino [-su][-font /fontdir/fontfile fontname fontsize]\n");
			exit(0);
		}
	}
}

void CNeutrinoApp::SetupFrameBuffer()
{
	frameBuffer->init();
	if (frameBuffer->setMode(720, 576, 8))
	{
		printf("Error while setting framebuffer mode\n");
		exit(-1);
	}

	//make 1..8 transparent for dummy painting
	for(int count =0;count<8;count++)
		frameBuffer->paletteSetColor(count, 0x000000, 0xffff);
	frameBuffer->paletteSet();
}

void CNeutrinoApp::SetupFonts()
{
    printf("FontFile: %s\n", (fontFile+ ".ttf").c_str() );
	printf("FontName: %s\n", fontName.c_str() );
	printf("FontSize: %d\n", fontsSizeOffset );

	g_fontRenderer->AddFont((fontFile+ ".ttf").c_str() );
	g_fontRenderer->AddFont((fontFile+ "_bold.ttf").c_str() );
	g_fontRenderer->AddFont((fontFile+ "_italic.ttf").c_str() );

	g_Fonts->menu =         g_fontRenderer->getFont(fontName.c_str(), "Bold", 20);
	g_Fonts->menu_title =   g_fontRenderer->getFont(fontName.c_str(), "Bold", 30);
	g_Fonts->menu_info =    g_fontRenderer->getFont(fontName.c_str(), "Regular", 16);

	g_Fonts->epg_title =    g_fontRenderer->getFont(fontName.c_str(), "Regular", 25+ fontsSizeOffset );

	g_Fonts->epg_info1 =	g_fontRenderer->getFont(fontName.c_str(), "Italic", 17 + 2* fontsSizeOffset );
	g_Fonts->epg_info2 =	g_fontRenderer->getFont(fontName.c_str(), "Regular", 17+ 2* fontsSizeOffset );

	g_Fonts->epg_date =		g_fontRenderer->getFont(fontName.c_str(), "Regular", 15+ 2* fontsSizeOffset );
	g_Fonts->alert =		g_fontRenderer->getFont(fontName.c_str(), "Regular", 100);

	g_Fonts->eventlist_title =		g_fontRenderer->getFont(fontName.c_str(), "Regular", 30);
	g_Fonts->eventlist_itemLarge =	g_fontRenderer->getFont(fontName.c_str(), "Bold", 20+ fontsSizeOffset );
	g_Fonts->eventlist_itemSmall =	g_fontRenderer->getFont(fontName.c_str(), "Regular", 14+ fontsSizeOffset );
	g_Fonts->eventlist_datetime =	g_fontRenderer->getFont(fontName.c_str(), "Regular", 16+ fontsSizeOffset );

	g_Fonts->gamelist_itemLarge =	g_fontRenderer->getFont(fontName.c_str(), "Bold", 20+ fontsSizeOffset );
	g_Fonts->gamelist_itemSmall =	g_fontRenderer->getFont(fontName.c_str(), "Regular", 16+ fontsSizeOffset );

	g_Fonts->channellist =			g_fontRenderer->getFont(fontName.c_str(), "Bold", 20+ fontsSizeOffset );
	g_Fonts->channellist_descr =	g_fontRenderer->getFont(fontName.c_str(), "Regular", 20+ fontsSizeOffset );
	g_Fonts->channellist_number =	g_fontRenderer->getFont(fontName.c_str(), "Bold", 14+ 2* fontsSizeOffset );
	g_Fonts->channel_num_zap =		g_fontRenderer->getFont(fontName.c_str(), "Bold", 40);

	g_Fonts->infobar_number =	g_fontRenderer->getFont(fontName.c_str(), "Bold", 50);
	g_Fonts->infobar_channame =	g_fontRenderer->getFont(fontName.c_str(), "Bold", 30);
	g_Fonts->infobar_info =		g_fontRenderer->getFont(fontName.c_str(), "Regular", 20+ fontsSizeOffset );
	g_Fonts->infobar_small =	g_fontRenderer->getFont(fontName.c_str(), "Regular", 14+ fontsSizeOffset );

}


void CNeutrinoApp::ClearFrameBuffer()
{
	memset(frameBuffer->getFrameBufferPointer(), 255, frameBuffer->getStride()*576);

	//backgroundmode
	frameBuffer->setBackgroundColor(COL_BACKGROUND);
	frameBuffer->useBackground(false);

	//background
	frameBuffer->paletteSetColor(COL_BACKGROUND, 0x000000, 0xffff);
	//Windows Colors
	frameBuffer->paletteSetColor(0x0, 0x010101, 0);
	frameBuffer->paletteSetColor(0x1, 0x800000, 0);
	frameBuffer->paletteSetColor(0x2, 0x008000, 0);
	frameBuffer->paletteSetColor(0x3, 0x808000, 0);
	frameBuffer->paletteSetColor(0x4, 0x000080, 0);
	frameBuffer->paletteSetColor(0x5, 0x800080, 0);
	frameBuffer->paletteSetColor(0x6, 0x008080, 0);
	//	frameBuffer.paletteSetColor(0x7, 0xC0C0C0, 0);
	frameBuffer->paletteSetColor(0x7, 0xA0A0A0, 0);

	//	frameBuffer.paletteSetColor(0x8, 0x808080, 0);
	frameBuffer->paletteSetColor(0x8, 0x505050, 0);

	frameBuffer->paletteSetColor(0x9, 0xFF0000, 0);
	frameBuffer->paletteSetColor(0xA, 0x00FF00, 0);
	frameBuffer->paletteSetColor(0xB, 0xFFFF00, 0);
	frameBuffer->paletteSetColor(0xC, 0x0000FF, 0);
	frameBuffer->paletteSetColor(0xD, 0xFF00FF, 0);
	frameBuffer->paletteSetColor(0xE, 0x00FFFF, 0);
	frameBuffer->paletteSetColor(0xF, 0xFFFFFF, 0);

	frameBuffer->paletteSet();
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


void CNeutrinoApp::InitScanSettings(CMenuWidget &settings)
{
	CMenuOptionChooser* ojBouquets = new CMenuOptionChooser("scants.bouquet", &((int)(scanSettings.bouquetMode)), true );
	ojBouquets->addOption( CZapitClient::BM_DELETEBOUQUETS, "scants.bouquet_erase");
	ojBouquets->addOption( CZapitClient::BM_CREATEBOUQUETS, "scants.bouquet_create");
	ojBouquets->addOption( CZapitClient::BM_DONTTOUCHBOUQUETS, "scants.bouquet_leave");

	//kabel-lnb-settings
	if (g_info.fe==1)
	{
		settings.addItem( new CMenuSeparator() );
		settings.addItem( new CMenuForwarder("menu.back") );
		settings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

		CZapitClient::SatelliteList satList;
		g_Zapit->getScanSatelliteList(satList);
		CMenuOptionStringChooser* ojSat = new CMenuOptionStringChooser("satsetup.satellite", (char*)&scanSettings.satNameNoDiseqc, scanSettings.diseqcMode == NO_DISEQC/*, new CSatelliteNotifier*/, NULL, false);
		for ( uint i=0; i< satList.size(); i++)
		{
			ojSat->addOption(satList[i].satName);
		}

		CMenuOptionChooser* ojDiseqcRepeats = new CMenuOptionChooser("satsetup.diseqcrepeat", &((int)(scanSettings.diseqcRepeat)), scanSettings.diseqcMode != NO_DISEQC/*, new CSatelliteNotifier*/, NULL, false);
		for ( uint i=0; i<=2; i++)
		{
			char ii[2];
			sprintf( ii, "%d", i);
			ojDiseqcRepeats->addOption(i, ii);
		}

		CMenuWidget* extSatSettings = new CMenuWidget("satsetup.extended", "settings.raw");
		extSatSettings->addItem( new CMenuSeparator() );
		extSatSettings->addItem( new CMenuForwarder("menu.back") );
		extSatSettings->addItem( new CMenuSeparator(CMenuSeparator::LINE) );
		CMenuForwarder* ojExtSatSettings = new CMenuForwarder("satsetup.extended", scanSettings.diseqcMode != NO_DISEQC, "", extSatSettings);
		for ( uint i=0; i< satList.size(); i++)
		{
			CMenuOptionChooser* oj = new CMenuOptionChooser( satList[i].satName, scanSettings.diseqscOfSat( satList[i].satName), true/*, new CSatelliteNotifier*/);
			oj->addOption( -1, "options.off");
			for ( int j=0; j<=3; j++)
			{
				char jj[2];
				sprintf( jj, "%d", j);
				oj->addOption( j, jj);
			}
			extSatSettings->addItem( oj);
		}

		CMenuOptionChooser* ojDiseqc = new CMenuOptionChooser("satsetup.disqeqc", &((int)(scanSettings.diseqcMode)), true, new CSatDiseqcNotifier( ojSat, ojExtSatSettings, ojDiseqcRepeats));
		ojDiseqc->addOption( NO_DISEQC,   "satsetup.nodiseqc");
		ojDiseqc->addOption( MINI_DISEQC, "satsetup.minidiseqc");
		ojDiseqc->addOption( DISEQC_1_0,  "satsetup.diseqc10");
		ojDiseqc->addOption( DISEQC_1_1,  "satsetup.diseqc11");
		ojDiseqc->addOption( SMATV_REMOTE_TUNING,  "satsetup.smatvremote");

		settings.addItem( ojDiseqc );
		settings.addItem( ojBouquets);
		settings.addItem( ojSat);
		settings.addItem( ojDiseqcRepeats );
		settings.addItem( ojExtSatSettings);

	}
	else
	{//kabel
		settings.addItem( new CMenuSeparator() );
		settings.addItem( new CMenuForwarder("menu.back") );
		settings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

		static int dummy = 0;
		FILE* fd = fopen("/var/etc/.specinv", "r");
		if(fd)
		{
			dummy=1;
			fclose(fd);
		}
		CMenuOptionChooser* ojInv = new CMenuOptionChooser("cablesetup.spectralInversion", &dummy, true, new CCableSpectalInversionNotifier );
		ojInv->addOption(0, "options.off");
		ojInv->addOption(1, "options.on");

		CZapitClient::SatelliteList providerList;
		g_Zapit->getScanSatelliteList(providerList);
/*		static int cableProvider = 0;
		for ( uint i=0; i<providerList.size(); i++)
		{
			if( !strcmp( providerList[i].satName, scanSettings.satellites[0].name))
			{
				cableProvider = i;
				break;
			}
		}
*/		CMenuOptionStringChooser* oj = new CMenuOptionStringChooser("cablesetup.provider", (char*)&scanSettings.satNameNoDiseqc, true/*, new CCableProviderNotifier*/);

		for ( uint i=0; i< providerList.size(); i++)
		{
			oj->addOption( providerList[i].satName);
		}
		settings.addItem( ojBouquets);
		settings.addItem( ojInv );
		settings.addItem( oj);
	}

	settings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	settings.addItem( new CMenuForwarder("scants.startnow", true, "", new CScanTs() ) );

}


void CNeutrinoApp::InitServiceSettings(CMenuWidget &service, CMenuWidget &scanSettings)
{
	service.addItem( new CMenuSeparator() );
	service.addItem( new CMenuForwarder("menu.back") );
	service.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	service.addItem( new CMenuForwarder("bouqueteditor.name", true, "", new CBEBouquetWidget()));
	service.addItem( new CMenuForwarder("servicemenu.scants", true, "", &scanSettings ) );
	service.addItem( new CMenuForwarder("servicemenu.ucodecheck", true, "", UCodeChecker ) );

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
		updateSettings->addItem( new CMenuForwarder("flashupdate.checkupdate", true, "", new CFlashUpdate() ));

		service.addItem( new CMenuForwarder("servicemenu.update", true, "", updateSettings ) );
	}

}

void CNeutrinoApp::InitMiscSettings(CMenuWidget &miscSettings)
{
	miscSettings.addItem( new CMenuSeparator() );
	miscSettings.addItem( new CMenuForwarder("menu.back") );
	miscSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "miscsettings.general" ) );

/*	CMenuOptionChooser *oj = new CMenuOptionChooser("miscsettings.boxtype", &g_settings.box_Type, false, NULL, false );
	oj->addOption(1, "Nokia");
	oj->addOption(2, "Sagem");
	oj->addOption(3, "Philips");
	miscSettings.addItem( oj );
*/
	CMenuOptionChooser *oj = new CMenuOptionChooser("miscsettings.shutdown_real", &g_settings.shutdown_real, true );
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

	keySetupNotifier = new CKeySetupNotifier;
	CStringInput*	keySettings_repeat_genericblocker= new CStringInput("keybindingmenu.repeatblockgeneric", g_settings.repeat_blocker, 3, "repeatblocker.hint_1", "repeatblocker.hint_2", "0123456789 ", keySetupNotifier);
	CStringInput*	keySettings_repeatBlocker= new CStringInput("keybindingmenu.repeatblock", g_settings.repeat_genericblocker, 3, "repeatblocker.hint_1", "repeatblocker.hint_2", "0123456789 ", keySetupNotifier);
	keySetupNotifier->changeNotify("initial", NULL);

	miscSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "keybindingmenu.RC") );
	miscSettings.addItem( new CMenuForwarder("keybindingmenu.repeatblock", true, "", keySettings_repeatBlocker ));
	miscSettings.addItem( new CMenuForwarder("keybindingmenu.repeatblockgeneric", true, "", keySettings_repeat_genericblocker ));

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

	CMenuOptionChooser* oj = new CMenuOptionChooser("audiomenu.analogout", &g_settings.audio_AnalogMode, true, audioSetupNotifier);
		oj->addOption(0, "audiomenu.stereo");
		oj->addOption(1, "audiomenu.monoleft");
		oj->addOption(2, "audiomenu.monoright");

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
	videoSettings.addItem( new CMenuForwarder("videomenu.screensetup", true, "", new CScreenSetup() ) );
}

void CNeutrinoApp::InitParentalLockSettings(CMenuWidget &parentallockSettings)
{
	parentallockSettings.addItem( new CMenuSeparator() );
	parentallockSettings.addItem( new CMenuForwarder("menu.back") );
	parentallockSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuOptionChooser* oj = new CMenuOptionChooser("parentallock.prompt", &g_settings.parentallock_prompt, true);
	oj->addOption(PARENTALLOCK_PROMPT_NEVER         , "parentallock.never");
	oj->addOption(PARENTALLOCK_PROMPT_ONSTART       , "parentallock.onstart");
	//oj->addOption(PARENTALLOCK_PROMPT_CHANGETOLOCKED, "parentallock.changetolocked");
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

	CIPInput*	networkSettings_NetworkIP= new CIPInput("networkmenu.ipaddress", g_settings.network_ip, "ipsetup.hint_1", "ipsetup.hint_2", MyIPChanger);
	CIPInput*	networkSettings_NetMask= new CIPInput("networkmenu.netmask", g_settings.network_netmask, "ipsetup.hint_1", "ipsetup.hint_2");
	CIPInput*	networkSettings_Broadcast= new CIPInput("networkmenu.broadcast", g_settings.network_broadcast, "ipsetup.hint_1", "ipsetup.hint_2");
	CIPInput*	networkSettings_Gateway= new CIPInput("networkmenu.gateway", g_settings.network_defaultgateway, "ipsetup.hint_1", "ipsetup.hint_2");
	CIPInput*	networkSettings_NameServer= new CIPInput("networkmenu.nameserver", g_settings.network_nameserver, "ipsetup.hint_1", "ipsetup.hint_2");

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

	CMenuWidget colorSettings_Themes("colorthememenu.head", "settings.raw");
	InitColorThemesSettings(colorSettings_Themes);

	colorSettings.addItem( new CMenuForwarder("colormenu.themeselect", true, "", &colorSettings_Themes) );
	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuWidget colorSettings_menuColors("colormenusetup.head", "settings.raw", 400, 400);
	InitColorSettingsMenuColors(colorSettings_menuColors);
	colorSettings.addItem( new CMenuForwarder("colormenu.menucolors", true, "", &colorSettings_menuColors) );

	CMenuWidget colorSettings_statusbarColors("colormenu.statusbar", "settings.raw");
	InitColorSettingsStatusBarColors(colorSettings_statusbarColors);
	colorSettings.addItem( new CMenuForwarder("colorstatusbar.head", true, "", &colorSettings_statusbarColors) );

	colorSettings.addItem( new CMenuSeparator(CMenuSeparator::LINE) );

	CMenuOptionChooser* oj = new CMenuOptionChooser("colormenu.fade", &g_settings.widget_fade, true );
	oj->addOption(0, "options.off");
	oj->addOption(1, "options.on");
	colorSettings.addItem( oj );

}

void CNeutrinoApp::InitColorThemesSettings(CMenuWidget &colorSettings_Themes)
{
	colorSettings_Themes.addItem( new CMenuSeparator() );
	colorSettings_Themes.addItem( new CMenuForwarder("menu.back") );
	colorSettings_Themes.addItem( new CMenuSeparator(CMenuSeparator::LINE) );
	colorSettings_Themes.addItem( new CMenuForwarder("colorthememenu.neutrino_theme", true, "", this, "theme_neutrino") );
	colorSettings_Themes.addItem( new CMenuForwarder("colorthememenu.classic_theme", true, "", this, "theme_classic") );
}

void CNeutrinoApp::InitColorSettingsMenuColors(CMenuWidget &colorSettings_menuColors)
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
}

void CNeutrinoApp::InitColorSettingsStatusBarColors(CMenuWidget &colorSettings_statusbarColors)
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
}

void CNeutrinoApp::InitKeySettings(CMenuWidget &keySettings)
{
	keySettings.addItem( new CMenuSeparator() );

	keySettings.addItem( new CMenuForwarder("menu.back") );

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

void CNeutrinoApp::SelectNVOD()
{
	if ( g_RemoteControl->subChannels.size()> 0 )
	{
		// NVOD/SubService- Kanal!
		CMenuWidget NVODSelector( g_RemoteControl->are_subchannels?"nvodselector.subservice":"nvodselector.head", "video.raw", 400 );

		NVODSelector.addItem( new CMenuSeparator() );

		int count = 0;
		char nvod_id[5];

		for ( CSubServiceListSorted::iterator e=g_RemoteControl->subChannels.begin(); e!=g_RemoteControl->subChannels.end(); ++e)
		{
			sprintf(nvod_id, "%d", count);

			if ( !g_RemoteControl->are_subchannels )
			{
				char nvod_time_a[50], nvod_time_e[50], nvod_time_x[50];
				char nvod_s[100];
				struct  tm *tmZeit;

				tmZeit= localtime( &e->startzeit );
				sprintf(nvod_time_a, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

				time_t endtime = e->startzeit+ e->dauer;
				tmZeit= localtime(&endtime);
				sprintf(nvod_time_e, "%02d:%02d", tmZeit->tm_hour, tmZeit->tm_min);

				time_t jetzt=time(NULL);
				if (e->startzeit > jetzt)
				{
					int mins=(e->startzeit- jetzt)/ 60;
					sprintf(nvod_time_x, g_Locale->getText("nvod.starting").c_str(), mins);
				}
				else
					if ( (e->startzeit<= jetzt) && (jetzt < endtime) )
					{
						int proz=(jetzt- e->startzeit)*100/ e->dauer;
						sprintf(nvod_time_x, g_Locale->getText("nvod.proz").c_str(), proz);
					}
					else
						nvod_time_x[0]= 0;

				sprintf(nvod_s, "%s - %s %s", nvod_time_a, nvod_time_e, nvod_time_x);
				NVODSelector.addItem( new CMenuForwarder(nvod_s, true, "", NVODChanger, nvod_id, false), (count == g_RemoteControl->selected_subchannel) );
			}
			else
			{
				NVODSelector.addItem( new CMenuForwarder(e->subservice_name, true, "", NVODChanger, nvod_id, false, (count<9)? (count+1) : CRCInput::RC_nokey ), (count == g_RemoteControl->selected_subchannel) );
			}

			count++;
		}
		NVODSelector.exec(NULL, "");
	}
}

void CNeutrinoApp::SelectAPID()
{
	if ( g_RemoteControl->current_PIDs.APIDs.size()> 1 )
	{
		// wir haben APIDs für diesen Kanal!

		CMenuWidget APIDSelector("apidselector.head", "audio.raw", 300);
		APIDSelector.addItem( new CMenuSeparator() );

		for( int count=0; count<g_RemoteControl->current_PIDs.APIDs.size(); count++ )
		{
			char apid[5];
			sprintf(apid, "%d", count);
			APIDSelector.addItem( new CMenuForwarder(g_RemoteControl->current_PIDs.APIDs[count].desc, true,
								  "", APIDChanger, apid, false, (count<9)? (count+1) : CRCInput::RC_nokey ), (count == g_RemoteControl->current_PIDs.PIDs.selected_apid) );
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

    g_PluginList->loadPlugins();

	for(unsigned int count=0;count<g_PluginList->getNumberOfPlugins();count++)
	{
    	if ( g_PluginList->getType(count)== 2 )
    	{
    		// zB vtxt-plugins

			sprintf(id, "%d", count);

			bool enable_it = ( ( !g_PluginList->getVTXT(count) )  || (g_RemoteControl->current_PIDs.PIDs.vtxtpid!=0) );
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

	// -- Add Channel to favorites
	StreamFeatureSelector.addItem( new CMenuForwarder("favorites.menueadd", true, "",
		new CFavorites, id, true, CRCInput::RC_yellow, "gelb.raw"), false );

	// -- Stream Info
	StreamFeatureSelector.addItem( new CMenuForwarder("streamfeatures.info", true, "",
		StreamFeaturesChanger, id, true, CRCInput::RC_help, "help_small.raw"), false );


	StreamFeatureSelector.exec(NULL, "");
}


void CNeutrinoApp::InitZapper()
{

	g_InfoViewer->start();
	g_EpgData->start();


	doChecks();
	firstChannel();
	if (firstchannel.mode == 't')
	{
		tvMode();
	}
	else
	{
		radioMode();
	}
}

int CNeutrinoApp::run(int argc, char **argv)
{
	g_info.box_Type = atoi(getenv("mID"));
	g_info.gtx_ID = -1;
	sscanf(getenv("gtxID"), "%x", &g_info.gtx_ID);
	g_info.enx_ID = -1;
	sscanf(getenv("enxID"), "%x", &g_info.enx_ID);
	g_info.fe = 0;
	sscanf(getenv("fe"), "%x", &g_info.fe);
	//printf("box_Type: %d, gtxID: %d, enxID: %d, fe: %d\n", g_info.box_Type, g_info.gtx_ID, g_info.enx_ID, g_info.fe);

	if(!loadSetup())
	{
		//setup default if configfile not exists
		setupDefaults();
		printf("using defaults...\n\n");
	}

	//timing  (Einheit= 1 sec )
	g_settings.timing_menu = 60;
	g_settings.timing_chanlist = 60;
	g_settings.timing_epg = 2* 60;
	g_settings.timing_infobar = 6;

	CmdParser(argc, argv);

	g_Fonts = new FontsDef;
	SetupFonts();

	ClearFrameBuffer();

	g_Controld = new CControldClient;
	g_Locale = new CLocaleManager;
	g_RCInput = new CRCInput;
	g_lcdd = new CLcddClient;
	g_Zapit = new CZapitClient;
	g_Sectionsd = new CSectionsdClient;
	g_Timerd = new CTimerdClient;

	g_RemoteControl = new CRemoteControl;
	g_EpgData = new CEpgData;
	g_InfoViewer = new CInfoViewer;
	g_EventList = new EventList;

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
	CMenuWidget keySettings("keybindingmenu.head", "keybinding.raw", 400, 460);
	CMenuWidget miscSettings("miscsettings.head", "settings.raw");
	CMenuWidget scanSettings("servicemenu.scants", "settings.raw");
	CMenuWidget service("servicemenu.head", "settings.raw");

	InitMainMenu(mainMenu, mainSettings, audioSettings, parentallockSettings, networkSettings,
	             colorSettings, keySettings, videoSettings, languageSettings, miscSettings, service);

	//service
	InitServiceSettings(service, scanSettings);

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

	// ScanSettings
	InitScanSettings(scanSettings);

	g_Controld->registerEvent(CControldClient::EVT_MUTECHANGED, 222, NEUTRINO_UDS_NAME);
    g_Controld->registerEvent(CControldClient::EVT_VOLUMECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_MODECHANGED, 222, NEUTRINO_UDS_NAME);
	g_Controld->registerEvent(CControldClient::EVT_VCRCHANGED, 222, NEUTRINO_UDS_NAME);

	g_Sectionsd->registerEvent(CSectionsdClient::EVT_TIMESET, 222, NEUTRINO_UDS_NAME);
	g_Sectionsd->registerEvent(CSectionsdClient::EVT_GOT_CN_EPG, 222, NEUTRINO_UDS_NAME);

	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_FAILED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_COMPLETE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_COMPLETE_IS_NVOD, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_SUB_COMPLETE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_ZAP_SUB_FAILED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_COMPLETE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_NUM_TRANSPONDERS, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_SATELLITE, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_NUM_CHANNELS, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_SCAN_PROVIDER, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_RECORDMODE_ACTIVATED, 222, NEUTRINO_UDS_NAME);
	g_Zapit->registerEvent(CZapitClient::EVT_RECORDMODE_DEACTIVATED, 222, NEUTRINO_UDS_NAME);

	g_Timerd->registerEvent(CTimerdClient::EVT_SHUTDOWN, 222, NEUTRINO_UDS_NAME);
	g_Timerd->registerEvent(CTimerdClient::EVT_NEXTPROGRAM, 222, NEUTRINO_UDS_NAME);

	//init programm
	InitZapper();

	//network Setup
	InitNetworkSettings(networkSettings);

	//color Setup
	InitColorSettings(colorSettings);

	//keySettings
	InitKeySettings(keySettings);

	current_volume= g_Controld->getVolume();
	AudioMute( g_Controld->getMute(), true );

	RealRun(mainMenu);

	ExitRun();
	return 0;
}

void CNeutrinoApp::RealRun(CMenuWidget &mainMenu)
{
	while( true )
	{
		uint msg; uint data;
		g_RCInput->getMsg( &msg, &data, 100 ); // 10 secs..

		if ( msg == NeutrinoMessages::STANDBY_ON )
		{
			if ( mode != mode_standby )
			{
				// noch nicht im Standby-Mode...
				standbyMode( true );
			}
			g_RCInput->clearMsg();
		}

		if ( msg == NeutrinoMessages::STANDBY_OFF )
		{
			if ( mode == mode_standby )
			{
				// WAKEUP
				standbyMode( false );
			}
			g_RCInput->clearMsg();
		}

		else if ( msg == NeutrinoMessages::SHUTDOWN )
		{
			// AUSSCHALTEN...
			ExitRun();
		}

		else if ( msg == NeutrinoMessages::VCR_ON )
		{
			if  ( mode != mode_scart )
			{
				// noch nicht im Scart-Mode...
				scartMode( true );
			}
		}

		else if ( msg == NeutrinoMessages::VCR_OFF )
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
				if ( msg == NeutrinoMessages::SHOW_EPG )
				{
					// show EPG

					g_EpgData->show( channelList->getActiveChannelOnid_sid() );

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
						  ( msg == NeutrinoMessages::SHOW_INFOBAR ) )
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

void CNeutrinoApp::showProfiling( string text )
{
	struct timeval tv;

	gettimeofday( &tv, NULL );
	long long now = (long long) tv.tv_usec + (long long)((long long) tv.tv_sec * (long long) 1000000);


	//printf("--> '%s' %f\n", text.c_str(), (now- last_profile_call)/ 1000.);
	last_profile_call = now;
}

int CNeutrinoApp::handleMsg(uint msg, uint data)
{
	int res = 0;

	res = res | g_RemoteControl->handleMsg(msg, data);
	res = res | g_InfoViewer->handleMsg(msg, data);
	res = res | channelList->handleMsg(msg, data);

	if ( res != messages_return::unhandled )
	{
		if ( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
			delete (unsigned char*) data;
		return ( res & ( 0xFFFFFFFF - messages_return::unhandled ) );
	}

    if ( msg == NeutrinoMessages::EVT_VCRCHANGED )
	{
		if ( g_settings.vcr_AutoSwitch == 1 )
		{
			if ( data != VCR_STATUS_OFF )
				g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
			else
				g_RCInput->postMsg( NeutrinoMessages::VCR_OFF, 0 );
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
        	g_RCInput->postMsg( NeutrinoMessages::STANDBY_OFF, 0 );
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

			g_RCInput->postMsg( ( diff >= 10 ) ? NeutrinoMessages::SHUTDOWN : NeutrinoMessages::STANDBY_ON, 0 );
        }
        else
        {
        	g_RCInput->postMsg( NeutrinoMessages::SHUTDOWN, 0 );
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
        	g_RCInput->postMsg( NeutrinoMessages::SHUTDOWN, 0 );
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
	else if ( msg == NeutrinoMessages::EVT_VOLCHANGED )
	{
		current_volume = data;
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_MUTECHANGED )
	{
		AudioMute( (bool)data, true );
		return messages_return::handled;
	}
	else if ( msg == NeutrinoMessages::EVT_RECORDMODE )
	{
		printf("neutino - recordmode %s\n", ( data ) ? "on":"off" );

		if ( ( !g_InfoViewer->is_visible ) && data )
			g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

		channelsInit();
	}
	else if ( ( msg == NeutrinoMessages::EVT_BOUQUETSCHANGED ) ||
			  ( msg == NeutrinoMessages::EVT_SERVICESCHANGED ) )
	{
		unsigned int old_id = channelList->getActiveChannelOnid_sid();

		channelsInit();
		tvMode( true );

		if ( ! channelList->zapToOnidSid ( old_id ) )
			channelList->zapTo( 0 );

		return messages_return::handled;
	}

	if ( ( msg>= CRCInput::RC_WithData ) && ( msg< CRCInput::RC_WithData+ 0x10000000 ) )
		delete (unsigned char*) data;

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

	for(int x=0;x<256;x++)
		frameBuffer->paletteSetColor(x, 0x000000, 0xffff);
	frameBuffer->paletteSet();

	frameBuffer->loadPicture2Mem("shutdown.raw", frameBuffer->getFrameBufferPointer() );
	frameBuffer->loadPal("shutdown.pal");

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
			frameBuffer->paintBoxRel(x, y, dx, dy, COL_INFOBAR);
			frameBuffer->paintIcon("mute.raw", x+5, y+5);
		}
		else
			frameBuffer->paintBackgroundBoxRel(x, y, dx, dy);
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
			frameBuffer->SaveScreen(x, y, dx, dy, pixbuf);
		frameBuffer->paintIcon("volume.raw",x,y, COL_INFOBAR);
	}

	uint msg = key;
	uint data;

	unsigned long long timeoutEnd;

	do
	{
		if ( msg <= CRCInput::RC_MaxRC )
			timeoutEnd = g_RCInput->calcTimeoutEnd( g_settings.timing_infobar/ 2 );

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
				if ( handleMsg( msg, data ) & messages_return::unhandled )
				{
					g_RCInput->postMsg( msg, data );

					msg= CRCInput::RC_timeout;
				}
			}
		}

		if (bDoPaint)
		{
			int vol = current_volume<<1;
			frameBuffer->paintBoxRel(x+40, y+12, 200, 15, COL_INFOBAR+1);
			frameBuffer->paintBoxRel(x+40, y+12, vol, 15, COL_INFOBAR+3);
        }

		if ( msg != CRCInput::RC_timeout )
		{
			g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
		}

	}
	while ( msg != CRCInput::RC_timeout );

    if ( (bDoPaint) && (pixbuf!= NULL) )
		frameBuffer->RestoreScreen(x, y, dx, dy, pixbuf);
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
	#ifdef USEACTIONLOG
		g_ActionLog->println("mode: tv");
	#endif

	//printf( "tv-mode\n" );

	memset(frameBuffer->getFrameBufferPointer(), 255, frameBuffer->getStride()*576);
	frameBuffer->useBackground(false);

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
		memset(frameBuffer->getFrameBufferPointer(), 255, frameBuffer->getStride()*576);
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

		memset(frameBuffer->getFrameBufferPointer(), 255, frameBuffer->getStride()*576);

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
	#ifdef USEACTIONLOG
		g_ActionLog->println("mode: radio");
	#endif

	frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	frameBuffer->loadBackground("radiomode.raw");
	frameBuffer->useBackground(true);
	frameBuffer->paintBackground();

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
		g_RCInput->postMsg( NeutrinoMessages::VCR_ON, 0 );
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


/**************************************************************************************
*                                                                                     *
*          Main programm - no function here                                           *
*                                                                                     *
**************************************************************************************/
int main(int argc, char **argv)
{
	printf("NeutrinoNG $Id: neutrino.cpp,v 1.253 2002/04/27 07:44:07 field Exp $\n\n");
	tzset();
	initGlobals();

	return CNeutrinoApp::getInstance()->run(argc, argv);
}

