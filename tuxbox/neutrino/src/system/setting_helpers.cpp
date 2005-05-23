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

#include <system/setting_helpers.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "libnet.h"

#include <libucodes.h>

#include <config.h>

#include <global.h>
#include <neutrino.h>

// obsolete #include <gui/streaminfo.h>

#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>

#include <gui/plugins.h>
#include <daemonc/remotecontrol.h>
extern CPlugins       * g_PluginList;    /* neutrino.cpp */
extern CRemoteControl * g_RemoteControl; /* neutrino.cpp */

extern "C" int pinghost( const char *hostname );

bool CSatDiseqcNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	if (*((int*) Data) == NO_DISEQC)
	{
		satMenu->setActive(true);
		extMenu->setActive(false);
		extMotorMenu->setActive(false);
		repeatMenu->setActive(false);
	}
	else
	if (*((int*) Data) == DISEQC_1_2)
	{
		satMenu->setActive(true);
		extMenu->setActive(true);
		extMotorMenu->setActive(true);
		repeatMenu->setActive(true);
	}
	else
	{
		satMenu->setActive(false);
		extMenu->setActive(true);
		extMotorMenu->setActive(false);
		repeatMenu->setActive((*((int*) Data) != DISEQC_1_0));
	}
	return true;
}

CDHCPNotifier::CDHCPNotifier( CMenuForwarder* a1, CMenuForwarder* a2, CMenuForwarder* a3, CMenuForwarder* a4, CMenuForwarder* a5)
{
	toDisable[0] = a1;
	toDisable[1] = a2;
	toDisable[2] = a3;
	toDisable[3] = a4;
	toDisable[4] = a5;
}


bool CDHCPNotifier::changeNotify(const neutrino_locale_t, void * data)
{
	CNeutrinoApp::getInstance()->networkConfig.inet_static = ((*(int*)(data)) == 0);
	for(int x=0;x<5;x++)
		toDisable[x]->setActive(CNeutrinoApp::getInstance()->networkConfig.inet_static);
	return true;
}

CStreamingNotifier::CStreamingNotifier( CMenuItem* i1, CMenuItem* i2, CMenuItem* i3, CMenuItem* i4, CMenuItem* i5, CMenuItem* i6, CMenuItem* i7, CMenuItem* i8, CMenuItem* i9, CMenuItem* i10, CMenuItem* i11)
{
   toDisable[0]=i1;
   toDisable[1]=i2;
   toDisable[2]=i3;
   toDisable[3]=i4;
   toDisable[4]=i5;
   toDisable[5]=i6;
   toDisable[6]=i7;
   toDisable[7]=i8;
   toDisable[8]=i9;
   toDisable[9]=i10;
   toDisable[10]=i11;
}

bool CStreamingNotifier::changeNotify(const neutrino_locale_t, void *)
{
   if(g_settings.streaming_type==0)
   {
      for (int i=0; i<=10; i++)
        toDisable[i]->setActive(false);
   }
   else if(g_settings.streaming_type==1)
   {
      for (int i=0; i<=10; i++)
        toDisable[i]->setActive(true);
   }
   return true;
}

CRecordingNotifier::CRecordingNotifier(CMenuItem* i1 , CMenuItem* i2 , CMenuItem* i3 ,
                                       CMenuItem* i4 , CMenuItem* i5 , CMenuItem* i6 ,
                                       CMenuItem* i7 , CMenuItem* i8 , CMenuItem* i9)
{
	toDisable[ 0] = i1;
	toDisable[ 1] = i2;
	toDisable[ 2] = i3;
	toDisable[ 3] = i4;
	toDisable[ 4] = i5;
	toDisable[ 5] = i6;
	toDisable[ 6] = i7;
	toDisable[ 7] = i8;
	toDisable[ 8] = i9;
}
bool CRecordingNotifier::changeNotify(const neutrino_locale_t, void *)
{
   if ((g_settings.recording_type == CNeutrinoApp::RECORDING_OFF) ||
       (g_settings.recording_type == CNeutrinoApp::RECORDING_FILE))
   {
	   for(int i = 0; i < 9; i++)
		   toDisable[i]->setActive(false);

	   if (g_settings.recording_type == CNeutrinoApp::RECORDING_FILE)
	   {
		   toDisable[7]->setActive(true);
		   toDisable[8]->setActive(true);
	   }
   }
   else if (g_settings.recording_type == CNeutrinoApp::RECORDING_SERVER)
   {
	   toDisable[0]->setActive(true);
	   toDisable[1]->setActive(true);
	   toDisable[2]->setActive(true);
	   toDisable[3]->setActive(g_settings.recording_server_wakeup==1);
	   toDisable[4]->setActive(true);
	   toDisable[5]->setActive(true);
	   toDisable[6]->setActive(false);
	   toDisable[7]->setActive(false);
	   toDisable[8]->setActive(true);
   }
   else if (g_settings.recording_type == CNeutrinoApp::RECORDING_VCR)
   {

	   toDisable[0]->setActive(false);
	   toDisable[1]->setActive(false);
	   toDisable[2]->setActive(false);
	   toDisable[3]->setActive(false);
	   toDisable[4]->setActive(false);
	   toDisable[5]->setActive(false);
	   toDisable[6]->setActive(true);
	   toDisable[7]->setActive(false);
	   toDisable[8]->setActive(false);
   }

   return true;
}

