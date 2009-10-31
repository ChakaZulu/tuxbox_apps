/*
	$Id: video_setup.cpp,v 1.4 2009/10/31 11:15:59 seife Exp $

	video setup implementation - Neutrino-GUI

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

	$Log: video_setup.cpp,v $
	Revision 1.4  2009/10/31 11:15:59  seife
	neutrino: add support for TD to video_setup, improve non-dbox case
	
	The dbox is the only box that can set RGB sync position, so do not show
	that setting on other boxes
	
	Revision 1.3  2009/10/27 21:35:08  dbt
	removed unnecessary include
	
	Revision 1.2  2009/10/27 21:29:31  dbt
	added missing correct parameter to enable/disable option chooser for rgbsync and video output
	
	Revision 1.1  2009/10/27 20:28:42  dbt
	init video setup for it's own modul
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "video_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>


#include "screensetup.h"

#include <driver/screen_max.h>

#include <system/debug.h>



CVideoSetup::CVideoSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	SyncControlerForwarder = NULL;
	VcrVideoOutSignalOptionChooser = NULL;

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

	video_out_signal = g_Controld->getVideoOutput();
	vcr_video_out_signal = g_Controld->getVCROutput();
}

CVideoSetup::~CVideoSetup()
{
	delete SyncControlerForwarder;
	delete VcrVideoOutSignalOptionChooser;
}

int CVideoSetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init video setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	showVideoSetup();

	
	return res;
}

void CVideoSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


#ifdef HAVE_DBOX_HARDWARE
#define VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT 5
#elif defined HAVE_TRIPLEDRAGON
#define VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT 3
#else
#define VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT 4
#endif
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOSIGNAL_OPTIONS[VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT] =
{
	{ 0, LOCALE_VIDEOMENU_VIDEOSIGNAL_COMPOSITE },
	{ 1, LOCALE_VIDEOMENU_VIDEOSIGNAL_RGB       },
#ifndef HAVE_TRIPLEDRAGON
	/* the tripledragon has a dedicated s-video output, so needs no option for it... */
	{ 2, LOCALE_VIDEOMENU_VIDEOSIGNAL_SVIDEO    },
#endif
	{ 3, LOCALE_VIDEOMENU_VIDEOSIGNAL_YUV_V     }
#ifdef HAVE_DBOX_HARDWARE
	, { 4, LOCALE_VIDEOMENU_VIDEOSIGNAL_YUV_C   }
#endif
};

#define VIDEOMENU_VCRSIGNAL_OPTION_COUNT 2
const CMenuOptionChooser::keyval VIDEOMENU_VCRSIGNAL_OPTIONS[VIDEOMENU_VCRSIGNAL_OPTION_COUNT] =
{
	{ 2, LOCALE_VIDEOMENU_VCRSIGNAL_SVIDEO    },
	{ 0, LOCALE_VIDEOMENU_VCRSIGNAL_COMPOSITE }
};

#define VIDEOMENU_VIDEOFORMAT_OPTION_COUNT 4
const CMenuOptionChooser::keyval VIDEOMENU_VIDEOFORMAT_OPTIONS[VIDEOMENU_VIDEOFORMAT_OPTION_COUNT] =
{
	{ CControldClient::VIDEOFORMAT_4_3, LOCALE_VIDEOMENU_VIDEOFORMAT_43         },
	{ CControldClient::VIDEOFORMAT_4_3_PS, LOCALE_VIDEOMENU_VIDEOFORMAT_431        },
	{ CControldClient::VIDEOFORMAT_16_9, LOCALE_VIDEOMENU_VIDEOFORMAT_169        },
	{ CControldClient::VIDEOFORMAT_AUTO, LOCALE_VIDEOMENU_VIDEOFORMAT_AUTODETECT }
};

void CVideoSetup::showVideoSetup()
{
	//init
	CMenuWidget * videosetup = new CMenuWidget(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS, width);
	//subhead
	videosetup->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_VIDEOMENU_HEAD));

	//video signal type
	CMenuOptionChooser * scart = new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEOSIGNAL, &video_out_signal, VIDEOMENU_VIDEOSIGNAL_OPTIONS, VIDEOMENU_VIDEOSIGNAL_OPTION_COUNT, true, this);

	videosetup->addItem(GenericMenuSeparator);
	videosetup->addItem(GenericMenuBack);
	videosetup->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_VIDEOMENU_TV_SCART));

	videosetup->addItem(scart); 	//video signal type
	
	//video format
	CMenuOptionChooser * oj1 = new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEOFORMAT, &g_settings.video_Format, VIDEOMENU_VIDEOFORMAT_OPTIONS, VIDEOMENU_VIDEOFORMAT_OPTION_COUNT, true, this);
	//video format background
	CMenuOptionChooser * oj2 = new CMenuOptionChooser(LOCALE_VIDEOMENU_VIDEOFORMAT_BG, &g_settings.video_backgroundFormat, VIDEOMENU_VIDEOFORMAT_OPTIONS, VIDEOMENU_VIDEOFORMAT_OPTION_COUNT-1, true, this);

