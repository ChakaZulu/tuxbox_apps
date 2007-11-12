/*
	Neutrino-GUI  -   DBoxII-Project

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

	$Id: themes.cpp,v 1.2 2007/11/12 08:54:43 ecosys Exp $ 

*/

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <global.h>
#include <neutrino.h>
#include "widget/menue.h"
#include <system/setting_helpers.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>

#include <sys/stat.h>
#include <sys/time.h>

#include "themes.h"

/* undef THEMESDIR cause was defined in config.h without ending / */
#ifdef THEMESDIR
#undef THEMESDIR
#endif
#define THEMESDIR "/share/tuxbox/neutrino/themes/"
#define USERDIR "/var" THEMESDIR
#define FILE_PREFIX ".theme"

CThemes::CThemes()
: themefile('\t')
{
	frameBuffer = CFrameBuffer::getInstance();
	width = 500;
	hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();
	height = hheight+13*mheight+ 10;
	x=(((g_settings.screen_EndX- g_settings.screen_StartX)-width) / 2) + g_settings.screen_StartX;
	y=(((g_settings.screen_EndY- g_settings.screen_StartY)-height) / 2) + g_settings.screen_StartY;

	themeFilter.addFilter ("theme");

}

int CThemes::exec(CMenuTarget* parent, const std::string & actionKey)
{
	int res = menu_return::RETURN_REPAINT;

	if(actionKey=="usertheme")
	{
		Path = USERDIR;
		fileChooser();
		return res;
	}
	else if(actionKey=="standardtheme")
	{
		Path = THEMESDIR;
		fileChooser();
		return res;
	}

	if (parent)
		parent->hide();

	Show();
	return res;
}

void CThemes::fileChooser()
{
	CFileBrowser filebrowser("");

	filebrowser.Multi_Select    = false;
	filebrowser.Dirs_Selectable = false;
	filebrowser.Filter          = &themeFilter;

	hide();

	if (filebrowser.exec(Path.c_str()))
	{
		CFile *files = filebrowser.getSelectedFile();
		tf.Filename = files->getFileName();
		tf.Path     = files->getPath();
		char tmp[256] = "\0";
		strcat(tmp, tf.Path.c_str());
		strcat(tmp, tf.Filename.c_str());
		readFile(tmp);
	}
	else
		printf("[neutrino theme] no file selected\n");
}

void CThemes::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void CThemes::Show()
{
	file_name = "";

	CMenuWidget* themes = new CMenuWidget(LOCALE_COLORTHEMEMENU_HEAD2, "settings.raw", 500);
	themes->addItem(GenericMenuSeparator);
	themes->addItem(GenericMenuBack);
	themes->addItem(GenericMenuSeparatorLine);

	CMenuForwarder *m1 = new CMenuForwarder(LOCALE_COLORTHEMEMENU_SELECT1, true, NULL, this, "usertheme");
	CMenuForwarder *m2 = new CMenuForwarder(LOCALE_COLORTHEMEMENU_SELECT2, true, NULL, this, "standardtheme");

	CStringInputSMS *nameInput = new CStringInputSMS(LOCALE_COLORTHEMEMENU_NAME, &file_name, 30, NONEXISTANT_LOCALE, NONEXISTANT_LOCALE, "abcdefghijklmnopqrstuvwxyz0123456789- ");
	CMenuForwarder *m3 = new CMenuForwarder(LOCALE_COLORTHEMEMENU_SAVE, true , NULL, nameInput);

	themes->addItem(m2);
	// Don't show User-Theme if Userdir does'nt exist
	if ( access(USERDIR, 0) == 0 ) {
		themes->addItem(m1);
		themes->addItem(GenericMenuSeparatorLine);
		themes->addItem(m3);
	}

	themes->exec(NULL, "");
	themes->hide();
	delete themes;

	if (strlen(file_name.c_str()) > 1) {
		std::string userfile = USERDIR + file_name + FILE_PREFIX;
		saveFile((char*)userfile.c_str());
	}
}

