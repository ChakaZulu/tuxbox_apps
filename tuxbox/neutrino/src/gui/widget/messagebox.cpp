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


#include <global.h>
#include <neutrino.h>

#include "messagebox.h"

//#define borderwidth 4


CMessageBox::CMessageBox(const std::string Caption, std::string Text, CMessageBoxNotifier* Notifier, const std::string Icon, const int Width, uint Default, uint ShowButtons, const bool utf8_encoded)
{
	theight = g_Fonts->menu_title->getHeight();
	fheight = g_Fonts->menu->getHeight();

	iconfile = Icon;
	caption  = Caption;
	utf8     = utf8_encoded;
	Text     = Text + "\n";
	text.clear();

	int pos;
	do
	{
		pos = Text.find_first_of("\n");
		if ( pos!=-1 )
		{
			text.insert( text.end(), Text.substr( 0, pos ) );
			Text= Text.substr( pos+ 1, uint(-1) );
		}
	} while ( ( pos != -1 ) );

	width  = (Width < 450) ? 450 : Width;
	height = (theight + 0) + fheight * (text.size() + 3);

	int nw= g_Fonts->menu_title->getRenderWidth(g_Locale->getText(caption).c_str(), utf8_encoded) + 20; // UTF-8
	if ( iconfile!="" )
		nw+= 30;
	if ( nw> width )
		width= nw;

	for (unsigned int i= 0; i< text.size(); i++)
	{
		int nw= g_Fonts->menu->getRenderWidth(text[i].c_str(), utf8_encoded) + 20; // UTF-8
		if ( nw> width )
			width= nw;
	}

	notifier = Notifier;
	switch (Default)
	{
		case mbrYes:
			selected = 0;
			break;
		case mbrNo:
			selected = 1;
			break;
		case mbrCancel:
			selected = 2;
			break;
		case mbrBack:
			selected = 2;
			break;
	}
	showbuttons= ShowButtons;
}

CMessageBox::~CMessageBox(void)
{
	if (window != NULL)
	{
		delete window;
		window = NULL;
	}
}

void CMessageBox::paintHead()
{

	window->paintBoxRel(0, 0, width, theight + 0, (CFBWindow::color_t)COL_MENUHEAD);
	if ( iconfile!= "" )
	{
		window->paintIcon(iconfile.c_str(), 8, 5);
		window->RenderString(g_Fonts->menu_title, 40, theight + 0, width - 40, g_Locale->getText(caption), (CFBWindow::color_t)COL_MENUHEAD, 0, utf8); // UTF-8
	}
	else
		window->RenderString(g_Fonts->menu_title, 10, theight + 0, width - 10, g_Locale->getText(caption), (CFBWindow::color_t)COL_MENUHEAD, 0, utf8); // UTF-8

	window->paintBoxRel(0, theight + 0, width, height - (theight + 0), (CFBWindow::color_t)COL_MENUCONTENT);
	for (unsigned int i = 0; i < text.size(); i++)
		window->RenderString(g_Fonts->menu, 10, (theight + 0) + (fheight >> 1) + fheight * (i + 1), width, text[i], (CFBWindow::color_t)COL_MENUCONTENT, 0, utf8); // UTF-8

}

void CMessageBox::paintButtons()
{
	//irgendwann alle vergleichen - aber cancel ist sicher der längste
	int MaxButtonTextWidth = g_Fonts->infobar_small->getRenderWidth(g_Locale->getText("messagebox.cancel").c_str());

	int ButtonWidth = 20 + 33 + MaxButtonTextWidth;

//	int ButtonSpacing = 40;
//	int startpos = (width - ((ButtonWidth*3)+(ButtonSpacing*2))) / 2;

	int startpos = 10;
	int ButtonSpacing = (width- 20- (ButtonWidth*3) ) / 2;

	int xpos = startpos;
	int color = COL_INFOBAR_SHADOW;

	if ( showbuttons & mbYes )
	{
		if(selected==0)
			color = COL_MENUCONTENTSELECTED;
		window->paintBoxRel(xpos, height - fheight - 20, ButtonWidth, fheight, (CFBWindow::color_t)color);
		window->paintIcon("rot.raw", xpos + 14, height - fheight - 15);
		window->RenderString(g_Fonts->infobar_small, xpos + 43, height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.yes"), (CFBWindow::color_t)color);
	}

	xpos = startpos+ButtonWidth+ButtonSpacing;

	if ( showbuttons & mbNo )
	{
		color = COL_INFOBAR_SHADOW;
		if(selected==1)
			color = COL_MENUCONTENTSELECTED;

		window->paintBoxRel(xpos, height-fheight-20, ButtonWidth, fheight, (CFBWindow::color_t)color);
		window->paintIcon("gruen.raw", xpos+14, height-fheight-15);
		window->RenderString(g_Fonts->infobar_small, xpos + 43, height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.no"), (CFBWindow::color_t)color);
    }

    xpos = startpos+ButtonWidth*2+ButtonSpacing*2;
    if ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) )
	{
		color = COL_INFOBAR_SHADOW;
		if(selected==2)
			color = COL_MENUCONTENTSELECTED;

		window->paintBoxRel(xpos, height-fheight-20, ButtonWidth, fheight, (CFBWindow::color_t)color);
		window->paintIcon("home.raw", xpos+10, height-fheight-19);
		window->RenderString(g_Fonts->infobar_small, xpos + 43, height-fheight+4, ButtonWidth- 53, g_Locale->getText( ( showbuttons & mbCancel ) ? "messagebox.cancel" : "messagebox.back" ), (CFBWindow::color_t)color);
	}
}