#ifdef HAVE_DBOX_HARDWARE
	//rgb centering
	CRGBCSyncControler * RGBCSyncControler = new CRGBCSyncControler(LOCALE_VIDEOMENU_RGB_CENTERING, &g_settings.video_csync);
	bool sc_active = ((video_out_signal == CControldClient::VIDEOOUTPUT_RGB) || (video_out_signal == CControldClient::VIDEOOUTPUT_YUV_VBS) || (video_out_signal ==  CControldClient::VIDEOOUTPUT_YUV_CVBS));
	SyncControlerForwarder = new CMenuForwarder(LOCALE_VIDEOMENU_RGB_CENTERING, sc_active, NULL , RGBCSyncControler, NULL, CRCInput::RC_red, NEUTRINO_ICON_BUTTON_RED);
#endif

	if (g_settings.video_Format == CControldClient::VIDEOFORMAT_AUTO)
	{
		this->changeNotify(LOCALE_VIDEOMENU_VIDEOFORMAT, NULL);
	}
	
	videosetup->addItem(oj1);	//video format
	videosetup->addItem(oj2);	//video format background
#ifdef HAVE_DBOX_HARDWARE
	/* only the dbox can change the RGB centering */
	videosetup->addItem(SyncControlerForwarder);	//rgb centering
#endif
	videosetup->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_VIDEOMENU_VCR_SCART));

	// Switching VCR Output presently does not work on the Philips and on non-dboxes
	if (g_info.box_Type == CControld::TUXBOX_MAKER_SAGEM || g_info.box_Type == CControld::TUXBOX_MAKER_NOKIA)
	{
		bool vo_active = ((video_out_signal == CControldClient::VIDEOOUTPUT_COMPOSITE) || (video_out_signal == CControldClient::VIDEOOUTPUT_SVIDEO));
		VcrVideoOutSignalOptionChooser = new CMenuOptionChooser(LOCALE_VIDEOMENU_VCRSIGNAL, &vcr_video_out_signal, VIDEOMENU_VCRSIGNAL_OPTIONS, VIDEOMENU_VCRSIGNAL_OPTION_COUNT, vo_active, this);
		videosetup->addItem(VcrVideoOutSignalOptionChooser);
	}
	else
		VcrVideoOutSignalOptionChooser = NULL;

	//video vcr switch
	videosetup->addItem(new CMenuOptionChooser(LOCALE_VIDEOMENU_VCRSWITCH, &g_settings.vcr_AutoSwitch, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true)); //video vcr switch

	// TODO move osd setup to it's own menue with all the other typical osd settings
	videosetup->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_VIDEOMENU_OSD));
	videosetup->addItem(new CMenuForwarder(LOCALE_VIDEOMENU_SCREENSETUP, true, NULL, new CScreenSetup(), NULL, CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN)); //osd setup

	videosetup->exec(NULL, "");
	videosetup->hide();
	delete videosetup;
}

bool CVideoSetup::changeNotify(const neutrino_locale_t OptionName, void *)
{
 	CNeutrinoApp * neutrino = CNeutrinoApp::getInstance();


 	if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEOSIGNAL))
 	{
		while ((vcr_video_out_signal) == CControldClient::VIDEOOUTPUT_SVIDEO && (video_out_signal != CControldClient::VIDEOOUTPUT_SVIDEO) && (video_out_signal != CControldClient::VIDEOOUTPUT_COMPOSITE) )
			video_out_signal = (video_out_signal + 1) % 5;
		g_Controld->setVideoOutput(video_out_signal);
		if (VcrVideoOutSignalOptionChooser)
			VcrVideoOutSignalOptionChooser->setActive((video_out_signal == CControldClient::VIDEOOUTPUT_COMPOSITE) || (video_out_signal == CControldClient::VIDEOOUTPUT_SVIDEO));
#ifdef HAVE_DBOX_HARDWARE
		SyncControlerForwarder->setActive((video_out_signal == CControldClient::VIDEOOUTPUT_RGB) || (video_out_signal == CControldClient::VIDEOOUTPUT_YUV_VBS) || (video_out_signal == CControldClient::VIDEOOUTPUT_YUV_CVBS));
#endif
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VCRSIGNAL))
	{
		g_Controld->setVCROutput(vcr_video_out_signal);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEOFORMAT) && (neutrino->getMode() != NeutrinoMessages::mode_radio))
	{
		g_Controld->setVideoFormat(g_settings.video_Format);
	}
	else if (ARE_LOCALES_EQUAL(OptionName, LOCALE_VIDEOMENU_VIDEOFORMAT_BG) && (neutrino->getMode() == NeutrinoMessages::mode_radio))
	{
		g_Controld->setVideoFormat(g_settings.video_backgroundFormat);
	}

	return true;
}


