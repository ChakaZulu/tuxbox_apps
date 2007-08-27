/*
$Id: personalize.h,v 1.1 2007/08/27 13:39:18 nitr8 Exp $

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

public:
CConfigFile                     configfile;
CPersonalizeGui();
void hide();
int exec(CMenuTarget* parent, const std::string & actionKey);
void ShowMainMenuOptions();
void ShowSettingsOptions();
void ShowServiceOptions();
void ShowPersonalizationMenu();
void SaveAndRestart();
};
#endif