void CMessageBox::yes()
{
	result = mbrYes;
	if (notifier)
		notifier->onYes();
}

void CMessageBox::no()
{
	result = mbrNo;
	if (notifier)
		notifier->onNo();
}

void CMessageBox::cancel()
{
	if ( showbuttons & mbCancel )
		result = mbrCancel;
	else
		result = mbrBack;
}

int CMessageBox::exec(int timeout)
{
	int res = menu_return::RETURN_REPAINT;

	window = new CFBWindow((((g_settings.screen_EndX- g_settings.screen_StartX) - width ) >> 1) + g_settings.screen_StartX,
			       (((g_settings.screen_EndY- g_settings.screen_StartY) - height) >> 1) + g_settings.screen_StartY,
			       width,
			       height);

	if (window == NULL)
	{
		return res; /* out of memory */
	}

/*
	unsigned char pixbuf[(width+ 2* borderwidth) * (height+ 2* borderwidth)];
	frameBuffer->SaveScreen(x- borderwidth, y- borderwidth, width+ 2* borderwidth, height+ 2* borderwidth, pixbuf);

	// clear border
	frameBuffer->paintBackgroundBoxRel(x- borderwidth, y- borderwidth, width+ 2* borderwidth, borderwidth);
	frameBuffer->paintBackgroundBoxRel(x- borderwidth, y+ height, width+ 2* borderwidth, borderwidth);
	frameBuffer->paintBackgroundBoxRel(x- borderwidth, y, borderwidth, height);
	frameBuffer->paintBackgroundBoxRel(x+ width, y, borderwidth, height);
*/
	paintHead();
	paintButtons();

	if ( timeout == -1 )
		timeout = g_settings.timing_epg ;

	unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd( timeout );
	uint msg; uint data;

	bool loop=true;
	while (loop)
	{

		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if ( ( (msg==CRCInput::RC_timeout) ||
			   (msg == (uint) g_settings.key_channelList_cancel) ) &&
			 ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) ) )
		{
			cancel();
			loop=false;
		}
		else if ( (msg==CRCInput::RC_green) && ( showbuttons & mbNo ) )
		{
			no();
			loop=false;
		}
		else if ( (msg==CRCInput::RC_red) && ( showbuttons & mbYes ) )
		{
			yes();
			loop=false;
		}
		else if(msg==CRCInput::RC_right)
		{
			bool ok = false;
			while (!ok)
			{
				selected++;
				switch (selected)
				{
					case 3:
						selected= -1;
					    break;
					case 0:
						ok = ( showbuttons & mbYes );
						break;
					case 1:
						ok = ( showbuttons & mbNo );
						break;
					case 2:
						ok = ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) );
						break;
				}
			}

			paintButtons();
		}
		else if(msg==CRCInput::RC_left)
		{
			bool ok = false;
			while (!ok)
			{
				selected--;
				switch (selected)
				{
					case -1:
						selected= 3;
					    break;
					case 0:
						ok = ( showbuttons & mbYes );
						break;
					case 1:
						ok = ( showbuttons & mbNo );
						break;
					case 2:
						ok = ( ( showbuttons & mbCancel ) || ( showbuttons & mbBack ) );
						break;
				}
			}

			paintButtons();

		}
		else if(msg==CRCInput::RC_ok)
		{
			//exec selected;
			switch (selected)
			{
				case 0: yes();
					break;
				case 1: no();
					break;
				case 2: cancel();
					break;
			}
			loop=false;
		}
		else if ( CNeutrinoApp::getInstance()->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			loop = false;
			res = menu_return::RETURN_EXIT_ALL;
		}

	}
/*
	frameBuffer->RestoreScreen(x- borderwidth, y- borderwidth, width+ 2* borderwidth, height+ 2* borderwidth, pixbuf);
*/
	if (window != NULL)
	{
		delete window;
		window = NULL;
	}
	
	return res;
}

int ShowMsg(const std::string Caption, std::string Text, uint Default, uint ShowButtons, const std::string Icon, int Width, int timeout, const bool utf8_encoded)
{
   	CMessageBox* messageBox = new CMessageBox(Caption, Text, NULL, Icon, Width, Default, ShowButtons, utf8_encoded);
	messageBox->exec(timeout);
	int res = messageBox->result;
	delete messageBox;
	
	return res;
}
