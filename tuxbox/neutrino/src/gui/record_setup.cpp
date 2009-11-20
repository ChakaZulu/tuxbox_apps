/*
	$Id: record_setup.cpp,v 1.1 2009/11/20 22:21:13 dbt Exp $

	record setup implementation - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2009 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/


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

	$Log: record_setup.cpp,v $
	Revision 1.1  2009/11/20 22:21:13  dbt
	init recordig setup for it's own modul
	
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/record_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/dirchooser.h>
#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <driver/screen_max.h>

#include <system/debug.h>


CRecordSetup::CRecordSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CRecordSetup::~CRecordSetup()
{

}

void CRecordSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width, height);
}

int CRecordSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init record setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	if(actionKey=="recording")
	{
		CNeutrinoApp::getInstance()->setupRecordingDevice();
		return res;
	}
	else if(actionKey == "help_recording")
	{
		ShowLocalizedMessage(LOCALE_SETTINGS_HELP, LOCALE_RECORDINGMENU_HELP, CMessageBox::mbrBack, CMessageBox::mbBack);
		return res;
	}


	showRecordSetup();
	
	return res;
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};

#define RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT 4
const CMenuOptionChooser::keyval RECORDINGMENU_RECORDING_TYPE_OPTIONS[RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT] =
{
	{ CNeutrinoApp::RECORDING_OFF   , LOCALE_RECORDINGMENU_OFF    },
	{ CNeutrinoApp::RECORDING_SERVER, LOCALE_RECORDINGMENU_SERVER },
	{ CNeutrinoApp::RECORDING_VCR   , LOCALE_RECORDINGMENU_VCR    },
	{ CNeutrinoApp::RECORDING_FILE  , LOCALE_RECORDINGMENU_FILE   }
};

#define RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT 3
const CMenuOptionChooser::keyval RECORDINGMENU_STOPSECTIONSD_OPTIONS[RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT] =
{
	{ 0, LOCALE_RECORDINGMENU_SECTIONSD_RUN     },
	{ 1, LOCALE_RECORDINGMENU_SECTIONSD_STOP    },
	{ 2, LOCALE_RECORDINGMENU_SECTIONSD_RESTART }
};

#define RECORDINGMENU_RINGBUFFER_SIZE_COUNT 5
const CMenuOptionChooser::keyval RECORDINGMENU_RINGBUFFER_SIZES[RECORDINGMENU_RINGBUFFER_SIZE_COUNT] =
{
	{ 0, LOCALE_RECORDINGMENU_RINGBUFFERS_05M },
	{ 1, LOCALE_RECORDINGMENU_RINGBUFFERS_1M },
	{ 2, LOCALE_RECORDINGMENU_RINGBUFFERS_2M },
	{ 3, LOCALE_RECORDINGMENU_RINGBUFFERS_4M },
	{ 4, LOCALE_RECORDINGMENU_RINGBUFFERS_8M }
};

void CRecordSetup::showRecordSetup()
{
	//menue init
	CMenuWidget* recordingSettings = new CMenuWidget(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS, width);

	//subhead
	recordingSettings->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MAINSETTINGS_RECORDING));	

	//prepare input record server ip
	CIPInput * recordingSettings_server_ip = new CIPInput(LOCALE_RECORDINGMENU_SERVER_IP, g_settings.recording_server_ip, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	//input record server ip
	CMenuForwarder * mf1 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_IP, (g_settings.recording_type == RECORDING_SERVER), g_settings.recording_server_ip, recordingSettings_server_ip);

	//prepare input record server port
	CStringInput * recordingSettings_server_port = new CStringInput(LOCALE_RECORDINGMENU_SERVER_PORT, g_settings.recording_server_port, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
	//input record server port
	CMenuForwarder * mf2 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_PORT, (g_settings.recording_type == RECORDING_SERVER), g_settings.recording_server_port, recordingSettings_server_port);

	//prepare input record server mac address
	CMACInput * recordingSettings_server_mac = new CMACInput(LOCALE_RECORDINGMENU_SERVER_MAC,  g_settings.recording_server_mac, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	//input record server mac address
	CMenuForwarder * mf3 = new CMenuForwarder(LOCALE_RECORDINGMENU_SERVER_MAC, ((g_settings.recording_type == RECORDING_SERVER) && g_settings.recording_server_wakeup==1), g_settings.recording_server_mac, recordingSettings_server_mac);

	CRecordingNotifier2 * RecordingNotifier2 = new CRecordingNotifier2(mf3);

	//prepare choose wol
	CMenuOptionChooser * oj2 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_SERVER_WAKEUP, &g_settings.recording_server_wakeup, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER), RecordingNotifier2);

	//prepare playback stop
	CMenuOptionChooser* oj3 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STOPPLAYBACK, &g_settings.recording_stopplayback, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE));

	//prepare epg stop
	CMenuOptionChooser* oj4 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_SECTIONSD, &g_settings.recording_stopsectionsd, RECORDINGMENU_STOPSECTIONSD_OPTIONS, RECORDINGMENU_STOPSECTIONSD_OPTION_COUNT, (g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE));

	//prepare zap on announce
	CMenuOptionChooser* oj4b = new CMenuOptionChooser(LOCALE_RECORDINGMENU_ZAP_ON_ANNOUNCE, &g_settings.recording_zap_on_announce, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	//prepare no scart switch
	CMenuOptionChooser* oj5 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_NO_SCART, &g_settings.recording_vcr_no_scart, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, (g_settings.recording_type == RECORDING_VCR));

	//prepare record in spts mode
	CMenuOptionChooser* oj12 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORD_IN_SPTS_MODE, &g_settings.recording_in_spts_mode, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT,(g_settings.recording_type == RECORDING_SERVER || g_settings.recording_type == RECORDING_FILE) );

	//prepare record correcture
	int rec_pre,rec_post;
	g_Timerd->getRecordingSafety(rec_pre,rec_post);
	sprintf(g_settings.record_safety_time_before, "%02d", rec_pre/60);
	sprintf(g_settings.record_safety_time_after, "%02d", rec_post/60);
	CRecordingSafetyNotifier *RecordingSafetyNotifier = new CRecordingSafetyNotifier;

	//timersettings submenue
	CMenuWidget *timerRecordingSettings = new CMenuWidget(LOCALE_MAINSETTINGS_RECORDING, NEUTRINO_ICON_TIMER, width);
	CMenuForwarder* mf15 = new CMenuForwarder(LOCALE_TIMERSETTINGS_SEPARATOR ,true, NULL, timerRecordingSettings, NULL, CRCInput::RC_yellow, NEUTRINO_ICON_BUTTON_YELLOW);

	//prepare time before
	CStringInput * timerSettings_record_safety_time_before = new CStringInput(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, g_settings.record_safety_time_before, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE_HINT_2,"0123456789 ", RecordingSafetyNotifier);
	CMenuForwarder *mf5 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_BEFORE, true, g_settings.record_safety_time_before, timerSettings_record_safety_time_before );

	//prepare time after
	CStringInput * timerSettings_record_safety_time_after = new CStringInput(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, g_settings.record_safety_time_after, 2, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_1, LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER_HINT_2,"0123456789 ", RecordingSafetyNotifier);
	CMenuForwarder *mf6 = new CMenuForwarder(LOCALE_TIMERSETTINGS_RECORD_SAFETY_TIME_AFTER, true, g_settings.record_safety_time_after, timerSettings_record_safety_time_after );

	//prepare zap to before
	int zapto_pre;
	g_Timerd->getZaptoSafety(zapto_pre);
	sprintf(g_settings.zapto_safety_time_before, "%02d", zapto_pre/60);
	CZaptoSafetyNotifier *ZaptoSafetyNotifier = new CZaptoSafetyNotifier;
	CStringInput * timerSettings_zapto_safety_time_before = new CStringInput(LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE, g_settings.zapto_safety_time_before, 2, LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE_HINT_1, LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE_HINT_2,"0123456789 ", ZaptoSafetyNotifier);
	CMenuForwarder *mf14 = new CMenuForwarder(LOCALE_TIMERSETTINGS_ZAPTO_SAFETY_TIME_BEFORE, true, g_settings.zapto_safety_time_before, timerSettings_zapto_safety_time_before );

	// default recording audio pids
	CMenuWidget *apidRecordingSettings = new CMenuWidget(LOCALE_MAINSETTINGS_RECORDING, NEUTRINO_ICON_AUDIO, width);
	CMenuForwarder* mf13 = new CMenuForwarder(LOCALE_RECORDINGMENU_APIDS ,true, NULL, apidRecordingSettings, NULL, CRCInput::RC_blue, NEUTRINO_ICON_BUTTON_BLUE);

	g_settings.recording_audio_pids_std = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_STD ) ? 1 : 0 ;
	g_settings.recording_audio_pids_alt = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_ALT ) ? 1 : 0 ;
	g_settings.recording_audio_pids_ac3 = ( g_settings.recording_audio_pids_default & TIMERD_APIDS_AC3 ) ? 1 : 0 ;

	CRecAPIDSettingsNotifier * an = new CRecAPIDSettingsNotifier;
	//prepare audio pids default
	CMenuOptionChooser* aoj1 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_STD, &g_settings.recording_audio_pids_std, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);
	//prepare audio pids alternate
	CMenuOptionChooser* aoj2 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_ALT, &g_settings.recording_audio_pids_alt, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);
	//prepare audio pids ac3
	CMenuOptionChooser* aoj3 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_APIDS_AC3, &g_settings.recording_audio_pids_ac3, MESSAGEBOX_NO_YES_OPTIONS, MESSAGEBOX_NO_YES_OPTION_COUNT, true, an);
	//subhead
	apidRecordingSettings->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_RECORDINGMENU_APIDS));

	apidRecordingSettings->addItem(GenericMenuSeparator);
	apidRecordingSettings->addItem(GenericMenuBack);
	apidRecordingSettings->addItem(GenericMenuSeparatorLine);
	apidRecordingSettings->addItem(aoj1);
	apidRecordingSettings->addItem(aoj2);
	apidRecordingSettings->addItem(aoj3);

	// directory menue for direct recording settings
	CMenuWidget *dirMenu = new CMenuWidget(LOCALE_RECORDINGMENU_DEFDIR, NEUTRINO_ICON_RECORDING, width);
	dirMenu->addItem(GenericMenuSeparator);
	CDirChooser* fc1[MAX_RECORDING_DIR];
	CMenuForwarder* mffc[MAX_RECORDING_DIR];
	char temp[10];
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		fc1[i] = new CDirChooser(&g_settings.recording_dir[i]);
		snprintf(temp,10,"%d:",i);
		temp[9]=0;// terminate for sure
		mffc[i] = new CMenuForwarderNonLocalized(temp, true, g_settings.recording_dir[i],fc1[i]);
	}
	for(int i=0 ; i < MAX_RECORDING_DIR ; i++)
	{
		dirMenu->addItem(mffc[i]);
	}
	dirMenu->addItem(GenericMenuSeparator);

	// for direct recording
	CMenuWidget *directRecordingSettings = new CMenuWidget(LOCALE_RECORDINGMENU_FILESETTINGS, NEUTRINO_ICON_RECORDING, width);
	
	CMenuForwarder* mf7 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILESETTINGS,(g_settings.recording_type == RECORDING_FILE), NULL, directRecordingSettings, NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN);

	CStringInput * recordingSettings_splitsize = new CStringInput(LOCALE_RECORDINGMENU_SPLITSIZE, g_settings.recording_splitsize, 6, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2, "0123456789 ");
	CMenuForwarder* mf9 = new CMenuForwarder(LOCALE_RECORDINGMENU_SPLITSIZE, true, g_settings.recording_splitsize, recordingSettings_splitsize);

	CMenuOptionChooser* oj6 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_O_SYNC, &g_settings.recording_use_o_sync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj7 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_USE_FDATASYNC, &g_settings.recording_use_fdatasync, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj8 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_VTXT_PID, &g_settings.recording_stream_vtxt_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj9 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_STREAM_SUBTITLE_PID, &g_settings.recording_stream_subtitle_pid, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj13 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RINGBUFFERS, &g_settings.recording_ringbuffers, RECORDINGMENU_RINGBUFFER_SIZES, RECORDINGMENU_RINGBUFFER_SIZE_COUNT, true);
	CMenuOptionChooser* oj10 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_CHOOSE_DIRECT_REC_DIR, &g_settings.recording_choose_direct_rec_dir, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CMenuOptionChooser* oj11 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_EPG_FOR_FILENAME, &g_settings.recording_epg_for_filename, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true);

	CStringInput * recordingSettings_filenameTemplate = new CStringInput(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, &g_settings.recording_filename_template[0], 21, LOCALE_RECORDINGMENU_FILENAME_TEMPLATE_HINT, LOCALE_IPSETUP_HINT_2, "%/-_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ");
	CMenuForwarder* mf11 = new CMenuForwarder(LOCALE_RECORDINGMENU_FILENAME_TEMPLATE, true, g_settings.recording_filename_template[0],recordingSettings_filenameTemplate);

	CStringInput * recordingSettings_dirPermissions = new CStringInput(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, g_settings.recording_dir_permissions[0], 3, LOCALE_RECORDINGMENU_DIR_PERMISSIONS_HINT, LOCALE_IPSETUP_HINT_2, "01234567");
	CMenuForwarder* mf12 = new CMenuForwarder(LOCALE_RECORDINGMENU_DIR_PERMISSIONS, true, g_settings.recording_dir_permissions[0],recordingSettings_dirPermissions);

	CRecordingNotifier *RecordingNotifier = new CRecordingNotifier(mf1,mf2,oj2,mf3,oj3,oj4,oj5,mf7,oj12);

	//recording type
	CMenuOptionChooser* oj1 = new CMenuOptionChooser(LOCALE_RECORDINGMENU_RECORDING_TYPE, &g_settings.recording_type, RECORDINGMENU_RECORDING_TYPE_OPTIONS, RECORDINGMENU_RECORDING_TYPE_OPTION_COUNT, true, RecordingNotifier);

	//paint menue entries
	//intros
	recordingSettings->addItem(GenericMenuSeparator);
	recordingSettings->addItem(GenericMenuBack);
	recordingSettings->addItem(GenericMenuSeparatorLine);

	recordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_SETUPNOW, true, NULL, this, "recording", CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
	recordingSettings->addItem(new CMenuForwarder(LOCALE_SETTINGS_HELP, true, NULL, this, "help_recording", CRCInput::RC_help, NEUTRINO_ICON_BUTTON_HELP_SMALL));
	recordingSettings->addItem(GenericMenuSeparatorLine);
	recordingSettings->addItem( oj1); //recording type (off, server, vcr, direct) 
	recordingSettings->addItem(GenericMenuSeparatorLine);
	recordingSettings->addItem( mf7); //direct record settings
	recordingSettings->addItem( mf1); //server ip
	recordingSettings->addItem( mf2); //server port
	recordingSettings->addItem( oj2); //wol
	recordingSettings->addItem( mf3); //mac
	recordingSettings->addItem( oj3); //stop playback
	recordingSettings->addItem( oj4); //stop epg
	recordingSettings->addItem( oj4b);//switch on announcement
	recordingSettings->addItem( oj5); //suppress scart switch
	recordingSettings->addItem(oj12); //use spts
	recordingSettings->addItem(GenericMenuSeparatorLine);
	recordingSettings->addItem( mf15);//timersettings
		//subhead
		timerRecordingSettings->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_TIMERSETTINGS_SEPARATOR));
		timerRecordingSettings->addItem(GenericMenuSeparator);
		timerRecordingSettings->addItem(GenericMenuBack);
		timerRecordingSettings->addItem(GenericMenuSeparatorLine);
		timerRecordingSettings->addItem( mf5); //start record correcture
		timerRecordingSettings->addItem( mf6); //end record correcture
		timerRecordingSettings->addItem( mf14);//switch correcture

	recordingSettings->addItem( mf13);//audio pid settings
		directRecordingSettings->addItem(GenericMenuSeparator);
		directRecordingSettings->addItem(GenericMenuBack);
		directRecordingSettings->addItem(GenericMenuSeparatorLine);
		directRecordingSettings->addItem(new CMenuForwarder(LOCALE_RECORDINGMENU_DEFDIR, true, NULL, dirMenu, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED));
		directRecordingSettings->addItem(oj10);
		directRecordingSettings->addItem(mf12);
		directRecordingSettings->addItem(GenericMenuSeparatorLine);
		directRecordingSettings->addItem(oj13); //ringbuffer
		directRecordingSettings->addItem(mf9);
		directRecordingSettings->addItem(oj6);
		directRecordingSettings->addItem(oj7);
		directRecordingSettings->addItem(oj8);
		directRecordingSettings->addItem(oj9);
		directRecordingSettings->addItem(GenericMenuSeparatorLine);
		directRecordingSettings->addItem(oj11);
		directRecordingSettings->addItem(mf11);


	recordingSettings->exec(NULL, "");
	recordingSettings->hide();
	delete recordingSettings;

	CNeutrinoApp::getInstance()->recordingstatus = 0;
}