void CThemes::readFile(char* themename)
{
	notifier = new CColorSetupNotifier;

	if(themefile.loadConfig(themename))
	{
		g_settings.menu_Head_alpha = themefile.getInt32( "menu_Head_alpha", 0x00 );
		g_settings.menu_Head_red = themefile.getInt32( "menu_Head_red", 0x00 );
		g_settings.menu_Head_green = themefile.getInt32( "menu_Head_green", 0x0A );
		g_settings.menu_Head_blue = themefile.getInt32( "menu_Head_blue", 0x19 );
		g_settings.menu_Head_Text_alpha = themefile.getInt32( "menu_Head_Text_alpha", 0x00 );
		g_settings.menu_Head_Text_red = themefile.getInt32( "menu_Head_Text_red", 0x5f );
		g_settings.menu_Head_Text_green = themefile.getInt32( "menu_Head_Text_green", 0x46 );
		g_settings.menu_Head_Text_blue = themefile.getInt32( "menu_Head_Text_blue", 0x00 );
		g_settings.menu_Content_alpha = themefile.getInt32( "menu_Content_alpha", 0x14 );
		g_settings.menu_Content_red = themefile.getInt32( "menu_Content_red", 0x00 );
		g_settings.menu_Content_green = themefile.getInt32( "menu_Content_green", 0x0f );
		g_settings.menu_Content_blue = themefile.getInt32( "menu_Content_blue", 0x23 );
		g_settings.menu_Content_Text_alpha = themefile.getInt32( "menu_Content_Text_alpha", 0x00 );
		g_settings.menu_Content_Text_red = themefile.getInt32( "menu_Content_Text_red", 0x64 );
		g_settings.menu_Content_Text_green = themefile.getInt32( "menu_Content_Text_green", 0x64 );
		g_settings.menu_Content_Text_blue = themefile.getInt32( "menu_Content_Text_blue", 0x64 );
		g_settings.menu_Content_Selected_alpha = themefile.getInt32( "menu_Content_Selected_alpha", 0x14 );
		g_settings.menu_Content_Selected_red = themefile.getInt32( "menu_Content_Selected_red", 0x19 );
		g_settings.menu_Content_Selected_green = themefile.getInt32( "menu_Content_Selected_green", 0x37 );
		g_settings.menu_Content_Selected_blue = themefile.getInt32( "menu_Content_Selected_blue", 0x64 );
		g_settings.menu_Content_Selected_Text_alpha = themefile.getInt32( "menu_Content_Selected_Text_alpha", 0x00 );
		g_settings.menu_Content_Selected_Text_red = themefile.getInt32( "menu_Content_Selected_Text_red", 0x00 );
		g_settings.menu_Content_Selected_Text_green = themefile.getInt32( "menu_Content_Selected_Text_green", 0x00 );
		g_settings.menu_Content_Selected_Text_blue = themefile.getInt32( "menu_Content_Selected_Text_blue", 0x00 );
		g_settings.menu_Content_inactive_alpha = themefile.getInt32( "menu_Content_inactive_alpha", 0x14 );
		g_settings.menu_Content_inactive_red = themefile.getInt32( "menu_Content_inactive_red", 0x00 );
		g_settings.menu_Content_inactive_green = themefile.getInt32( "menu_Content_inactive_green", 0x0f );
		g_settings.menu_Content_inactive_blue = themefile.getInt32( "menu_Content_inactive_blue", 0x23 );
		g_settings.menu_Content_inactive_Text_alpha = themefile.getInt32( "menu_Content_inactive_Text_alpha", 0x00 );
		g_settings.menu_Content_inactive_Text_red = themefile.getInt32( "menu_Content_inactive_Text_red", 55 );
		g_settings.menu_Content_inactive_Text_green = themefile.getInt32( "menu_Content_inactive_Text_green", 70 );
		g_settings.menu_Content_inactive_Text_blue = themefile.getInt32( "menu_Content_inactive_Text_blue", 85 );
		g_settings.infobar_alpha = themefile.getInt32( "infobar_alpha", 0x14 );
		g_settings.infobar_red = themefile.getInt32( "infobar_red", 0x00 );
		g_settings.infobar_green = themefile.getInt32( "infobar_green", 0x0e );
		g_settings.infobar_blue = themefile.getInt32( "infobar_blue", 0x23 );
		g_settings.infobar_Text_alpha = themefile.getInt32( "infobar_Text_alpha", 0x00 );
		g_settings.infobar_Text_red = themefile.getInt32( "infobar_Text_red", 0x64 );
		g_settings.infobar_Text_green = themefile.getInt32( "infobar_Text_green", 0x64 );
		g_settings.infobar_Text_blue = themefile.getInt32( "infobar_Text_blue", 0x64 );

		notifier->changeNotify(NONEXISTANT_LOCALE, NULL);
	}
	else
		printf("[neutrino theme} %s not found\n", themename);

}

