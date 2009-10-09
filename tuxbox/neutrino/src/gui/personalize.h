/*
$Id: personalize.h,v 1.5 2009/10/09 04:58:18 dbt Exp $

Customization Menu - Neutrino-GUI

Copyright (C) 2007 Speed2206
and some other guys

Kommentar:

This is the customization menu, as originally showcased in
Oxygen. It is a more advanced version of the 'user levels'
patch currently available.


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
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/


#ifndef __personalize__
#define __personalize__
#include <string>
#include <vector>
#include <configfile.h>
#include <driver/framebuffer.h>
#include <system/lastchannel.h>
#include <system/setting_helpers.h>

using namespace std;

class CPersonalizeGui : public CMenuTarget
{
private:
CFrameBuffer *frameBuffer;
int x, y, width, height, hheight, mheight;
void ShowHelpPersonalize();

public:

enum PERSONALIZE_MODE
{
	PERSONALIZE_MODE_NOTVISIBLE =  0,
	PERSONALIZE_MODE_VISIBLE  =  1,
	PERSONALIZE_MODE_PIN  = 2
};

enum PERSONALIZE_PROTECT_MODE
{
	PROTECT_MODE_NOT_PROTECTED =  0,
	PROTECT_MODE_PIN_PROTECTED  =  1
};

enum PERSONALIZE_ACTIVE_MODE
{
	PERSONALIZE_MODE_DISABLED =  0,
	PERSONALIZE_MODE_ENABLED  =  1
};

CConfigFile                     configfile;
CPersonalizeGui();
void hide();
int exec(CMenuTarget* parent, const std::string & actionKey);
void ShowMainMenuOptions();
void ShowSettingsOptions();
void ShowServiceOptions();
void ShowPersonalizationMenu();
void SaveAndRestart();

int addItem(	CMenuWidget &item,
		const neutrino_locale_t Text,
		bool isActiv = PERSONALIZE_MODE_ENABLED,
		const char * const Option = NULL,
		CMenuTarget* Target = NULL,
		const char * const ActionKey = NULL,
		neutrino_msg_t DirectKey = NULL,
		const char * const IconName = NULL,
		const bool defaultselected = false,
		const int & personalize_mode = PERSONALIZE_MODE_VISIBLE,
		const int & personalize_protect_mode = PROTECT_MODE_NOT_PROTECTED, 
		const bool alwaysAsk = true);

neutrino_msg_t	setShortcut(const int & shortcut_num, neutrino_msg_t alternate_rc_key = CRCInput::RC_nokey);

};
#endif
