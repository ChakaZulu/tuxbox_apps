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

#include "stringinput_ext.h"
#include "../global.h"

CExtendedInput::CExtendedInput(string Name, char* Value, string Hint_1, string Hint_2, CChangeObserver* Observ, bool Localizing)
{
	name = Name;
	value = Value;

	hint_1 = Hint_1;
	hint_2 = Hint_2;

	observ = Observ;

	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	iheight = g_Fonts->menu_info->getHeight();

	localizing = Localizing;
	if(localizing)
	{
		width = g_Fonts->menu_title->getRenderWidth( g_Locale->getText(name).c_str())+20;
	}
	else
	{
		width = g_Fonts->menu_title->getRenderWidth( name.c_str())+20;
	}
	height = hheight+ mheight+ 20;

	if ( hint_1.length()> 0 )
		height+= iheight;
	if ( hint_2.length()> 0 )
		height+= iheight;

	x = ((720-width)>>1);
	y = ((500-height)>>1);
}

void CExtendedInput::addInputField( CExtendedInput_Item* fld )
{
	inputFields.insert(inputFields.end(), fld);
}


void CExtendedInput::calculateDialog()
{
	int ix = 0;
	int iy = 0;
	int maxX = 0;
	int maxY = 0;
	selectedChar = -1;
	for(int i=0; i<inputFields.size();i++)
	{
		inputFields[i]->init( ix, iy);
		inputFields[i]->setDataPointer( &value[i] );
		if ((selectedChar==-1) && (inputFields[i]->isSelectable()))
		{
			selectedChar = i;
		}
		maxX = ix>maxX?ix:maxX;
		maxY = iy>maxY?iy:maxY;
	}
	
	width = width>maxX+40?width:maxX+40;
	height = height>maxY+hheight+ mheight?height:maxY+hheight+ mheight;

	hintPosY = y + height -10;

	if ( hint_1.length()> 0 )
		height+= iheight;
	if ( hint_2.length()> 0 )
		height+= iheight;

	x = ((720-width)>>1);
	y = ((500-height)>>1);
}


int CExtendedInput::exec( CMenuTarget* parent, string )
{
	int res = menu_return::RETURN_REPAINT;
	char oldval[inputFields.size()+10];

	if (parent)
	{
		parent->hide();
	}

	paint();

	bool loop = true;
	uint msg; uint data;
	
	while(loop)
	{
		g_lcdd->setMenuText(1, value, selectedChar);

		g_RCInput->getMsg( &msg, &data, 300 );

		if (msg==CRCInput::RC_left)
		{
			if(selectedChar>0)
			{
				bool found = false;
				int oldSelectedChar = selectedChar;
				for(int i=selectedChar-1; i>=0;i--)
				{
					if ((!found) && (inputFields[i]->isSelectable()))
					{
						found = true;
						selectedChar = i;
					}
				}
				if(found)
				{
					inputFields[oldSelectedChar]->paint( x+20, y+hheight +20, false );
					inputFields[selectedChar]->paint( x+20, y+hheight +20, true );
				}
			}
		}
		else if (msg==CRCInput::RC_right)
		{
			if(selectedChar<inputFields.size()-1)
			{
				bool found = false;
				int oldSelectedChar = selectedChar;
				for(int i=selectedChar+1; i<inputFields.size();i++)
				{
					if ((!found) && (inputFields[i]->isSelectable()))
					{
						found = true;
						selectedChar = i;
					}
				}
				if(found)
				{
					inputFields[oldSelectedChar]->paint( x+20, y+hheight +20, false );
					inputFields[selectedChar]->paint( x+20, y+hheight +20, true );
				}
			}
		}
		else if ( ((msg>= 0) && (msg<=9)) || (msg == CRCInput::RC_red) || (msg == CRCInput::RC_green) || (msg == CRCInput::RC_blue) || (msg == CRCInput::RC_yellow)
					|| (msg == CRCInput::RC_up) || (msg == CRCInput::RC_down))
		{
			inputFields[selectedChar]->keyPressed(msg);
			inputFields[selectedChar]->paint( x+20, y+hheight +20, true );
		}
		else if (msg==CRCInput::RC_ok)
		{
			loop=false;
		}
		else if ( (msg==CRCInput::RC_home) || (msg==CRCInput::RC_timeout) )
		{/*
			if ( ( strcmp(value, oldval) != 0) &&
			     ( ShowMsg(name, g_Locale->getText("messagebox.discard"), CMessageBox::mbrYes, CMessageBox::mbYes | CMessageBox::mbCancel, 380 ) == CMessageBox::mbrCancel ) )
				continue;

			strcpy(value, oldval);*/
			loop=false;
		}
		else if ( neutrino->handleMsg( msg, data ) & messages_return::cancel_all )
		{
			loop = false;
			res = menu_return::RETURN_EXIT_ALL;
		}
	}

	hide();


	if ( (observ) && (msg==CRCInput::RC_ok) )
	{
		observ->changeNotify( name, value );
	}

	return res;
}

