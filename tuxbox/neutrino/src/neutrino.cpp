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

#include "neutrino.h"



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
										int(settings->infobar_blue*0.4))
					, settings->infobar_alpha);

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
	settings->audio_DolbyDigital  = 0;

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
	
	delete channelList;
	channelList = new(CChannelList);
	
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
  		perror("Couldn't connect to server!");
		exit(-1);
	}

	write(sock_fd, &sendmessage, sizeof(sendmessage));
	return_buf = (char*) malloc(4);
	
	if (recv(sock_fd, return_buf, 3,0) <= 0 ) {
		perror("Nothing could be received\n");
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
int CNeutrinoApp::run(int argc, char **argv)
{
	printf("neutrino2\n");

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
	
	fontRenderer = new fontRenderClass( &frameBuffer);
	
	if (frameBuffer.setMode(720, 576, 8))
	{
		printf("Error while setting framebuffer mode\n");
		exit(-1);
	}

	//make 1..8 transpartent fory dummy painting
	for(int count =0;count<8;count++)
		frameBuffer.paletteSetColor(count, 0x000000, 0xffff); 
	frameBuffer.paletteSet();

	//paint...
	fonts.menu=fontRenderer->getFont("Arial", "Bold", 20);
	fonts.menu->RenderString( 10,100, 500, "DEMO!", 0 );
        fonts.menu_number=fontRenderer->getFont("Arial", "Regular", 15);
	fonts.menu_title=fontRenderer->getFont("Arial Black", "Regular", 24);
	fonts.menu_title->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.epg_title=fontRenderer->getFont("Arial", "Regular", 30);
	fonts.epg_title->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.epg_info1=fontRenderer->getFont("Arial", "Italic", 20);
	fonts.epg_info1->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.epg_info2=fontRenderer->getFont("Arial", "Regular", 17);
	fonts.epg_info2->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.epg_date=fontRenderer->getFont("Arial", "Regular", 15);
	fonts.epg_date->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.alert=fontRenderer->getFont("Arial", "Regular", 100);
	fonts.channellist=fontRenderer->getFont("Arial", "Regular", 20);
	fonts.channellist->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.infobar_number=fontRenderer->getFont("Arial", "Regular", 50);
	fonts.infobar_number->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.infobar_channame=fontRenderer->getFont("Arial", "Regular", 30);
	fonts.infobar_channame->RenderString( 10,100, 500, "DEMO!", 0 );
	fonts.infobar_info=fontRenderer->getFont("Arial", "Regular", 20);
	fonts.infobar_info->RenderString( 10,100, 500, "DEMO!", 0 );


	//clear frame...
	memset(frameBuffer.lfb, 255, frameBuffer.Stride()*576);


	//background
	frameBuffer.paletteSetColor(255, 0x000000, 0xffff); 
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


	channelList = new CChannelList(1,"All Services");

	if(!loadSetup(&settings))
	{
		//setup default if configfile not exists
		setupDefaults( &settings);
		printf("useing defaults...\n\n");
	}


	colorSetupNotifier = new CColorSetupNotifier(&frameBuffer, &settings);
	CAudioSetupNotifier		audioSetupNotifier;
	CVideoSetupNotifier		videoSetupNotifier;
	CNetworkSetupNotifier	networkSetupNotifier(&settings);;

	colorSetupNotifier->changeNotify("initial");
	networkSetupNotifier.changeNotify("initial");

	
	//Main settings
	CMenuWidget			mainSettings;
	CMenuWidget			videoSettings;
	CMenuWidget			audioSettings;
	CMenuWidget			networkSettings;
	CMenuWidget			colorSettings;
	CMenuWidget			keySettings;
	//CMenuWidget			screenSettings;

	mainSettings.setName("Neutrino setup");
	mainSettings.setIcon("settings.raw");
	mainSettings.addItem( new CMenuSeparator(4) );
	mainSettings.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "Run mode") );
	mainSettings.addItem( new CMenuForwarder("Shutdown", true, "", this, "shutdown") );
	mainSettings.addItem( new CMenuForwarder("TV-Mode", true, "", this, "tv"), true );
	mainSettings.addItem( new CMenuForwarder("Radio-Mode", true, "", this, "radio") );
	mainSettings.addItem( new CMenuForwarder("MP3-Player", false, "", this, "mp3") );
	mainSettings.addItem( new CMenuForwarder("Stream playback", false, "", this, "playback") );
	mainSettings.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "Settings") );
	mainSettings.addItem( new CMenuForwarder("Video", true, "", &videoSettings) );
		CScreenSetup	screenSettings("Screen setup", &settings);
	mainSettings.addItem( new CMenuForwarder("Screen Setup", true, "", &screenSettings) );
	mainSettings.addItem( new CMenuForwarder("Audio", true, "", &audioSettings) );
	mainSettings.addItem( new CMenuForwarder("Network", true, "", &networkSettings) );
	mainSettings.addItem( new CMenuForwarder("Colors", true,"", &colorSettings) );
	mainSettings.addItem( new CMenuForwarder("Key binding", true,"", &keySettings) );

	//audio Setup
	audioSettings.setName("Audio setup");
	audioSettings.setIcon("audio.raw");
	audioSettings.addItem( new CMenuSeparator(4) );
	audioSettings.addItem( new CMenuForwarder("back") );
	audioSettings.addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
		CMenuOptionChooser* oj = new CMenuOptionChooser("Stereo", &settings.audio_Stereo, true, &audioSetupNotifier);
		oj->addOption(0, "off");
		oj->addOption(1, "on");
	audioSettings.addItem( oj );
		oj = new CMenuOptionChooser("Dolby Digital", &settings.audio_DolbyDigital, true, &audioSetupNotifier);
		oj->addOption(0, "off");
		oj->addOption(1, "on");
	audioSettings.addItem( oj );	

	//video Setup
	videoSettings.setName("Video setup");
	videoSettings.setIcon("video.raw");
	videoSettings.addItem( new CMenuSeparator(4) );
	videoSettings.addItem( new CMenuForwarder("back") );
	videoSettings.addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
		oj = new CMenuOptionChooser("Output signal", &settings.video_Signal, true, &videoSetupNotifier);
		oj->addOption(0, "RGB");
		oj->addOption(1, "S-Video");
		oj->addOption(2, "FBAS");
	videoSettings.addItem( oj );
		oj = new CMenuOptionChooser("Format", &settings.video_Format, true, &videoSetupNotifier);
		oj->addOption(0, "4:3");
		oj->addOption(1, "16:9");
	videoSettings.addItem( oj );	

	//Screen  Setup
	/*
	screenSettings.setName("Screen setup");
	screenSettings.setIcon("video.raw");
	screenSettings.addItem( new CMenuSeparator(4) );
	screenSettings.addItem( new CMenuForwarder("back") );
	screenSettings.addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
		CScreenSetup	screenSettings_upleft("Screen setup upper left corner", &settings);
	screenSettings.addItem( new CMenuForwarder("lower right corner", true, "",&screenSettings_lowright ));
*/

	//network Setup
	networkSettings.setName("Network setup");
	networkSettings.setIcon("settings.raw");
	networkSettings.addItem( new CMenuSeparator(4) );
	networkSettings.addItem( new CMenuForwarder("back") );
	networkSettings.addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
		oj = new CMenuOptionChooser("setup network on startup", &settings.networkSetOnStartup, true, &networkSetupNotifier);
		oj->addOption(0, "off");
		oj->addOption(1, "on");
	networkSettings.addItem( oj );	
	networkSettings.addItem( new CMenuSeparator(6, CMenuSeparator::LINE) );
		CStringInput	networkSettings_NetworkIP("IP Adress", settings.network_ip, 3*4+3);
		CStringInput	networkSettings_NetMask("Network mask", settings.network_netmask, 3*4+3);
		CStringInput	networkSettings_Broadcast("Broadcast", settings.network_broadcast, 3*4+3);
		CStringInput	networkSettings_Gateway("Default gateway", settings.network_defaultgateway, 3*4+3);
		CStringInput	networkSettings_NameServer("Nameserver", settings.network_nameserver, 3*4+3);
	networkSettings.addItem( new CMenuForwarder("IP Adress", true, settings.network_ip, &networkSettings_NetworkIP ));
	networkSettings.addItem( new CMenuForwarder("Netmask", true, settings.network_netmask, &networkSettings_NetMask ));
	networkSettings.addItem( new CMenuForwarder("Broadcast", true, settings.network_broadcast, &networkSettings_Broadcast ));
	networkSettings.addItem( new CMenuSeparator(6, CMenuSeparator::LINE) );
	networkSettings.addItem( new CMenuForwarder("Default gateway", true, settings.network_defaultgateway, &networkSettings_Gateway ));
	networkSettings.addItem( new CMenuForwarder("Nameserver", true, settings.network_nameserver, &networkSettings_NameServer ));

	//color Setup
	colorSettings.setName("Color setup");
	colorSettings.setIcon("settings.raw");
	colorSettings.addItem( new CMenuSeparator(4) );
	colorSettings.addItem( new CMenuForwarder("back") );
	colorSettings.addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
		CMenuWidget		audioSettings_Themes;
		audioSettings_Themes.setName("Select a theme");
		audioSettings_Themes.setIcon("settings.raw");
		audioSettings_Themes.addItem( new CMenuSeparator(4) );
		audioSettings_Themes.addItem( new CMenuForwarder("back") );
		audioSettings_Themes.addItem( new CMenuSeparator(11, CMenuSeparator::LINE) );
		audioSettings_Themes.addItem( new CMenuForwarder("Neutrino default theme", true, "", this, "theme_neutrino") );
		audioSettings_Themes.addItem( new CMenuForwarder("Classic theme", true, "", this, "theme_classic") );
	colorSettings.addItem( new CMenuForwarder("select color theme", true, "", &audioSettings_Themes) );
	colorSettings.addItem( new CMenuSeparator(6, CMenuSeparator::LINE) );
		CMenuWidget		colorSettings_menuColors;
		colorSettings_menuColors.setName("menu colors");
		colorSettings_menuColors.setIcon("settings.raw");
		colorSettings_menuColors.addItem( new CMenuSeparator(4) );
		colorSettings_menuColors.addItem( new CMenuForwarder("back") );
			CColorChooser chHeadcolor("background color", &settings.menu_Head_red, &settings.menu_Head_green, &settings.menu_Head_blue, 
					&settings.menu_Head_alpha, colorSetupNotifier);
			CColorChooser chHeadTextcolor("text color", &settings.menu_Head_Text_red, &settings.menu_Head_Text_green, &settings.menu_Head_Text_blue, 
					NULL, colorSetupNotifier);
			CColorChooser chContentcolor("background color", &settings.menu_Content_red, &settings.menu_Content_green, &settings.menu_Content_blue, 
					&settings.menu_Content_alpha, colorSetupNotifier);
			CColorChooser chContentTextcolor("text color", &settings.menu_Content_Text_red, &settings.menu_Content_Text_green, &settings.menu_Content_Text_blue, 
					NULL, colorSetupNotifier);
			CColorChooser chContentSelectedcolor("background color", &settings.menu_Content_Selected_red, &settings.menu_Content_Selected_green, &settings.menu_Content_Selected_blue, 
					&settings.menu_Content_Selected_alpha, colorSetupNotifier);
			CColorChooser chContentSelectedTextcolor("text color", &settings.menu_Content_Selected_Text_red, &settings.menu_Content_Selected_Text_green, &settings.menu_Content_Selected_Text_blue, 
					NULL, colorSetupNotifier);
			CColorChooser chContentInactivecolor("background color", &settings.menu_Content_inactive_red, &settings.menu_Content_inactive_green, &settings.menu_Content_inactive_blue, 
					&settings.menu_Content_inactive_alpha, colorSetupNotifier);
			CColorChooser chContentInactiveTextcolor("text color", &settings.menu_Content_inactive_Text_red, &settings.menu_Content_inactive_Text_green, &settings.menu_Content_inactive_Text_blue, 
					NULL, colorSetupNotifier);
		colorSettings_menuColors.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu heads") );
		colorSettings_menuColors.addItem( new CMenuForwarder("Background", true, "",&chHeadcolor ));
		colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", true, "",&chHeadTextcolor ));
		colorSettings_menuColors.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu body") );
		colorSettings_menuColors.addItem( new CMenuForwarder("Background", true, "", &chContentcolor ));
		colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", true, "", &chContentTextcolor ));
		colorSettings_menuColors.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu body - inactive") );
		colorSettings_menuColors.addItem( new CMenuForwarder("Background", true, "", &chContentInactivecolor ));
		colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", true, "", &chContentInactiveTextcolor));
		colorSettings_menuColors.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "Menu body - selected") );
		colorSettings_menuColors.addItem( new CMenuForwarder("Background", true, "", &chContentSelectedcolor ));
		colorSettings_menuColors.addItem( new CMenuForwarder("Textcolor", true, "", &chContentSelectedTextcolor ));
	colorSettings.addItem( new CMenuForwarder("menu colors", true, "", &colorSettings_menuColors) );
		CMenuWidget		colorSettings_statusbarColors;
		colorSettings_statusbarColors.setName("statusbar colors");
		colorSettings_statusbarColors.setIcon("settings.raw");
		colorSettings_statusbarColors.addItem( new CMenuSeparator(4) );
		colorSettings_statusbarColors.addItem( new CMenuForwarder("back") );
			CColorChooser chInfobarcolor("background color", &settings.infobar_red, &settings.infobar_green, &settings.infobar_blue, 
					&settings.menu_Content_inactive_alpha, colorSetupNotifier);
			CColorChooser chInfobarTextcolor("text color", &settings.infobar_Text_red, &settings.infobar_Text_green, &settings.infobar_Text_blue, 
					NULL, colorSetupNotifier);
		colorSettings_statusbarColors.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "Status bars") );
		colorSettings_statusbarColors.addItem( new CMenuForwarder("Background", true, "", &chInfobarcolor ));
		colorSettings_statusbarColors.addItem( new CMenuForwarder("Textcolor", true, "", &chInfobarTextcolor ));
	colorSettings.addItem( new CMenuForwarder("statusbar colors", true, "", &colorSettings_statusbarColors) );

	//keySettings
	keySettings.setName("key setup");
	keySettings.setIcon("settings.raw");
	keySettings.addItem( new CMenuSeparator(4) );
	keySettings.addItem( new CMenuForwarder("back") );
		CKeyChooser			keySettings_tvradio_mode(&settings.key_tvradio_mode, "tv/radio mode key setup", "settings.raw");
		CKeyChooser			keySettings_channelList_pageup(&settings.key_channelList_pageup, "page up key setup", "settings.raw");
		CKeyChooser			keySettings_channelList_pagedown(&settings.key_channelList_pagedown, "page down key setup", "settings.raw");
		CKeyChooser			keySettings_channelList_cancel(&settings.key_channelList_cancel, "cancel key setup", "settings.raw");
		CKeyChooser			keySettings_quickzap_up(&settings.key_quickzap_up, "up key setup", "settings.raw");
		CKeyChooser			keySettings_quickzap_down(&settings.key_quickzap_down, "down key setup", "settings.raw");
	keySettings.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "mode change") );
	keySettings.addItem( new CMenuForwarder("tv/radio mode", true, "", &keySettings_tvradio_mode ));
	keySettings.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "channellist") );
	keySettings.addItem( new CMenuForwarder("page up", true, "", &keySettings_channelList_pageup ));
	keySettings.addItem( new CMenuForwarder("page down", true, "", &keySettings_channelList_pagedown ));
	keySettings.addItem( new CMenuForwarder("cancel", true, "", &keySettings_channelList_cancel ));
	keySettings.addItem( new CMenuSeparator(20, CMenuSeparator::LINE | CMenuSeparator::STRING, "quickzap") );
	keySettings.addItem( new CMenuForwarder("channel up", true, "", &keySettings_quickzap_up ));
	keySettings.addItem( new CMenuForwarder("channel down", true, "", &keySettings_quickzap_down ));



	//init programm
	remoteControl.setZapper(zapit);
	if (zapit)
		remoteControl.tvMode();
	volume = 100;
	channelsInit();
	infoViewer.start(&frameBuffer, &fonts, &settings);
	epgData.start(&frameBuffer, &fonts, &rcInput, &settings);
	channelList->zapTo(&remoteControl, &infoViewer,  0);
	mute = false;
	while(nRun)
	{
		int key = rcInput.getKey(); 

		if (key==CRCInput::RC_setup)
		{
			infoViewer.killTitle();
			mainSettings.exec(&frameBuffer,&fonts, &rcInput, NULL, "");
		}
		else if (key==CRCInput::RC_standby)
		{
			//exit 
			infoViewer.killTitle();
			nRun=false;
		}
		
		if (mode==mode_tv || mode==mode_radio)
		{
			if (key==CRCInput::RC_ok)
			{	//channellist
				infoViewer.killTitle();
				channelList->exec(&frameBuffer,&fonts, &rcInput, &remoteControl, &infoViewer, &settings);
			}
			else if ((key==settings.key_quickzap_up) || (key==settings.key_quickzap_down))
			{
				//quickzap
				channelList->quickZap(&frameBuffer,&fonts, &rcInput, &remoteControl, &infoViewer, &settings, key);
			}
			else if (key==CRCInput::RC_help)
			{	//epg
				infoViewer.killTitle();
				epgData.show( channelList->getActiveChannelName() );
			}
			else if ((key>=0) && (key<=9))
			{ //numeric zap
				channelList->numericZap( &frameBuffer, &fonts, &rcInput, &remoteControl, &infoViewer, key);
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
	sleep(5);
	remoteControl.shutdown();
	sleep(55555);
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
	if (zapit) {
	remoteControl.tvMode();
	channelsInit();
	channelList->zapTo(&remoteControl, &infoViewer,  0);
	}
}

void CNeutrinoApp::radioMode()
{
	mode = mode_radio;
	infoViewer.killTitle();
	frameBuffer.loadPal("dboxradio.pal", 18, 199);
	frameBuffer.paintIcon8("dboxradio.raw",0,0, 18);
	if (zapit) {
	remoteControl.radioMode();
	channelsInit();
	channelList->zapTo(&remoteControl, &infoViewer,  0);
	} else {
	while(rcInput.getKey(1)!=-1);
	rcInput.getKey(600);
	tvMode();
	}
}


/**************************************************************************************
*                                                                                     *
*          CNeutrinoApp -  exec, menuitem callback (shutdown)                         *
*                                                                                     *
**************************************************************************************/
int CNeutrinoApp::exec(CFrameBuffer* frameBuffer, FontsDef *fonts, CRCInput* rcInput, CMenuTarget* parent, string actionKey)
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

