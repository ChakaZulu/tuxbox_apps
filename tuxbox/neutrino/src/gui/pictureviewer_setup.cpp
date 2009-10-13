/*
	$Id: pictureviewer_setup.cpp,v 1.1 2009/10/13 19:19:36 dbt Exp $

	pictureviewer setup implementation - Neutrino-GUI

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

	$Log: pictureviewer_setup.cpp,v $
	Revision 1.1  2009/10/13 19:19:36  dbt
	init pictureviewer_setup for it's own file
	
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include "gui/pictureviewer_setup.h"

#include <global.h>
#include <neutrino.h>

#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include "gui/pictureviewer.h"
#include "gui/filebrowser.h"

#include <driver/screen_max.h>

#include <system/debug.h>


CPictureViewerSetup::CPictureViewerSetup()
{
	frameBuffer = CFrameBuffer::getInstance();

	width = w_max (500, 100);
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;
}

CPictureViewerSetup::~CPictureViewerSetup()
{

}

int CPictureViewerSetup::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "init pctureviwer setup\n");
	int   res = menu_return::RETURN_REPAINT;

	if (parent)
	{
		parent->hide();
	}


	if(actionKey == "picturedir")
	{
		parent->hide();
		CFileBrowser b;
		b.Dir_Mode=true;
		if (b.exec(g_settings.picviewer_picturedir))
			strncpy(g_settings.picviewer_picturedir, b.getSelectedFile()->Name.c_str(), sizeof(g_settings.picviewer_picturedir)-1);
		return res;
	}

	showPictureViewerSetup();
	
	return res;
}

void CPictureViewerSetup::hide()
{
	frameBuffer->paintBackgroundBoxRel(x,y, width,height);
}

#define MESSAGEBOX_NO_YES_OPTION_COUNT 2
const CMenuOptionChooser::keyval MESSAGEBOX_NO_YES_OPTIONS[MESSAGEBOX_NO_YES_OPTION_COUNT] =
{
	{ 0, LOCALE_MESSAGEBOX_NO  },
	{ 1, LOCALE_MESSAGEBOX_YES }
};


#define PICTUREVIEWER_SCALING_OPTION_COUNT 3
const CMenuOptionChooser::keyval PICTUREVIEWER_SCALING_OPTIONS[PICTUREVIEWER_SCALING_OPTION_COUNT] =
{
	{ CPictureViewer::SIMPLE, LOCALE_PICTUREVIEWER_RESIZE_SIMPLE        },
	{ CPictureViewer::COLOR , LOCALE_PICTUREVIEWER_RESIZE_COLOR_AVERAGE },
	{ CPictureViewer::NONE  , LOCALE_PICTUREVIEWER_RESIZE_NONE          }
};


void CPictureViewerSetup::showPictureViewerSetup()
/*shows the setup menue*/
{

	CMenuWidget* picviewsetup = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_SETTINGS, width);
	picviewsetup->addItem( new CMenuSeparator(CMenuSeparator::ALIGN_LEFT | CMenuSeparator::SUB_HEAD | CMenuSeparator::STRING, LOCALE_PICTUREVIEWER_HEAD));

	// intros: back ande save
	picviewsetup->addItem(GenericMenuSeparator);
	picviewsetup->addItem(GenericMenuBack);
	picviewsetup->addItem(GenericMenuSeparatorLine);

	picviewsetup->addItem(new CMenuOptionChooser(LOCALE_PICTUREVIEWER_SCALING  , &g_settings.picviewer_scaling     , PICTUREVIEWER_SCALING_OPTIONS  , PICTUREVIEWER_SCALING_OPTION_COUNT  , true ));

	CStringInput * pic_timeout= new CStringInput(LOCALE_PICTUREVIEWER_SLIDE_TIME, g_settings.picviewer_slide_time, 2, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_SLIDE_TIME, true, g_settings.picviewer_slide_time, pic_timeout));
	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DEFDIR, true, g_settings.picviewer_picturedir, this, "picturedir"));

	CIPInput * picViewSettings_DecServerIP = new CIPInput( LOCALE_PICTUREVIEWER_DECODE_SERVER_IP, g_settings.picviewer_decode_server_ip, LOCALE_IPSETUP_HINT_1, LOCALE_IPSETUP_HINT_2);
	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DECODE_SERVER_IP, true, g_settings.picviewer_decode_server_ip, picViewSettings_DecServerIP));

	CStringInput * picViewSettings_DecServerPort= new CStringInput(LOCALE_PICTUREVIEWER_DECODE_SERVER_PORT, g_settings.picviewer_decode_server_port, 5, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "0123456789 ");

	picviewsetup->addItem(new CMenuForwarder(LOCALE_PICTUREVIEWER_DECODE_SERVER_PORT, true, g_settings.picviewer_decode_server_port, picViewSettings_DecServerPort));


	picviewsetup->exec (NULL, "");
	picviewsetup->hide ();
	delete picviewsetup;

}
