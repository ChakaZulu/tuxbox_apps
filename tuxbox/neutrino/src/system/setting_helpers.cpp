/*
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

#include "setting_helpers.h"
#include "../global.h"

bool CStartNeutrinoDirectNotifier::changeNotify(string OptionName, void* Data)
{
	if( *((int*) Data)!=0)
	{	//file anlegen (direktstart)
		FILE* fd = fopen("/var/etc/.neutrino", "w");
		if(fd)
		{
			fclose(fd);
		}
	}
	else
	{
		remove("/var/etc/.neutrino");
	}
}


bool CColorSetupNotifier::changeNotify(string OptionName, void*)
{
	unsigned char r,g,b;
	//setting colors-..
	g_FrameBuffer->paletteGenFade(COL_MENUHEAD,
	                              convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue),
	                              convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue),
	                              8, convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ) );

	g_FrameBuffer->paletteGenFade(COL_MENUCONTENT,
	                              convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );


	g_FrameBuffer->paletteGenFade(COL_MENUCONTENTDARK,
	                              convertSetupColor2RGB(int(g_settings.menu_Content_red*0.6), int(g_settings.menu_Content_green*0.6), int(g_settings.menu_Content_blue*0.6)),
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

/*	g_FrameBuffer->paletteSetColor( COL_INFOBAR_SHADOW,
	                                convertSetupColor2RGB(
	                                    int(g_settings.infobar_red*0.4),
	                                    int(g_settings.infobar_green*0.4),
	                                    int(g_settings.infobar_blue*0.4)),
	                                g_settings.infobar_alpha);
*/
	g_FrameBuffer->paletteGenFade(COL_INFOBAR_SHADOW,
	                              convertSetupColor2RGB(int(g_settings.infobar_red*0.4), int(g_settings.infobar_green*0.4), int(g_settings.infobar_blue*0.4)),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );


	g_FrameBuffer->paletteSet();
	return false;
}

bool CAudioSetupNotifier::changeNotify(string OptionName, void*)
{
	printf("notify: %s\n", OptionName.c_str() );
	return false;
}

bool CVideoSetupNotifier::changeNotify(string OptionName, void*)
{
	if(OptionName=="videomenu.videosignal")
	{
		g_Controld->setVideoOutput( g_settings.video_Signal );
	}
	else if(OptionName=="videomenu.videoformat")
	{
		g_Controld->setVideoFormat( g_settings.video_Format );
	}

	printf("video notify: %s\n", OptionName.c_str() );
	return false;
}

bool CBoxTypeSetupNotifier::changeNotify(string OptionName, void*)
{
	g_Controld->setBoxType( g_settings.box_Type );
}

bool CLanguageSetupNotifier::changeNotify(string OptionName, void*)
{
	//	printf("language notify: %s - %s\n", OptionName.c_str(), g_settings.language );
	g_Locale->loadLocale(g_settings.language);
	return true;
}

bool CKeySetupNotifier::changeNotify(string OptionName, void*)
{
	//    printf("CKeySetupNotifier notify: %s\n", OptionName.c_str() );
	g_RCInput->repeat_block = atoi(g_settings.repeat_blocker)* 1000;
	g_RCInput->repeat_block_generic = atoi(g_settings.repeat_genericblocker)* 1000;
	return false;
}

int CAPIDChangeExec::exec(CMenuTarget* parent, string actionKey)
{
	//    printf("CAPIDChangeExec exec: %s\n", actionKey.c_str());
	int sel= atoi(actionKey.c_str());
	if (g_RemoteControl->audio_chans.selected!= sel )
		g_RemoteControl->setAPID(atoi(actionKey.c_str()));
	return RETURN_EXIT;
}

int CNVODChangeExec::exec(CMenuTarget* parent, string actionKey)
{
	//    printf("CNVODChangeExec exec: %s\n", actionKey.c_str());
	unsigned sel= atoi(actionKey.c_str());
	g_RemoteControl->setSubChannel(sel);
	g_RCInput->pushbackKey(CRCInput::RC_help);
	return RETURN_EXIT;
}

int CStreamFeaturesChangeExec::exec(CMenuTarget* parent, string actionKey)
{
	//printf("CStreamFeaturesChangeExec exec: %s\n", actionKey.c_str());
	int sel= atoi(actionKey.c_str());

	parent->hide();
	if (sel==-1)
	{
		g_StreamInfo->exec(NULL, "");
	}
	else
	if (sel>=0)
	{
		g_PluginList->setvtxtpid( g_RemoteControl->vtxtpid );
		g_PluginList->startPlugin( sel );
	}

	return RETURN_EXIT;
}


void setNetworkAddress(char* ip, char* netmask, char* broadcast)
{
	printf("IP       : %s\n", ip);
	printf("Netmask  : %s\n", netmask);
	printf("Broadcast: %s\n", broadcast);
	netSetIP( "eth0", ip, netmask, broadcast);
}

void setDefaultGateway(char* ip)
{
	printf("Gateway  : %s\n", ip);
	netSetDefaultRoute( ip );
}

void setNameServer(char* ip)
{
	FILE* fd = fopen("/etc/resolv.conf", "w");
	if(fd)
	{
		fprintf(fd, "# resolv.conf - generated by neutrino\n\n");
		fprintf(fd, "nameserver %s\n", ip);
		fclose(fd);
	}
	else
	{
		perror("cannot write /etc/resolv.conf");
	}
}