void CExtendedInput::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void CExtendedInput::paint()
{
	g_FrameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD);
	if(localizing)
	{
		g_Fonts->menu_title->RenderString(x+ 10, y+ hheight, width- 10, g_Locale->getText(name).c_str(), COL_MENUHEAD);
	}
	else
	{
		g_Fonts->menu_title->RenderString(x+ 10, y+ hheight, width- 10, name.c_str(), COL_MENUHEAD);
	}
	g_FrameBuffer->paintBoxRel(x, y+ hheight, width, height-hheight, COL_MENUCONTENT);

	if ( hint_1.length()> 0 )
	{
		if(localizing)
		{
			g_Fonts->menu_info->RenderString(x+ 20, hintPosY, width- 20, g_Locale->getText(hint_1).c_str(), COL_MENUCONTENT);
		}
		else
		{
			g_Fonts->menu_info->RenderString(x+ 20, hintPosY, width- 20, hint_1.c_str(), COL_MENUCONTENT);
		}
		if ( hint_2.length()> 0 )
		{
			if(localizing)
			{
				g_Fonts->menu_info->RenderString(x+ 20, hintPosY + iheight, width- 20, g_Locale->getText(hint_2).c_str(), COL_MENUCONTENT);
			}
			else
			{
				g_Fonts->menu_info->RenderString(x+ 20, hintPosY + iheight, width- 20, hint_2.c_str(), COL_MENUCONTENT);
			}

		}
	}

	for(int i=0; i<inputFields.size();i++)
	{
		inputFields[i]->paint( x+20, y+hheight +20, (i==selectedChar) );
	}

	
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


CExtendedInput_Item_Char::CExtendedInput_Item_Char(string Chars, bool Selectable )
{
	idx = 20;
	idy = g_Fonts->menu->getHeight();
	allowedChars = Chars;
	selectable = Selectable;
}

void CExtendedInput_Item_Char::init(int &x, int &y)
{
	ix = x;
	iy = y;
	x += idx;
}

void CExtendedInput_Item_Char::setAllowedChars( string ac )
{
	allowedChars = ac;
}

void CExtendedInput_Item_Char::paint(int x, int y, bool focusGained )
{
	int startx = ix + x;
	int starty = iy + y;

	int color = COL_MENUCONTENT;
	if (focusGained)
		color = COL_MENUCONTENTSELECTED;

	g_FrameBuffer->paintBoxRel( startx, starty, idx, idy, COL_MENUCONTENT+4);
	g_FrameBuffer->paintBoxRel( startx+1, starty+1, idx-2, idy-2, color);

	char text[2];
	text[0] = *data;
	text[1] = 0;
	int xfpos = startx + 1 + ((idx- g_Fonts->menu->getRenderWidth( text ))>>1);

	g_Fonts->menu->RenderString(xfpos,starty+idy, idx, text, color);
}

bool CExtendedInput_Item_Char::isAllowedChar( char ch )
{
	return (allowedChars.find(ch)!=-1);
}

int CExtendedInput_Item_Char::getCharID( char ch )
{
	return allowedChars.find(ch);
}

void CExtendedInput_Item_Char::keyPressed( int key )
{
	if(isAllowedChar( (char) '0' + key))
	{
		*data = (char) '0' + key;
		g_RCInput->postMsg( CRCInput::RC_right, 0 );
		return;
	}
	else
	{
		int pos = getCharID( *data );
		if (key==CRCInput::RC_up)
		{
			if(pos<allowedChars.size()-1)
			{
				*data = allowedChars[pos+1];
			}
			else
			{
				*data = allowedChars[0];
			}
		}
		else if (key==CRCInput::RC_down)
		{
			if(pos>0)
			{
				*data = allowedChars[pos-1];
			}
			else
			{
				*data = allowedChars[allowedChars.size()-1];
			}
		}
	}
}

//-----------------------------#################################-------------------------------------------------------

CIPInput::CIPInput(string Name, char* Value, string Hint_1 = "", string Hint_2 = "", CChangeObserver* Observ = NULL)
	: CExtendedInput(Name, Value, Hint_1, Hint_2, Observ)
{

	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Spacer(20) );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_Char("0123456789") );
	addInputField( new CExtendedInput_Item_newLiner(30) );
	Value[3] = '.';
	Value[7] = '.';
	Value[11] = '.';
	Value[15] = 0;
	calculateDialog();
}
