/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
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

#include <gui/widget/keychooser.h>

#include <global.h>
#include <neutrino.h>

#include <gui/color.h>



CKeyChooser::CKeyChooser( int* Key, const char * const title, const std::string & Icon )
		: CMenuWidget(title, Icon)
{
	frameBuffer = CFrameBuffer::getInstance();
	key = Key;
	keyChooser = new CKeyChooserItem("keychooser.head", key);
	keyDeleter = new CKeyChooserItemNoKey(key);

	addItem( new CMenuSeparator(CMenuSeparator::STRING, " ") );
	addItem(GenericMenuSeparatorLine);
	addItem(GenericMenuBack);
	addItem(GenericMenuSeparatorLine);
	addItem(new CMenuForwarder("keychoosermenu.setnew", true, NULL, keyChooser) );
	addItem(new CMenuForwarder("keychoosermenu.setnone", true, NULL, keyDeleter) );
}


CKeyChooser::~CKeyChooser()
{
	delete keyChooser;
	delete keyDeleter;
}


void CKeyChooser::paint()
{
	std::string * text = &(((CMenuSeparator *)(items[0]))->text);
	*text = g_Locale->getText("keychoosermenu.currentkey");
	(*text) += ": ";
	(*text) += CRCInput::getKeyName(*key);

	CMenuWidget::paint();
}

//*****************************
CKeyChooserItem::CKeyChooserItem(const char * const Name, int *Key)
{
	name = Name;
	key = Key;
	x = y = width = height = 0;
}


int CKeyChooserItem::exec(CMenuTarget* parent, const std::string &)
{
	neutrino_msg_t      msg;
	neutrino_msg_data_t data;

	unsigned long long timeoutEnd;

	int res = menu_return::RETURN_REPAINT;

	if (parent)
		parent->hide();

	paint();

	g_RCInput->clearRCMsg();

	timeoutEnd = CRCInput::calcTimeoutEnd(g_settings.timing_menu);

 get_Message:
	g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );
	
	if (msg != CRCInput::RC_timeout)
	{
		if ((msg >= 0) && (msg <= CRCInput::RC_MaxRC))
			*key = msg;
		else if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
			res = menu_return::RETURN_EXIT_ALL;
		else
			goto get_Message;
	}

	hide();
	return res;
}

void CKeyChooserItem::hide()
{
	CFrameBuffer::getInstance()->paintBackgroundBoxRel(x, y, width, height);
}

void CKeyChooserItem::paint()
{
	int hheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->getHeight();
	int mheight = g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->getHeight();

	width       = 350;
	height      = hheight + 2 * mheight;
	x           = ((720-width) >> 1) -20;
	y           = (576-height) >> 1;

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	frameBuffer->paintBoxRel(x, y          , width, hheight         , COL_MENUHEAD_PLUS_0   );
	frameBuffer->paintBoxRel(x, y + hheight, width, height - hheight, COL_MENUCONTENT_PLUS_0);

	g_Font[SNeutrinoSettings::FONT_TYPE_MENU_TITLE]->RenderString(x+ 10, y+ hheight, width, g_Locale->getText(name), COL_MENUHEAD, 0, true); // UTF-8

	//paint msg...
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, y+ hheight+ mheight, width, g_Locale->getText("keychooser.text1"), COL_MENUCONTENT, 0, true); // UTF-8
	g_Font[SNeutrinoSettings::FONT_TYPE_MENU]->RenderString(x+ 10, y+ hheight+ mheight* 2, width, g_Locale->getText("keychooser.text2"), COL_MENUCONTENT, 0, true); // UTF-8
}