void CThemes::saveFile(char * themename)
{
	themefile.setInt32( "menu_Head_alpha", g_settings.menu_Head_alpha );
	themefile.setInt32( "menu_Head_red", g_settings.menu_Head_red );
	themefile.setInt32( "menu_Head_green", g_settings.menu_Head_green );
	themefile.setInt32( "menu_Head_blue", g_settings.menu_Head_blue );
	themefile.setInt32( "menu_Head_Text_alpha", g_settings.menu_Head_Text_alpha );
	themefile.setInt32( "menu_Head_Text_red", g_settings.menu_Head_Text_red );
	themefile.setInt32( "menu_Head_Text_green", g_settings.menu_Head_Text_green );
	themefile.setInt32( "menu_Head_Text_blue", g_settings.menu_Head_Text_blue );
	themefile.setInt32( "menu_Content_alpha", g_settings.menu_Content_alpha );
	themefile.setInt32( "menu_Content_red", g_settings.menu_Content_red );
	themefile.setInt32( "menu_Content_green", g_settings.menu_Content_green );
	themefile.setInt32( "menu_Content_blue", g_settings.menu_Content_blue );
	themefile.setInt32( "menu_Content_Text_alpha", g_settings.menu_Content_Text_alpha );
	themefile.setInt32( "menu_Content_Text_red", g_settings.menu_Content_Text_red );
	themefile.setInt32( "menu_Content_Text_green", g_settings.menu_Content_Text_green );
	themefile.setInt32( "menu_Content_Text_blue", g_settings.menu_Content_Text_blue );
	themefile.setInt32( "menu_Content_Selected_alpha", g_settings.menu_Content_Selected_alpha );
	themefile.setInt32( "menu_Content_Selected_red", g_settings.menu_Content_Selected_red );
	themefile.setInt32( "menu_Content_Selected_green", g_settings.menu_Content_Selected_green );
	themefile.setInt32( "menu_Content_Selected_blue", g_settings.menu_Content_Selected_blue );
	themefile.setInt32( "menu_Content_Selected_Text_alpha", g_settings.menu_Content_Selected_Text_alpha );
	themefile.setInt32( "menu_Content_Selected_Text_red", g_settings.menu_Content_Selected_Text_red );
	themefile.setInt32( "menu_Content_Selected_Text_green", g_settings.menu_Content_Selected_Text_green );
	themefile.setInt32( "menu_Content_Selected_Text_blue", g_settings.menu_Content_Selected_Text_blue );
	themefile.setInt32( "menu_Content_inactive_alpha", g_settings.menu_Content_inactive_alpha );
	themefile.setInt32( "menu_Content_inactive_red", g_settings.menu_Content_inactive_red );
	themefile.setInt32( "menu_Content_inactive_green", g_settings.menu_Content_inactive_green );
	themefile.setInt32( "menu_Content_inactive_blue", g_settings.menu_Content_inactive_blue );
	themefile.setInt32( "menu_Content_inactive_Text_alpha", g_settings.menu_Content_inactive_Text_alpha );
	themefile.setInt32( "menu_Content_inactive_Text_red", g_settings.menu_Content_inactive_Text_red );
	themefile.setInt32( "menu_Content_inactive_Text_green", g_settings.menu_Content_inactive_Text_green );
	themefile.setInt32( "menu_Content_inactive_Text_blue", g_settings.menu_Content_inactive_Text_blue );
	themefile.setInt32( "infobar_alpha", g_settings.infobar_alpha );
	themefile.setInt32( "infobar_red", g_settings.infobar_red );
	themefile.setInt32( "infobar_green", g_settings.infobar_green );
	themefile.setInt32( "infobar_blue", g_settings.infobar_blue );
	themefile.setInt32( "infobar_Text_alpha", g_settings.infobar_Text_alpha );
	themefile.setInt32( "infobar_Text_red", g_settings.infobar_Text_red );
	themefile.setInt32( "infobar_Text_green", g_settings.infobar_Text_green );
	themefile.setInt32( "infobar_Text_blue", g_settings.infobar_Text_blue );

	if (!themefile.saveConfig(themename))
		printf("[neutrino theme} %s write error\n", themename);

}
