/*
	$Id: setting_helpers.cpp,v 1.179 2009/09/04 11:25:31 rhabarber1848 Exp $

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

#include <sstream>
#include <iomanip>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>

#include <libucodes.h>

#include <config.h>

#include <global.h>
#include <neutrino.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/plugins.h>
#include <daemonc/remotecontrol.h>

extern "C" int pinghost( const char *hostname );

#ifndef TUXTXT_CFG_STANDALONE
extern "C" int  tuxtxt_stop();
extern "C" void tuxtxt_close();
extern "C" int  tuxtxt_init();
extern "C" int  tuxtxt_start(int tpid);
#endif

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

CTP_scanNotifier::CTP_scanNotifier(CMenuOptionChooser* i1, CMenuOptionChooser* i2, CMenuForwarder* i3, CMenuForwarder* i4, CMenuOptionStringChooser* i5)
{
	toDisable1[0]=i1;
	toDisable1[1]=i2;
	toDisable2[0]=i3;
	toDisable2[1]=i4;
	toDisable3[0]=i5;
}

bool CTP_scanNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
//	bool set_true_false=CNeutrinoApp::getInstance()->getScanSettings().TP_scan;
	bool set_true_false = true;

	if ((*((int*) Data) == 0) || (*((int*) Data) == 2)) // all sats || one sat
		set_true_false = false;

	for (int i=0; i<2; i++)
	{
		if (toDisable1[i]) toDisable1[i]->setActive(set_true_false);
		if (toDisable2[i]) toDisable2[i]->setActive(set_true_false);
	}

	if (toDisable3[0]) {
		if (*((int*) Data) == 0) // all sat
			toDisable3[0]->setActive(false);
		else
			toDisable3[0]->setActive(true);
	}
	return true;

}

bool CScanSettingsSatManNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
	(CNeutrinoApp::getInstance()->ScanSettings()).TP_diseqc =
		 *((CNeutrinoApp::getInstance()->ScanSettings()).diseqscOfSat((char*)Data));
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

COnOffNotifier::COnOffNotifier( CMenuItem* a1,CMenuItem* a2,CMenuItem* a3,CMenuItem* a4,CMenuItem* a5)
{
	number = 0;
	if(a1 != NULL){ toDisable[0] =a1;number++;};
	if(a2 != NULL){ toDisable[1] =a2;number++;};
	if(a3 != NULL){ toDisable[2] =a3;number++;};
	if(a4 != NULL){ toDisable[3] =a4;number++;};
	if(a5 != NULL){ toDisable[4] =a5;number++;};
}

bool COnOffNotifier::changeNotify(const neutrino_locale_t, void *Data)
{
	if(*(int*)(Data) == 0)
	{
		for (int i=0; i<number ; i++)
			toDisable[i]->setActive(false);
	}
	else
	{
		for (int i=0; i<number ; i++)
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
			   toDisable[4]->setActive(true);
			   toDisable[5]->setActive(true);
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

CMiscNotifier::CMiscNotifier( CMenuItem* i1, CMenuItem* i2, CMenuItem* i3, CMenuItem* i4)
{
	toDisable[0]=i1;
	toDisable[1]=i2;
	toDisable[2]=i3;
	toDisable[3]=i4;
}

bool CMiscNotifier::changeNotify(const neutrino_locale_t, void *)
{
	toDisable[0]->setActive(!g_settings.shutdown_real);
	toDisable[1]->setActive(!g_settings.shutdown_real);
	toDisable[2]->setActive(!g_settings.shutdown_real);
	toDisable[3]->setActive(!g_settings.shutdown_real);
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

#ifdef HAVE_DBOX_HARDWARE
bool CSPTSNotifier::changeNotify(const neutrino_locale_t, void *)
{
	if (g_settings.misc_spts)
		g_Zapit->PlaybackSPTS();
	else
		g_Zapit->PlaybackPES();

	return true;
}
#endif

#ifndef TUXTXT_CFG_STANDALONE
bool CTuxtxtCacheNotifier::changeNotify(const neutrino_locale_t, void *)
{
	int vtpid=g_RemoteControl->current_PIDs.PIDs.vtxtpid;

	if (g_settings.tuxtxt_cache)
	{
		tuxtxt_init();
		if (vtpid)
			tuxtxt_start(vtpid);
	}
	else
	{
		tuxtxt_stop();
		tuxtxt_close();
	}

	return true;
}
#endif

bool CSectionsdConfigNotifier::changeNotify(const neutrino_locale_t, void *)
{
	CNeutrinoApp::getInstance()->SendSectionsdConfig();
	return true;
}

#ifdef ENABLE_RADIOTEXT
bool CRadiotextNotifier::changeNotify(const neutrino_locale_t, void *)
{
	if (g_settings.radiotext_enable)
	{
		if (g_Radiotext == NULL)
			g_Radiotext = new CRadioText;
		if (g_Radiotext && ((CNeutrinoApp::getInstance()->getMode()) == NeutrinoMessages::mode_radio))
			g_Radiotext->setPid(g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].pid);
	}
	else
	{
		// stop radiotext PES decoding
		if (g_Radiotext)
			g_Radiotext->radiotext_stop();
		delete g_Radiotext;
		g_Radiotext = NULL;
	}

	return true;
}
#endif

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

CZapitSetupNotifier::CZapitSetupNotifier(CMenuForwarder* i1, CMenuForwarder* i2)
{
	toDisable[0]=i1;
	toDisable[1]=i2;
}

bool CZapitSetupNotifier::changeNotify(const neutrino_locale_t OptionName, void * data)
{
	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_ZAPITCONFIG_REMAINING_BOUQUET))
	{
		g_Zapit->setRemainingChannelsBouquet((*((int *)data)) != 0);
		CNeutrinoApp::getInstance()->exec(NULL, "reloadchannels");
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_ZAPITCONFIG_SAVE_LAST_CHANNEL))
	{
		g_Zapit->setSaveLastChannel((*((int *)data)) != 0);
		if((*((int *)data)) == 0)
		{
			for (int i=0; i<2; i++)
			{
				toDisable[i]->setActive(true);
			}
		}
		else
		{
			for (int i=0; i<2; i++)
			{
				toDisable[i]->setActive(false);
			}
		}
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_ZAPITCONFIG_SAVE_AUDIO_PID))
	{
		g_Zapit->setSaveAudioPIDs((*((int *)data)) != 0);
	}
	return true;
}

extern char CstartChannelRadio[30]; /* defined in gui/neutrino_menu.cpp */
extern char CstartChannelTV[30];

