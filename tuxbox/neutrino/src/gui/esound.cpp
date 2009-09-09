/*
  $Id: esound.cpp,v 1.9 2009/09/09 19:05:35 rhabarber1848 Exp $
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

#ifdef ENABLE_LIRC
#include <irsend/irsend.h>
#endif

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

#define ESOUNDSERVER_START_SCRIPT CONFIGDIR "/esoundserver.start"
#define ESOUNDSERVER_END_SCRIPT CONFIGDIR "/esoundserver.end"

bool esound_active = false;

CEsoundGui::CEsoundGui()
{
	m_frameBuffer = CFrameBuffer::getInstance();
}

CEsoundGui::~CEsoundGui()
{
	if(!esound_active)
	{
		g_Zapit->setStandby (false);
		g_Sectionsd->setPauseScanning (false);
	}
}

//------------------------------------------------------------------------
int CEsoundGui::exec(CMenuTarget* parent, const std::string &)
{
	if(esound_active)
	{
		puts("[esound.cpp] esound already active..."); 
		return menu_return::RETURN_EXIT_ALL;
	}

	std::string tmp;
	std::string esound_start_script ("start_esound");
	std::string esound_start_path1 ("/etc/init.d/");
	std::string esound_start_path2 ("/var/etc/init.d/");
	std::string esound_start_command;

	// check for Esound start script in /etc
	tmp = (esound_start_path1+esound_start_script).c_str();
	if (access(tmp.c_str(), X_OK) == 0)
	{
		esound_start_script = tmp;
	}
	else
	{
		// check for Esound start script in /var/etc
		tmp = (esound_start_path2+esound_start_script).c_str();
		if (access(tmp.c_str(), X_OK) == 0)
		{
			esound_start_script = tmp;
		}
	}

	if (tmp == "")
	{
		printf("[esound.cpp] %s in %s or %s not found, returning...", esound_start_script.c_str(), esound_start_path1.c_str(), esound_start_path2.c_str());
		return menu_return::RETURN_EXIT_ALL;
	}
	else
	{
		esound_start_command = (esound_start_script+' '+g_settings.esound_port);
	}

	esound_active = true;
	if(parent)
	{
		parent->hide();
	}

	if(g_settings.video_Format != g_settings.video_backgroundFormat)
		g_Controld->setVideoFormat(g_settings.video_backgroundFormat);

	bool usedBackground = m_frameBuffer->getuseBackground();
	if (usedBackground)
		m_frameBuffer->saveBackgroundImage();
	m_frameBuffer->loadPal("radiomode.pal", 18, COL_MAXFREE);
	m_frameBuffer->useBackground(m_frameBuffer->loadBackground("radiomode.raw"));// set useBackground true or false
	m_frameBuffer->paintBackground();

#ifdef HAVE_DBOX_HARDWARE
	// If Audiomode is OST then save setting and switch to AVS-Mode
	if(g_settings.audio_avs_Control == CControld::TYPE_OST)
	{
		char tmpvol;
		tmpvol = g_Controld->getVolume(CControld::TYPE_OST);
		g_Controld->setVolume(100, CControld::TYPE_OST);
		m_vol_ost = true;
		g_settings.audio_avs_Control = CControld::TYPE_AVS;
		g_Controld->setVolume(tmpvol, CControld::TYPE_AVS);
	}
	else
#endif
		m_vol_ost = false;

	// set zapit in standby mode
	g_Zapit->setStandby(true);

	// tell neutrino we're in audio mode
	CNeutrinoApp::getInstance()->handleMsg( NeutrinoMessages::CHANGEMODE , NeutrinoMessages::mode_audio );
	// remember last mode
	if (CNeutrinoApp::getInstance()->zapto_on_init_done)
		m_LastMode=(CNeutrinoApp::getInstance()->getLastMode() | NeutrinoMessages::norezap);
	else
		m_LastMode=(CNeutrinoApp::getInstance()->getLastMode());

	// Stop sectionsd
	g_Sectionsd->setPauseScanning(true);

#ifdef ENABLE_LIRC
	//Send ir
	CIRSend irs("esoundon");
	irs.Send();
#endif

	puts("[esound.cpp] executing " ESOUNDSERVER_START_SCRIPT "."); 
	if (system(ESOUNDSERVER_START_SCRIPT) != 0) 
		perror("Datei " ESOUNDSERVER_START_SCRIPT " fehlt.Bitte erstellen, wenn gebraucht.\nFile " ESOUNDSERVER_START_SCRIPT " not found. Please create if needed.\n");

#ifdef HAVE_DBOX_HARDWARE
	// disable iec aka digi out
	g_Zapit->IecOff();
#endif

	printf("[esound.cpp] executing %s.\n", esound_start_command.c_str());
	if (system(esound_start_command.c_str()) == 0)
		show();

	esound_active = false;
	puts("[esound.cpp] stopping Esound...");
	if (system("killall esd") != 0) 
		perror("[esound.cpp] stopping Esound failed...");

	// wait for esound to end
	sleep(2);

	// Restore previous background
	if (usedBackground)
		m_frameBuffer->restoreBackgroundImage();
	m_frameBuffer->useBackground(usedBackground);
	m_frameBuffer->paintBackground();

	// Restore last mode
	g_Zapit->setStandby(false);
#ifdef HAVE_DBOX_HARDWARE
	if(m_vol_ost)
	{
		char tmpvol;
		tmpvol = g_Controld->getVolume(CControld::TYPE_AVS);
		g_Controld->setVolume(100, CControld::TYPE_AVS);
		g_settings.audio_avs_Control = CControld::TYPE_OST;
		g_Controld->setVolume(tmpvol, CControld::TYPE_OST);
	}
#endif

#ifdef ENABLE_LIRC
	//Send ir
	CIRSend irs2("esoundoff");
	irs2.Send();
#endif

	puts("[esound.cpp] executing " ESOUNDSERVER_END_SCRIPT "."); 
	if (system(ESOUNDSERVER_END_SCRIPT) != 0) 
		perror("Datei " ESOUNDSERVER_END_SCRIPT " fehlt. Bitte erstellen, wenn gebraucht.\nFile " ESOUNDSERVER_END_SCRIPT " not found. Please create if needed.\n");

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

	ShowHintUTF( LOCALE_MESSAGEBOX_INFO, g_Locale->getText(LOCALE_ESOUND_INFO) );
	while(loop)
	{
		if(CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_audio)
		{
			// stop if mode was changed in another thread
			loop = false;
		}

		g_RCInput->getMsg(&msg, &data, 1);

		if( msg == CRCInput::RC_timeout)
		{
			// nothing
		}
		else if( msg == CRCInput::RC_home || msg == NeutrinoMessages::ESOUND_OFF)
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
