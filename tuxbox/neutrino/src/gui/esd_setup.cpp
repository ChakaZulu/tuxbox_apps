/*
	$Id: esd_setup.cpp,v 1.3 2009/11/29 21:57:05 dbt Exp $

	esound setup implementation - Neutrino-GUI

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

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/esd_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include "gui/widget/stringinput.h"

#include "gui/filebrowser.h"

#include <driver/screen_max.h>

#include <system/debug.h>



CEsdSetup::CEsdSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CEsdSetup::~CEsdSetup()
{

}

int CEsdSetup::exec(CMenuTarget* parent, const std::string &/*actionKey*/)
{
	dprintf(DEBUG_DEBUG, "init esd setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}

	showEsdSetup();
	
	return res;
}

void CEsdSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}


void CEsdSetup::showEsdSetup()
/*shows the esd setup menue*/
{

	CMenuWidget* esdSetup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_SETTINGS, width);
	esdSetup->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_ESOUND_NAME));

	// intros
	esdSetup->addItem(GenericMenuSeparator);
	esdSetup->addItem(GenericMenuBack);
	esdSetup->addItem(GenericMenuSeparatorLine);

	// entry
	CStringInput * setup_EsoundPort= new CStringInput(LOCALE_ESOUND_PORT, g_settings.esound_port, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");
	esdSetup->addItem(new CMenuForwarder(LOCALE_ESOUND_PORT, true, g_settings.esound_port, setup_EsoundPort));

	esdSetup->exec (NULL, "");
	esdSetup->hide ();
	delete esdSetup;

}
