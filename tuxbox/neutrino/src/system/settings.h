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

#ifndef __settings__
#define __settings__


struct SNeutrinoSettings
{
	//video
	int video_Signal;
	int video_Format;

	//misc
	int box_Type;
	int shutdown_real;
	int shutdown_showclock;
	int show_camwarning;

	//audio
	int audio_AnalogMode;
	int audio_DolbyDigital;

	//vcr
	int	vcr_AutoSwitch;

	//language
	char language[25];

	//timing
	int timing_menu;
	int timing_chanlist;
	int timing_epg;
	int timing_infobar;

	//scan
	int scan_astra;
	int scan_eutel;
	int scan_kopernikus;
	int scan_sirius;
	int scan_thor;
	int scan_digituerk;
	int scan_tuerksat;
	int scan_bouquet;

	//colors
	unsigned char menu_Head_alpha;
	unsigned char menu_Head_red;
	unsigned char menu_Head_green;
	unsigned char menu_Head_blue;

	unsigned char menu_Head_Text_alpha;
	unsigned char menu_Head_Text_red;
	unsigned char menu_Head_Text_green;
	unsigned char menu_Head_Text_blue;

	unsigned char menu_Content_alpha;
	unsigned char menu_Content_red;
	unsigned char menu_Content_green;
	unsigned char menu_Content_blue;

	unsigned char menu_Content_Text_alpha;
	unsigned char menu_Content_Text_red;
	unsigned char menu_Content_Text_green;
	unsigned char menu_Content_Text_blue;

	unsigned char menu_Content_Selected_alpha;
	unsigned char menu_Content_Selected_red;
	unsigned char menu_Content_Selected_green;
	unsigned char menu_Content_Selected_blue;

	unsigned char menu_Content_Selected_Text_alpha;
	unsigned char menu_Content_Selected_Text_red;
	unsigned char menu_Content_Selected_Text_green;
	unsigned char menu_Content_Selected_Text_blue;

	unsigned char menu_Content_inactive_alpha;
	unsigned char menu_Content_inactive_red;
	unsigned char menu_Content_inactive_green;
	unsigned char menu_Content_inactive_blue;

	unsigned char menu_Content_inactive_Text_alpha;
	unsigned char menu_Content_inactive_Text_red;
	unsigned char menu_Content_inactive_Text_green;
	unsigned char menu_Content_inactive_Text_blue;

	unsigned char infobar_alpha;
	unsigned char infobar_red;
	unsigned char infobar_green;
	unsigned char infobar_blue;

	unsigned char infobar_Text_alpha;
	unsigned char infobar_Text_red;
	unsigned char infobar_Text_green;
	unsigned char infobar_Text_blue;

	//network
	int networkSetOnStartup;
	int network_dhcp;
	char network_ip[20];
	char network_netmask[20];
	char network_broadcast[20];
	char network_defaultgateway[20];
	char network_nameserver[20];

	//streaming
	int network_streaming_use;
	char network_streamingserver[31];
	char network_streamingserverport[10];


	//key configuration
	int key_tvradio_mode;

	int key_channelList_pageup;
	int key_channelList_pagedown;
	int key_channelList_cancel;

	int key_quickzap_up;
	int key_quickzap_down;
	int key_bouquet_up;
	int key_bouquet_down;
	int key_subchannel_up;
	int key_subchannel_down;

	char repeat_blocker[4];
	char repeat_genericblocker[4];

	//screen configuration
	int	screen_StartX;
	int	screen_StartY;
	int	screen_EndX;
	int	screen_EndY;

	//Software-update
	int softupdate_mode;
	char softupdate_currentversion[20];
	char softupdate_proxyserver[31];
	char softupdate_proxyusername[31];
	char softupdate_proxypassword[31];

	//BouquetHandling
	int bouquetlist_mode;

	// parentallock
	int parentallock_prompt;
	int parentallock_lockage;
	char parentallock_pincode[4];
};

const int PARENTALLOCK_PROMPT_NEVER          = 0;
const int PARENTALLOCK_PROMPT_ONSTART        = 1;
const int PARENTALLOCK_PROMPT_CHANGETOLOCKED = 2;
const int PARENTALLOCK_PROMPT_ONSIGNAL       = 3;

#endif