void InitZapitChannelHelper(CZapitClient::channelsMode mode)
{
	CZapitChannelExec *ZapitChannelChooser = new CZapitChannelExec;
	std::vector<CMenuWidget *> toDelete;
	CZapitClient zapit;
	CZapitClient::BouquetList bouquetlist;
	zapit.getBouquets(bouquetlist, false, true, mode); // UTF-8
	CZapitClient::BouquetList::iterator bouquet = bouquetlist.begin();
	CMenuWidget mctv(LOCALE_TIMERLIST_BOUQUETSELECT, NEUTRINO_ICON_SETTINGS);

	for(; bouquet != bouquetlist.end();bouquet++)
	{
		CMenuWidget* mwtv = new CMenuWidget(LOCALE_TIMERLIST_CHANNELSELECT, NEUTRINO_ICON_SETTINGS);
		toDelete.push_back(mwtv);
		CZapitClient::BouquetChannelList channellist;
		zapit.getBouquetChannels(bouquet->bouquet_nr,channellist,mode, true); // UTF-8
		CZapitClient::BouquetChannelList::iterator channel = channellist.begin();
		for(; channel != channellist.end();channel++)
		{
			char cChannelId[32];
			sprintf(cChannelId,
				"ZC%c:%d,",
				(mode==CZapitClient::MODE_TV)?'T':'R',
				channel->nr);
			mwtv->addItem(new CMenuForwarderNonLocalized(channel->name, true, NULL, ZapitChannelChooser, (std::string(cChannelId) + channel->name).c_str()));
		}
		if (!channellist.empty())
			mctv.addItem(new CMenuForwarderNonLocalized(bouquet->name, true, NULL, mwtv));
		channellist.clear();
	}
	mctv.exec (NULL, "");
	mctv.hide ();

	// delete dynamic created objects
	for(unsigned int count=0;count<toDelete.size();count++)
	{
		delete toDelete[count];
	}
	toDelete.clear();
}

