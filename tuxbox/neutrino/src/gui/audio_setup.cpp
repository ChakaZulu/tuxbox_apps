/*
	$Id: audio_setup.cpp,v 1.2 2009/10/27 21:37:29 dbt Exp $

	audio setup implementation - Neutrino-GUI

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

	$Log: audio_setup.cpp,v $
	Revision 1.2  2009/10/27 21:37:29  dbt
	removed forgotten comment
	
	Revision 1.1  2009/10/17 11:38:37  dbt
	init audio_setup for it's own modules
	
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/audio_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>

#include <driver/screen_max.h>

#include <system/debug.h>



CAudioSetup::CAudioSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CAudioSetup::~CAudioSetup()
{

}

int CAudioSetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init audio setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	showAudioSetup();
	
	return res;
}

void CAudioSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


#define AUDIOMENU_ANALOGOUT_OPTION_COUNT 3
const CMenuOptionChooser::keyval AUDIOMENU_ANALOGOUT_OPTIONS[AUDIOMENU_ANALOGOUT_OPTION_COUNT] =
{
	{ 0, LOCALE_AUDIOMENU_STEREO    },
	{ 1, LOCALE_AUDIOMENU_MONOLEFT  },
	{ 2, LOCALE_AUDIOMENU_MONORIGHT }
};

#ifdef HAVE_DBOX_HARDWARE
#ifdef ENABLE_LIRC
#define AUDIOMENU_AVS_CONTROL_OPTION_COUNT 3
#else
#define AUDIOMENU_AVS_CONTROL_OPTION_COUNT 2
#endif
const CMenuOptionChooser::keyval AUDIOMENU_AVS_CONTROL_OPTIONS[AUDIOMENU_AVS_CONTROL_OPTION_COUNT] =
{
	{ CControld::TYPE_OST , LOCALE_AUDIOMENU_OST  },
	{ CControld::TYPE_AVS , LOCALE_AUDIOMENU_AVS  },
#ifdef ENABLE_LIRC
	{ CControld::TYPE_LIRC, LOCALE_AUDIOMENU_LIRC }
#endif
};
#endif

#define AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_LEFT_RIGHT_SELECTABEL_OPTIONS[AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT] =
{
      { true, LOCALE_OPTIONS_ON },
      { false, LOCALE_OPTIONS_OFF }
};

#define AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT 2
const CMenuOptionChooser::keyval AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_OPTIONS[AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT] =
{
        { true, LOCALE_OPTIONS_ON },
        { false, LOCALE_OPTIONS_OFF }
};

/* audio settings menu */
void CAudioSetup::showAudioSetup()
{
	CAudioSetupNotifier *audioSetupNotifier = new CAudioSetupNotifier;

	//menue init
	CMenuWidget* audioSettings = new CMenuWidget(LOCALE_MAINSETTINGS_HEAD, NEUTRINO_ICON_SETTINGS, width);

	//subhead
	audioSettings->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_MAINSETTINGS_AUDIO));

	// intros
	audioSettings->addItem(GenericMenuSeparator);
	audioSettings->addItem(GenericMenuBack);
	audioSettings->addItem(GenericMenuSeparatorLine);

	CMenuOptionChooser* oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_ANALOGOUT, &g_settings.audio_AnalogMode, AUDIOMENU_ANALOGOUT_OPTIONS, AUDIOMENU_ANALOGOUT_OPTION_COUNT, true, audioSetupNotifier);

	audioSettings->addItem( oj );
	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AUDIO_LEFT_RIGHT_SELECTABLE, &g_settings.audio_left_right_selectable, AUDIOMENU_LEFT_RIGHT_SELECTABEL_OPTIONS, AUDIOMENU_LEFT_RIGHT_SELECTABLE_OPTION_COUNT, true, audioSetupNotifier);
	audioSettings->addItem( oj );

	audioSettings->addItem(GenericMenuSeparatorLine);

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE, &g_settings.audiochannel_up_down_enable, AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_OPTIONS, AUDIOMENU_AUDIOCHANNEL_UP_DOWN_ENABLE_COUNT, true, audioSetupNotifier);
	audioSettings->addItem( oj );

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_DOLBYDIGITAL, &g_settings.audio_DolbyDigital, OPTIONS_OFF0_ON1_OPTIONS, OPTIONS_OFF0_ON1_OPTION_COUNT, true, audioSetupNotifier);
	audioSettings->addItem(oj);
	
#ifdef HAVE_DBOX_HARDWARE
	audioSettings->addItem(GenericMenuSeparatorLine);

	CStringInput * audio_PCMOffset = new CStringInput(LOCALE_AUDIOMENU_PCMOFFSET, g_settings.audio_PCMOffset, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ", audioSetupNotifier);
	CMenuForwarder *mf = new CMenuForwarder(LOCALE_AUDIOMENU_PCMOFFSET, (g_settings.audio_avs_Control == CControld::TYPE_LIRC), g_settings.audio_PCMOffset, audio_PCMOffset );
	CAudioSetupNotifier2 *audioSetupNotifier2 = new CAudioSetupNotifier2(mf);

	oj = new CMenuOptionChooser(LOCALE_AUDIOMENU_AVS_CONTROL, &g_settings.audio_avs_Control, AUDIOMENU_AVS_CONTROL_OPTIONS, AUDIOMENU_AVS_CONTROL_OPTION_COUNT, true, audioSetupNotifier2);
	audioSettings->addItem(oj);
	audioSettings->addItem(mf);
#endif
	
	// volume bar steps
	CStringInput * audio_step = new CStringInput(LOCALE_AUDIOMENU_VOLUMEBAR_AUDIOSTEPS,g_settings.audio_step, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ", audioSetupNotifier);
	CMenuForwarder *as = new CMenuForwarder(LOCALE_AUDIOMENU_VOLUMEBAR_AUDIOSTEPS, true, g_settings.audio_step, audio_step );
	audioSettings->addItem(as);


	audioSettings->exec(NULL, "");
	audioSettings->hide();
	delete audioSettings;
}