CRecordingNotifier2::CRecordingNotifier2( CMenuItem* i)
{
   toDisable[0]=i;
}
bool CRecordingNotifier2::changeNotify(const neutrino_locale_t, void *)
{
   toDisable[0]->setActive(g_settings.recording_server_wakeup==1);
   return true;
}

bool CRecordingSafetyNotifier::changeNotify(const neutrino_locale_t, void *)
{
	g_Timerd->setRecordingSafety(atoi(g_settings.record_safety_time_before)*60, atoi(g_settings.record_safety_time_after)*60);
   return true;
}

CMiscNotifier::CMiscNotifier( CMenuItem* i1)
{
   toDisable[0]=i1;
}
bool CMiscNotifier::changeNotify(const neutrino_locale_t, void *)
{
   toDisable[0]->setActive(!g_settings.shutdown_real);
   return true;
}

bool CLcdNotifier::changeNotify(const neutrino_locale_t, void *)
{
	CLCD::getInstance()->setlcdparameter();
	CLCD::getInstance()->setAutoDimm(g_settings.lcd_setting[SNeutrinoSettings::LCD_AUTODIMM]);
	return true;
}

bool CPauseSectionsdNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	g_Sectionsd->setPauseScanning((*((int *)Data)) == 0);

	return true;
}

bool CSPTSNotifier::changeNotify(const neutrino_locale_t, void *)
{
	if (g_settings.misc_spts)
		g_Zapit->PlaybackSPTS();
	else
		g_Zapit->PlaybackPES();

	return true;
}

bool CTouchFileNotifier::changeNotify(const neutrino_locale_t, void * data)
{
	if ((*(int *)data) != 0)
	{
		FILE * fd = fopen(filename, "w");
		if (fd)
			fclose(fd);
		else
			return false;
	}
	else
		remove(filename);
	return true;
}

bool CColorSetupNotifier::changeNotify(const neutrino_locale_t, void *)
{
	CFrameBuffer *frameBuffer = CFrameBuffer::getInstance();
//	unsigned char r,g,b;
	//setting colors-..
	frameBuffer->paletteGenFade(COL_MENUHEAD,
	                              convertSetupColor2RGB(g_settings.menu_Head_red, g_settings.menu_Head_green, g_settings.menu_Head_blue),
	                              convertSetupColor2RGB(g_settings.menu_Head_Text_red, g_settings.menu_Head_Text_green, g_settings.menu_Head_Text_blue),
	                              8, convertSetupAlpha2Alpha( g_settings.menu_Head_alpha ) );

	frameBuffer->paletteGenFade(COL_MENUCONTENT,
	                              convertSetupColor2RGB(g_settings.menu_Content_red, g_settings.menu_Content_green, g_settings.menu_Content_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );


	frameBuffer->paletteGenFade(COL_MENUCONTENTDARK,
	                              convertSetupColor2RGB(int(g_settings.menu_Content_red*0.6), int(g_settings.menu_Content_green*0.6), int(g_settings.menu_Content_blue*0.6)),
	                              convertSetupColor2RGB(g_settings.menu_Content_Text_red, g_settings.menu_Content_Text_green, g_settings.menu_Content_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_alpha) );

	frameBuffer->paletteGenFade(COL_MENUCONTENTSELECTED,
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_red, g_settings.menu_Content_Selected_green, g_settings.menu_Content_Selected_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_Selected_Text_red, g_settings.menu_Content_Selected_Text_green, g_settings.menu_Content_Selected_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_Selected_alpha) );

	frameBuffer->paletteGenFade(COL_MENUCONTENTINACTIVE,
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_red, g_settings.menu_Content_inactive_green, g_settings.menu_Content_inactive_blue),
	                              convertSetupColor2RGB(g_settings.menu_Content_inactive_Text_red, g_settings.menu_Content_inactive_Text_green, g_settings.menu_Content_inactive_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.menu_Content_inactive_alpha) );

	frameBuffer->paletteGenFade(COL_INFOBAR,
	                              convertSetupColor2RGB(g_settings.infobar_red, g_settings.infobar_green, g_settings.infobar_blue),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );

/*	frameBuffer->paletteSetColor( COL_INFOBAR_SHADOW,
	                                convertSetupColor2RGB(
	                                    int(g_settings.infobar_red*0.4),
	                                    int(g_settings.infobar_green*0.4),
	                                    int(g_settings.infobar_blue*0.4)),
	                                g_settings.infobar_alpha);
*/
	frameBuffer->paletteGenFade(COL_INFOBAR_SHADOW,
	                              convertSetupColor2RGB(int(g_settings.infobar_red*0.4), int(g_settings.infobar_green*0.4), int(g_settings.infobar_blue*0.4)),
	                              convertSetupColor2RGB(g_settings.infobar_Text_red, g_settings.infobar_Text_green, g_settings.infobar_Text_blue),
	                              8, convertSetupAlpha2Alpha(g_settings.infobar_alpha) );


	frameBuffer->paletteSet();
	return false;
}

bool CAudioSetupNotifier::changeNotify(const neutrino_locale_t OptionName, void *)
{
	//printf("notify: %d\n", OptionName);

	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_PCMOFFSET))
	{
		if (g_settings.audio_avs_Control == 2)   //lirc
			g_Controld->setVolume(100 - atoi(g_settings.audio_PCMOffset), CControld::TYPE_OST);
	}

	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_AUDIOMENU_ANALOGOUT))
	{
		g_Zapit->setAudioMode(g_settings.audio_AnalogMode);
	}
	return true;
}

CAudioSetupNotifier2::CAudioSetupNotifier2( CMenuItem* i1)
{
   toDisable[0]=i1;
}

bool CAudioSetupNotifier2::changeNotify(const neutrino_locale_t, void *)
{
	toDisable[0]->setActive(g_settings.audio_avs_Control == 2);

	if (g_settings.audio_avs_Control == 2)
		g_Controld->setVolume(100 - atoi(g_settings.audio_PCMOffset), CControld::TYPE_OST);
	// tell controld the new volume_type
	g_Controld->setVolume(g_Controld->getVolume((CControld::volume_type)g_settings.audio_avs_Control),
									 (CControld::volume_type)g_settings.audio_avs_Control);
	return true;
}

bool CKeySetupNotifier::changeNotify(const neutrino_locale_t, void *)
{
	g_RCInput->repeat_block = atoi(g_settings.repeat_blocker)* 1000;
	g_RCInput->repeat_block_generic = atoi(g_settings.repeat_genericblocker)* 1000;
	return false;
}

bool CShutdownCountNotifier::changeNotify(const neutrino_locale_t, void *)
{
 	printf("[neutrino] shutdown counter changed to %d minutes\n", atoi(g_settings.shutdown_count));
	return true;
}

bool CIPChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	char ip[16];
	unsigned char _ip[4];
	sscanf((char*) Data, "%hhu.%hhu.%hhu.%hhu", &_ip[0], &_ip[1], &_ip[2], &_ip[3]);

	sprintf(ip, "%hhu.%hhu.%hhu.255", _ip[0], _ip[1], _ip[2]);
	CNeutrinoApp::getInstance()->networkConfig.broadcast = ip;

	CNeutrinoApp::getInstance()->networkConfig.netmask = (_ip[0] == 10) ? "255.0.0.0" : "255.255.255.0";

	return true;
}

bool CConsoleDestChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	g_settings.uboot_console = *(int *)Data;

	return true;
}

bool CTimingSettingsNotifier::changeNotify(const neutrino_locale_t OptionName, void *)
{
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		if (ARE_LOCALES_EQUAL(OptionName, timing_setting_name[i]))
		{
			g_settings.timing[i] = 	atoi(g_settings.timing_string[i]);
			return true;
		}
	}
	return false;
}

bool CFontSizeNotifier::changeNotify(const neutrino_locale_t, void *)
{
	CHintBox hintBox(LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_FONTSIZE_HINT)); // UTF-8
	hintBox.paint();

	CNeutrinoApp::getInstance()->SetupFonts();

	hintBox.hide();

	return true;
}

int CAPIDChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//    printf("CAPIDChangeExec exec: %s\n", actionKey.c_str());
	unsigned int sel= atoi(actionKey.c_str());
	if (g_RemoteControl->current_PIDs.PIDs.selected_apid!= sel )
	{
		g_RemoteControl->setAPID(sel);
	}
	return menu_return::RETURN_EXIT;
}


int CNVODChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//    printf("CNVODChangeExec exec: %s\n", actionKey.c_str());
	unsigned sel= atoi(actionKey.c_str());
	g_RemoteControl->setSubChannel(sel);

	parent->hide();
	g_InfoViewer->showSubchan();
	return menu_return::RETURN_EXIT;
}

int CStreamFeaturesChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	//printf("CStreamFeaturesChangeExec exec: %s\n", actionKey.c_str());
	int sel= atoi(actionKey.c_str());

	parent->hide();
	// -- obsolete (rasc 2004-06-10)
	// if (sel==-1)
	// {
	// 	CStreamInfo StreamInfo;
	//	StreamInfo.exec(NULL, "");
	// } else
	if (sel>=0)
	{
		g_PluginList->startPlugin(sel,0);
	}

	return menu_return::RETURN_EXIT;
}

int CMoviePluginChangeExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	int sel= atoi(actionKey.c_str());
	parent->hide();
	if (sel>=0)
	{
			g_settings.movieplayer_plugin=g_PluginList->getName(sel);
	}
	return menu_return::RETURN_EXIT;
}

int CUCodeCheckExec::exec(CMenuTarget* parent, const std::string & actionKey)
{
	std::string text;
	char res[60];

	text = g_Locale->getText(LOCALE_UCODECHECK_AVIA500);
	text += ": ";
	checkFile(UCODEDIR "/avia500.ux", (char*) &res);
	text += res;
	text += '\n';
	text += g_Locale->getText(LOCALE_UCODECHECK_AVIA600);
	text += ": ";
	checkFile(UCODEDIR "/avia600.ux", (char*) &res);
	text += res;
	text += '\n';
	text += g_Locale->getText(LOCALE_UCODECHECK_UCODE);
	text += ": ";
	checkFile(UCODEDIR "/ucode.bin", (char*) &res);
	if (strcmp("not found", res) == 0)
		text += "ucode_0014 (built-in)";
	else
		text += res;
	text += '\n';
	text += g_Locale->getText(LOCALE_UCODECHECK_CAM_ALPHA);
	text += ": ";
	checkFile(UCODEDIR "/cam-alpha.bin", (char*) &res);
	text += res;

	ShowMsgUTF(LOCALE_UCODECHECK_HEAD, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
	return 1;
}

const char * mypinghost(const char * const host)
{
	int retvalue = pinghost(host);
	switch (retvalue)
	{
		case 1: return (g_Locale->getText(LOCALE_PING_OK));
		case 0: return (g_Locale->getText(LOCALE_PING_UNREACHABLE));
		case -1: return (g_Locale->getText(LOCALE_PING_PROTOCOL));
		case -2: return (g_Locale->getText(LOCALE_PING_SOCKET));
	}
	return "";
}

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool dhcp)
{
	char our_ip[16];
	char our_mask[16];
	char our_broadcast[16];
	char our_gateway[16];
	char our_nameserver[16];
	std::string text;

	if (!dhcp) {
		strcpy(our_ip,ip);
		strcpy(our_mask,netmask);
		strcpy(our_broadcast,broadcast);
		strcpy(our_gateway,gateway);
		strcpy(our_nameserver,nameserver);
	}
	else {
		netGetIP("eth0",our_ip,our_mask,our_broadcast);
		netGetDefaultRoute(our_gateway);
		netGetNameserver(our_nameserver);
	}

	printf("testNw IP       : %s\n", our_ip);
	printf("testNw Netmask  : %s\n", our_mask);
	printf("testNw Broadcast: %s\n", our_broadcast);
	printf("testNw Gateway: %s\n", our_gateway);
	printf("testNw Nameserver: %s\n", our_nameserver);

	text = our_ip;
	text += ": ";
	text += mypinghost(our_ip);
	text += '\n';
	text += g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY);
	text += ": ";
	text += our_gateway;
	text += ' ';
	text += mypinghost(our_gateway);
	text += '\n';
	text += g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER);
	text += ": ";
	text += our_nameserver;
	text += ' ';
	text += mypinghost(our_nameserver);
	text += "\ndboxupdate.berlios.de: ";
	text += mypinghost("195.37.77.138");

	ShowMsgUTF(LOCALE_NETWORKMENU_TEST, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

void showCurrentNetworkSettings()
{
	char ip[16];
	char mask[16];
	char broadcast[16];
	char router[16];
	char nameserver[16];
	std::string text;

	netGetIP("eth0",ip,mask,broadcast);
	if (ip[0] == 0) {
		text = "Network inactive\n";
	}
	else {
		netGetNameserver(nameserver);
		netGetDefaultRoute(router);
		text  = g_Locale->getText(LOCALE_NETWORKMENU_IPADDRESS );
		text += ": ";
		text += ip;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_NETMASK   );
		text += ": ";
		text += mask;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_BROADCAST );
		text += ": ";
		text += broadcast;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER);
		text += ": ";
		text += nameserver;
		text += '\n';
		text += g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY   );
		text += ": ";
		text += router;
	}
	ShowMsgUTF(LOCALE_NETWORKMENU_SHOW, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

unsigned long long getcurrenttime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
}
