/*
  $Id: esound.cpp,v 1.1 2009/03/29 20:47:14 rhabarber1848 Exp $
  Neutrino-GUI  -   DBoxII-Project

  based on
  AudioPlayer by Dirch,Zwen

  (C) 2002-2008 the tuxbox project contributors
  (C) 2008 Novell, Inc. Author: Stefan Seyfried

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gui/esound.h>

#include <global.h>
#include <neutrino.h>

#include <driver/encoding.h>
#include <driver/fontrenderer.h>
#include <driver/rcinput.h>

#include <daemonc/remotecontrol.h>

#include <gui/eventlist.h>
#include <gui/color.h>
#include <gui/infoviewer.h>

#include <gui/widget/buttons.h>
#include <gui/widget/icons.h>
#include <gui/widget/menue.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <system/settings.h>

#include <irsend/irsend.h>

#include <algorithm>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <sstream>

#if HAVE_DVB_API_VERSION >= 3
#include <linux/dvb/audio.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/video.h>
#define ADAP	"/dev/dvb/adapter0"
#define ADEC	ADAP "/audio0"
#define VDEC	ADAP "/video0"
#define DMX	ADAP "/demux0"
#define DVR	ADAP "/dvr0"
#endif

#define ESOUND_START_SCRIPT1 "/etc/init.d/start_esound"
#define ESOUND_START_SCRIPT2 "/var/etc/init.d/start_esound"
#define ESOUND_END_SCRIPT "killall esd"

CEsoundGui::CEsoundGui()
{
	m_frameBuffer = CFrameBuffer::getInstance();
}

CEsoundGui::~CEsoundGui()
{
	g_Zapit->setStandby (false);
	g_Sectionsd->setPauseScanning (false);
}

//------------------------------------------------------------------------
int CEsoundGui::exec(CMenuTarget* parent, const std::string &)
{
	if(parent)
	{
		parent->hide();
	}

	if(g_settings.video_Format != CControldClient::VIDEOFORMAT_4_3)
		g_Controld->setVideoFormat(CControldClient::VIDEOFORMAT_4_3);

	bool usedBackground = m_frameBuffer->getuseBackground();
	if (usedBackground)
		m_frameBuffer->saveBackgroundImage();
	m_frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	m_frameBuffer->useBackground(m_frameBuffer->loadBackground("radiomode.raw"));// set useBackground true or false
	m_frameBuffer->paintBackground();

	// set zapit in standby mode
	g_Zapit->setStandby(true);

	// If Audiomode is OST then save setting and switch to AVS-Mode
	if(g_settings.audio_avs_Control == CControld::TYPE_OST)
	{
		m_vol_ost = true;
		g_settings.audio_avs_Control = CControld::TYPE_AVS;
	}
	else
		m_vol_ost = false;

	// tell neutrino we're in audio mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_audio );
	// remember last mode
	m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true);

	//Send ir
	CIRSend irs("esoundon");
	irs.Send();

#ifdef HAVE_DBOX_HARDWARE
	// disable iec aka digi out
	g_Zapit->IecOff();
#endif

	if (access(ESOUND_START_SCRIPT1, X_OK) == 0)
	{
		puts("[esound.cpp] executing " ESOUND_START_SCRIPT1 "."); 
		if (system(ESOUND_START_SCRIPT1) != 0) 
			perror("Datei " ESOUND_START_SCRIPT1 " fehlt.Bitte erstellen, wenn gebraucht.\nFile " ESOUND_START_SCRIPT1 " not found. Please create if needed.\n");
		show();
	}
	else if (access(ESOUND_START_SCRIPT2, X_OK) == 0)
	{
		puts("[esound.cpp] executing " ESOUND_START_SCRIPT2 "."); 
		if (system(ESOUND_START_SCRIPT2) != 0) 
			perror("Datei " ESOUND_START_SCRIPT2 " fehlt.Bitte erstellen, wenn gebraucht.\nFile " ESOUND_START_SCRIPT2 " not found. Please create if needed.\n");
		show();
	}
	else
		puts("[esound.cpp] " ESOUND_START_SCRIPT1 " or " ESOUND_START_SCRIPT2 " not found, returning..." );

	puts("[esound.cpp] executing " ESOUND_END_SCRIPT "."); 
	if (system(ESOUND_END_SCRIPT) != 0) 
		perror("Datei " ESOUND_END_SCRIPT " fehlt. Bitte erstellen, wenn gebraucht.\nFile " ESOUND_END_SCRIPT " not found. Please create if needed.\n");

	// wait for esound to end
	sleep(2);

	// Restore previous background
	if (usedBackground)
		m_frameBuffer->restoreBackgroundImage();
	m_frameBuffer->useBackground(usedBackground);
	m_frameBuffer->paintBackground();

	// Restore last mode
	g_Zapit->setStandby(false);
	if(m_vol_ost)
	{
		g_Controld->setVolume(100, CControld::TYPE_AVS);
		g_settings.audio_avs_Control = CControld::TYPE_OST;
	}

	//Send ir
	CIRSend irs2("esoundoff");
	irs2.Send();

	// Start Sectionsd
	g_Sectionsd->setPauseScanning(false);

#ifdef HAVE_DBOX_HARDWARE
	// enable iec aka digi out
	g_Zapit->IecOn();
#endif

	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , m_LastMode );
	g_RCInput->postMsg( NeutrinoMessages::SHOW_INFOBAR, 0 );

	// always exit all
	return menu_return::RETURN_EXIT_ALL;
}

//------------------------------------------------------------------------

int CEsoundGui::show()
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	int res = -1;

	CLCD::getInstance()->setMode(CLCD::MODE_AUDIO);

	bool loop = true;
	bool update = true;
	bool clear_before_update = false;


	ShowHintUTF( LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_ESOUND_INFO) );
	while(loop)
	{
		if(CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_audio)
		{
			// stop if mode was changed in another thread
			loop = false;
		}

		if (update)
		{
			if(clear_before_update)
			{
				hide();
				clear_before_update = false;
			}
			update = false;
		}
		g_RCInput->getMsg(&msg, &data, 10); // 1 sec timeout to update play/stop state display

		if( msg == CRCInput::RC_timeout)
		{
			// nothing
		}
		else if( msg == CRCInput::RC_home)
		{ 
			loop=false;
		}
		else if(msg == NeutrinoMessages::CHANGEMODE)
		{
			if((data & NeutrinoMessages::mode_mask) != NeutrinoMessages::mode_audio)
			{
				loop = false;
				m_LastMode=data;
			}
		}
		else if(msg == NeutrinoMessages::RECORD_START ||
				msg == NeutrinoMessages::ZAPTO ||
				msg == NeutrinoMessages::STANDBY_ON ||
				msg == NeutrinoMessages::SHUTDOWN ||
				msg == NeutrinoMessages::SLEEPTIMER)
		{
			// Exit for Record/Zapto Timers
			loop = false;
			g_RCInput->postMsg(msg, data);
		}
		else if(msg == NeutrinoMessages::EVT_TIMER)
		{
			CNeutrinoApp::getInstance()->handleMsg( msg, data );
		}
		else
		{
			if( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
			{
				loop = false;
			}
		}
	}
	hide();

	return(res);
}