int CZapitChannelExec::exec(CMenuTarget*, const std::string & actionKey)
{
	int delta;
	unsigned int cnr;
	if (strncmp(actionKey.c_str(), "ZCT:", 4) == 0)
	{
		sscanf(&(actionKey[4]),
			"%u"
			"%n",
			&cnr,
			&delta);
		g_Zapit->setStartChannelTV(cnr-1);
		strcpy(CstartChannelTV,g_Zapit->getChannelNrName(g_Zapit->getStartChannelTV(), CZapitClient::MODE_TV).c_str());
	}
	else if (strncmp(actionKey.c_str(), "ZCR:", 4) == 0)
	{
		sscanf(&(actionKey[4]),
			"%u"
			"%n",
			&cnr,
			&delta);
		g_Zapit->setStartChannelRadio(cnr-1);
		strcpy(CstartChannelRadio,g_Zapit->getChannelNrName(g_Zapit->getStartChannelRadio(), CZapitClient::MODE_RADIO).c_str());
	}
	g_RCInput->postMsg(CRCInput::RC_timeout, 0);
	return menu_return::RETURN_EXIT;
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
	g_RCInput->setRepeat(atoi(g_settings.repeat_blocker), atoi(g_settings.repeat_genericblocker));
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

bool CFdxChangeNotifier::changeNotify(const neutrino_locale_t, void * Data)
{
	g_settings.uboot_dbox_duplex = *(int *)Data;

	return true;
}

bool CTimingSettingsNotifier::changeNotify(const neutrino_locale_t OptionName, void *)
{
	for (int i = 0; i < TIMING_SETTING_COUNT; i++)
	{
		if (ARE_LOCALES_EQUAL(OptionName, timing_setting_name[i]))
		{
			g_settings.timing[i] = atoi(g_settings.timing_string[i]);
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

bool CRecAPIDSettingsNotifier::changeNotify(const neutrino_locale_t, void *)
{
	g_settings.recording_audio_pids_default = ( (g_settings.recording_audio_pids_std ? TIMERD_APIDS_STD : 0) |
						  (g_settings.recording_audio_pids_alt ? TIMERD_APIDS_ALT : 0) |
						  (g_settings.recording_audio_pids_ac3 ? TIMERD_APIDS_AC3 : 0));
	return true;
}

int CAPIDChangeExec::exec(CMenuTarget*, const std::string & actionKey)
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

	if(parent != NULL)
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

#ifdef HAVE_DBOX_HARDWARE
int CUCodeCheckExec::exec(CMenuTarget*, const std::string &)
{
	std::stringstream text;
	char res[60];

	text << g_Locale->getText(LOCALE_UCODECHECK_AVIA500) << ": ";
	checkFile(UCODEDIR "/avia500.ux", (char*) &res);
	text << (std::string)res + '\n' + g_Locale->getText(LOCALE_UCODECHECK_AVIA600) << ": ";
	checkFile(UCODEDIR "/avia600.ux", (char*) &res);
	text << (std::string)res + '\n' + g_Locale->getText(LOCALE_UCODECHECK_UCODE) << ": ";
	checkFile(UCODEDIR "/ucode.bin", (char*) &res);
	if (strcmp("not found", res) == 0)
		text << "ucode_0014 (built-in)";
	else
		text << res;
	text << (std::string)"\n" + g_Locale->getText(LOCALE_UCODECHECK_CAM_ALPHA) << ": ";
	checkFile(UCODEDIR "/cam-alpha.bin", (char*) &res);
	text << res;

	ShowMsgUTF(LOCALE_UCODECHECK_HEAD, text.str(), CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
	return 1;
}
#endif

int CDVBInfoExec::exec(CMenuTarget*, const std::string &)
{
	std::stringstream text;

//	text<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)addr[i]<<':';
	text << g_Locale->getText(LOCALE_TIMERLIST_MODETV) << ": " << CNeutrinoApp::getInstance()->channelListTV->getSize() << "\n";
	text << g_Locale->getText(LOCALE_TIMERLIST_MODERADIO) << ": " << CNeutrinoApp::getInstance()->channelListRADIO->getSize() << "\n \n";
	text << g_Locale->getText(LOCALE_SERVICEMENU_CHAN_EPG_STAT_EPG_STAT) << ":\n" << g_Sectionsd->getStatusinformation() << "\n";

	ShowMsgUTF(LOCALE_SERVICEMENU_CHAN_EPG_STAT, text.str(), CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
	return 1;
}

long CNetAdapter::mac_addr_sys ( u_char *addr) //only for function getMacAddr()
{
	struct ifreq ifr;
	struct ifreq *IFR;
	struct ifconf ifc;
	char buf[1024];
	int s, i;
	int ok = 0;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s==-1) 
	{
		return -1;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	ioctl(s, SIOCGIFCONF, &ifc);
	IFR = ifc.ifc_req;
	for (i = ifc.ifc_len / sizeof(struct ifreq); --i >= 0; IFR++)
	{
		strcpy(ifr.ifr_name, IFR->ifr_name);
		if (ioctl(s, SIOCGIFFLAGS, &ifr) == 0) 
		{
			if (! (ifr.ifr_flags & IFF_LOOPBACK)) 
			{
				if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) 
				{
					ok = 1;
					break;
				}
			}
		}
	}
	close(s);
	if (ok)
	{
		memmove(addr, ifr.ifr_hwaddr.sa_data, 6);
	}
	else
	{
		return -1;
	}
	return 0;
}

std::string CNetAdapter::getMacAddr(void)
{
	long stat;
	u_char addr[6];
	stat = mac_addr_sys( addr);
	if (0 == stat)
	{
		std::stringstream mac_tmp;
		for(int i=0;i<6;++i)
		mac_tmp<<std::hex<<std::setfill('0')<<std::setw(2)<<(int)addr[i]<<':';
		return mac_tmp.str().substr(0,17);
	}
	else
	{
		return "not found";
	}
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

void testNetworkSettings(const char* ip, const char* netmask, const char* broadcast, const char* gateway, const char* nameserver, bool ip_static)
{
	char our_ip[16];
	char our_mask[16];
	char our_broadcast[16];
	char our_gateway[16];
	char our_nameserver[16];
	std::string text, ethID, testsite;
	//set default testdomain and wiki-IP
	std::string defaultsite = "www.google.de", wiki_IP = "88.198.50.98";
	
	//set physical adress
	static CNetAdapter netadapter; ethID=netadapter.getMacAddr();
	
	//get www-domain testsite from /.version 	
	CConfigFile config('\t');
	config.loadConfig("/.version");
	testsite = config.getString("homepage",defaultsite);	
	testsite.replace( 0, testsite.find("www",0), "" );
	
	//use default testdomain if testsite missing
	if (testsite.length()==0) testsite = defaultsite; 

	if (ip_static) {
		strcpy(our_ip, ip);
		strcpy(our_mask, netmask);
		strcpy(our_broadcast, broadcast);
		strcpy(our_gateway, gateway);
		strcpy(our_nameserver, nameserver);
	}
	else {
		netGetIP("eth0", our_ip, our_mask, our_broadcast);
		netGetDefaultRoute(our_gateway);
		netGetNameserver(our_nameserver);
	}
	
	printf("testNw IP: %s\n", our_ip);
	printf("testNw MAC-address%s\n", ethID.c_str());
	printf("testNw Netmask: %s\n", our_mask);
	printf("testNw Broadcast: %s\n", our_broadcast);
	printf("testNw Gateway: %s\n", our_gateway);
	printf("testNw Nameserver: %s\n", our_nameserver);
	printf("testNw Testsite %s\n", testsite.c_str());
 
	text = (std::string)"dbox:\n"
	     + "    " + our_ip + ": " + mypinghost(our_ip) + '\n'
		 + "    " + "eth-ID: " + ethID + '\n'
	     + g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY) + ":\n"
	     + "    " + our_gateway + ": " + ' ' + mypinghost(our_gateway) + '\n'
	     + g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER) + ":\n"
	     + "    " + our_nameserver + ": " + ' ' + mypinghost(our_nameserver) + '\n'
	     + "wiki.tuxbox.org:\n"
	     + "    via IP (" + wiki_IP + "): " + mypinghost(wiki_IP.c_str()) + '\n';
	if (1 == pinghost(our_nameserver)) text += (std::string)
	       "    via DNS: " + mypinghost("wiki.tuxbox.org") + '\n'
	     + testsite + ":\n"
	     + "    via DNS: " + mypinghost(testsite.c_str()) + '\n';

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
	
	netGetIP("eth0", ip, mask, broadcast);
	if (ip[0] == 0) {
		text = "Network inactive\n";
	}
	else {
		netGetNameserver(nameserver);
		netGetDefaultRoute(router);
		text = (std::string)g_Locale->getText(LOCALE_NETWORKMENU_IPADDRESS ) + ": " + ip + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_NETMASK   ) + ": " + mask + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_BROADCAST ) + ": " + broadcast + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_NAMESERVER) + ": " + nameserver + '\n'
				  + g_Locale->getText(LOCALE_NETWORKMENU_GATEWAY   ) + ": " + router;
	}
	ShowMsgUTF(LOCALE_NETWORKMENU_SHOW, text, CMessageBox::mbrBack, CMessageBox::mbBack); // UTF-8
}

unsigned long long getcurrenttime()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );
	return (unsigned long long) tv.tv_usec + (unsigned long long)((unsigned long long) tv.tv_sec * (unsigned long long) 1000000);
}

