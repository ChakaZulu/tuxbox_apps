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


#include <gui/widget/messagebox.h>

#include <gui/widget/icons.h>

#include <global.h>
#include <neutrino.h>


CMessageBox::CMessageBox(const char * const Caption, const char * const Text, const int Width, const char * const Icon, const uint Default, const uint ShowButtons, CMessageBoxNotifier * const Notifier) : CHintBox(Caption, Text, Width, Icon)
{
	height += (fheight << 1);


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
		case mbrBack:
			selected = 2;
			break;
	}
	showbuttons= ShowButtons;
}

void CMessageBox::paintButtons()
{
	int color;
	//irgendwann alle vergleichen - aber cancel ist sicher der längste
	int MaxButtonTextWidth = g_Fonts->infobar_small->getRenderWidth(g_Locale->getText("messagebox.cancel"), true); // UTF-8

	int ButtonWidth = 20 + 33 + MaxButtonTextWidth;

//	int ButtonSpacing = 40;
//	int startpos = (width - ((ButtonWidth*3)+(ButtonSpacing*2))) / 2;

	int ButtonSpacing = (width- 20- (ButtonWidth*3) ) / 2;

	int xpos = 10;

	if (showbuttons & mbYes)
	{
		color = (selected == 0) ? COL_MENUCONTENTSELECTED : COL_INFOBAR_SHADOW;
		window->paintBoxRel(xpos, height - fheight - 20, ButtonWidth, fheight, (CFBWindow::color_t)color);
		window->paintIcon(NEUTRINO_ICON_BUTTON_RED, xpos + 14, height - fheight - 15);
		window->RenderString(g_Fonts->infobar_small, xpos + 43, height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.yes"), (CFBWindow::color_t)color, 0, true); // UTF-8
	}

	xpos += ButtonWidth + ButtonSpacing;

	if (showbuttons & mbNo)
	{
		color = (selected == 1) ? COL_MENUCONTENTSELECTED : COL_INFOBAR_SHADOW;

		window->paintBoxRel(xpos, height-fheight-20, ButtonWidth, fheight, (CFBWindow::color_t)color);
		window->paintIcon(NEUTRINO_ICON_BUTTON_GREEN, xpos+14, height-fheight-15);
		window->RenderString(g_Fonts->infobar_small, xpos + 43, height-fheight+4, ButtonWidth- 53, g_Locale->getText("messagebox.no"), (CFBWindow::color_t)color, 0, true); // UTF-8
	}

	xpos += ButtonWidth + ButtonSpacing;

	if (showbuttons & (mbCancel | mbBack))
	{
		color = (selected == 2) ? COL_MENUCONTENTSELECTED : COL_INFOBAR_SHADOW;

		window->paintBoxRel(xpos, height-fheight-20, ButtonWidth, fheight, (CFBWindow::color_t)color);
		window->paintIcon(NEUTRINO_ICON_BUTTON_HOME, xpos+10, height-fheight-19);
		window->RenderString(g_Fonts->infobar_small, xpos + 43, height-fheight+4, ButtonWidth- 53, g_Locale->getText( ( showbuttons & mbCancel ) ? "messagebox.cancel" : "messagebox.back" ), (CFBWindow::color_t)color, 0, true); // UTF-8
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

	CHintBox::paint();

	if (window == NULL)
	{
		return res; /* out of memory */
	}

	paintButtons();

	if ( timeout == -1 )
		timeout = g_settings.timing_epg ;

	unsigned long long timeoutEnd = CRCInput::calcTimeoutEnd( timeout );
	uint msg; uint data;

	bool loop=true;
	while (loop)
	{

		g_RCInput->getMsgAbsoluteTimeout( &msg, &data, &timeoutEnd );

		if (((msg == CRCInput::RC_timeout) || (msg  == (uint)g_settings.key_channelList_cancel)) &&
		    (showbuttons & (mbCancel | mbBack)))
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
						ok = (showbuttons & (mbCancel | mbBack));
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
						ok = (showbuttons & (mbCancel | mbBack));
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

	hide();
	
	return res;
}

int ShowMsgUTF(const char * const Caption, const char * const Text, const uint Default, const uint ShowButtons, const char * const Icon, const int Width, const int timeout)
{
   	CMessageBox* messageBox = new CMessageBox(Caption, Text, Width, Icon, Default, ShowButtons);
	messageBox->exec(timeout);
	int res = messageBox->result;
	delete messageBox;
	
	return res;
}

int ShowMsgUTF(const char * const Caption, const std::string & Text, const uint Default, const uint ShowButtons, const char * const Icon, const int Width, const int timeout)
{
	return ShowMsgUTF(Caption, Text.c_str(), Default, ShowButtons, Icon, Width, timeout);
}

void DisplayErrorMessage(const char * const ErrorMsg)
{
	ShowMsgUTF("messagebox.error", ErrorMsg, CMessageBox::mbrCancel, CMessageBox::mbCancel, "error.raw");
}