// USERMENU
#define USERMENU_ITEM_OPTION_COUNT SNeutrinoSettings::ITEM_MAX
const CMenuOptionChooser::keyval USERMENU_ITEM_OPTIONS[USERMENU_ITEM_OPTION_COUNT] =
{
	{SNeutrinoSettings::ITEM_NONE, LOCALE_USERMENU_ITEM_NONE} ,
	{SNeutrinoSettings::ITEM_BAR, LOCALE_USERMENU_ITEM_BAR} ,
	{SNeutrinoSettings::ITEM_EPG_LIST, LOCALE_EPGMENU_EVENTLIST} ,
	{SNeutrinoSettings::ITEM_EPG_SUPER, LOCALE_EPGMENU_EPGPLUS} ,
	{SNeutrinoSettings::ITEM_EPG_INFO, LOCALE_EPGMENU_EVENTINFO} ,
	{SNeutrinoSettings::ITEM_EPG_MISC, LOCALE_USERMENU_ITEM_EPG_MISC} ,
	{SNeutrinoSettings::ITEM_AUDIO_SELECT, LOCALE_AUDIOSELECTMENUE_HEAD} ,
	{SNeutrinoSettings::ITEM_SUBCHANNEL, LOCALE_INFOVIEWER_SUBSERVICE} ,
	{SNeutrinoSettings::ITEM_PLUGIN, LOCALE_TIMERLIST_PLUGIN} ,
	{SNeutrinoSettings::ITEM_VTXT, LOCALE_USERMENU_ITEM_VTXT} ,
	{SNeutrinoSettings::ITEM_RECORD, LOCALE_TIMERLIST_TYPE_RECORD} ,
	{SNeutrinoSettings::ITEM_MOVIEPLAYER_TS, LOCALE_MAINMENU_MOVIEPLAYER} ,
	{SNeutrinoSettings::ITEM_MOVIEPLAYER_MB, LOCALE_MOVIEBROWSER_HEAD} ,
	{SNeutrinoSettings::ITEM_TIMERLIST, LOCALE_TIMERLIST_NAME} ,
	{SNeutrinoSettings::ITEM_REMOTE, LOCALE_RCLOCK_MENUEADD} ,
	{SNeutrinoSettings::ITEM_FAVORITS, LOCALE_FAVORITES_MENUEADD} ,
	{SNeutrinoSettings::ITEM_TECHINFO, LOCALE_EPGMENU_STREAMINFO}
};

int CUserMenuMenu::exec(CMenuTarget* parent, const std::string &)
{
	if(parent != NULL)
		parent->hide();

	CMenuWidget menu (local , "keybinding.raw");
	menu.addItem(GenericMenuSeparator);
	menu.addItem(GenericMenuBack);
	menu.addItem(GenericMenuSeparatorLine);
	
	CStringInputSMS name(LOCALE_USERMENU_NAME,    &g_settings.usermenu_text[button], 11, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyzäöüß/- ");
	menu.addItem(new CMenuForwarder(LOCALE_USERMENU_NAME,    true, g_settings.usermenu_text[button],&name));
	menu.addItem(GenericMenuSeparatorLine);
	
	char text[10];
	for(int item = 0; item < SNeutrinoSettings::ITEM_MAX && item <13; item++) // Do not show more than 13 items
	{
		snprintf(text,10,"%d:",item);
		text[9]=0;// terminate for sure
		menu.addItem( new CMenuOptionChooser(text, &g_settings.usermenu[button][item], USERMENU_ITEM_OPTIONS, USERMENU_ITEM_OPTION_COUNT,true ));
	}

	menu.exec(NULL,"");

	return menu_return::RETURN_REPAINT;	
}
