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

#include "stringinput.h"
#include "../global.h"

CStringInput::CStringInput(string Name, char* Value, int Size,  string Hint_1, string Hint_2, char* Valid_Chars, CChangeObserver* Observ)
{
	name = Name;
	value = Value;
	size = Size;

	hint_1 = Hint_1;
	hint_2 = Hint_2;
	validchars = Valid_Chars;

	observ = Observ;
	width = (Size*20)+40;


	if (width<420)
	{
		width=420;
	}

	int neededWidth = g_Fonts->menu_title->getRenderWidth( g_Locale->getText(name).c_str());
	if (neededWidth+20> width)
	{
		width= neededWidth+20;
    }

	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	iheight = g_Fonts->menu_info->getHeight();

	height = hheight+ mheight+ 50;
	if ( hint_1.length()> 0 )
		height+= iheight;
	if ( hint_2.length()> 0 )
		height+= iheight;

	x = ((720-width)>>1);
	y = ((500-height)>>1);
	selected = 0;
}

void CStringInput::key0_9Pressed(int key)
{
	value[selected]=validchars[key];
	paintChar(selected);

	if (selected < (size- 1))
		selected++;
	paintChar(selected-1);
	paintChar(selected);
}

void CStringInput::keyRedPressed()
{
	if ( strstr(validchars, " ")!=NULL )
	{
		value[selected]=' ';
		paintChar(selected);

		if (selected < (size- 1))
			selected++;
		paintChar(selected-1);
		paintChar(selected);
	}
}

void CStringInput::keyUpPressed()
{
	int npos = 0;
	for(int count=0;count<(int)strlen(validchars);count++)
		if(value[selected]==validchars[count])
			npos = count;
	npos++;
	if(npos>=(int)strlen(validchars))
		npos = 0;
	value[selected]=validchars[npos];
	paintChar(selected);
}

void CStringInput::keyDownPressed()
{
	int npos = 0;
	for(int count=0;count<(int)strlen(validchars);count++)
		if(value[selected]==validchars[count])
			npos = count;
	npos--;
	if(npos<0)
		npos = strlen(validchars)-1;
	value[selected]=validchars[npos];
	paintChar(selected);
}

void CStringInput::keyLeftPressed()
{
	if(selected>0)
	{
		selected--;
		paintChar(selected+1);
		paintChar(selected);
	}
}

void CStringInput::keyRightPressed()
{
	if(selected<(int)strlen(value)-1)
	{
		selected++;
		paintChar(selected-1);
		paintChar(selected);
	}
}


int CStringInput::exec( CMenuTarget* parent, string )
{
	char oldval[size];
	int key;

	strcpy(oldval, value);

	if (parent)
	{
		parent->hide();
	}

	for(int count=strlen(value)-1;count<size-1;count++)
		strcat(value, " ");

	paint();

	bool loop = true;

	//int selected = 0;
	while(loop)
	{
		g_lcdd->setMenuText(1, value, selected+1);
		key = g_RCInput->getKey(300);
		if (key==CRCInput::RC_left)
		{
			keyLeftPressed();
		}
		else if (key==CRCInput::RC_right)
		{
			keyRightPressed();
		}
		else if ( ( key>= 0 ) && ( key<= 9) )
		{
			key0_9Pressed( key);
		}
		else if (key==CRCInput::RC_red)
		{
			keyRedPressed();
		}
		else if ( (key==CRCInput::RC_green) && ( strstr(validchars, ".")!=NULL ) )
		{
			value[selected]='.';
			paintChar(selected);

			if (selected < (size- 1))
				selected++;
			paintChar(selected-1);
			paintChar(selected);
		}
		else if (key==CRCInput::RC_up)
		{
			keyUpPressed();
		}
		else if (key==CRCInput::RC_down)
		{
			keyDownPressed();
		}
		else if (key==CRCInput::RC_ok)
		{
			loop=false;
		}
		else if ( (key==CRCInput::RC_home) || (key==CRCInput::RC_timeout) )
		{
			strcpy(value, oldval);
			loop=false;
		}
	}

	hide();

	for(int count=size-1;count>=0;count--)
	{
		if((value[count]==' ') || (value[count]==0))
		{
			value[count]=0;
		}
		else
			break;
	}
	value[size]=0;

	if ( (observ) && (key==CRCInput::RC_ok) )
	{
		observ->changeNotify( name, value );
	}

	return CMenuTarget::RETURN_REPAINT;
}

void CStringInput::hide()
{
	g_FrameBuffer->paintBackgroundBoxRel(x, y, width, height);
}

void CStringInput::paint()
{
	g_FrameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+ 10, y+ hheight, width- 10, g_Locale->getText(name).c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, y+ hheight, width, height- hheight, COL_MENUCONTENT);

	if ( hint_1.length()> 0 )
	{
		g_Fonts->menu_info->RenderString(x+ 20, y+ hheight+ mheight+ iheight+ 40, width- 20, g_Locale->getText(hint_1).c_str(), COL_MENUCONTENT);
		if ( hint_2.length()> 0 )
			g_Fonts->menu_info->RenderString(x+ 20, y+ hheight+ mheight+ iheight* 2+ 40, width- 20, g_Locale->getText(hint_2).c_str(), COL_MENUCONTENT);
	}

	for (int count=0;count<size;count++)
		paintChar(count);

}

void CStringInput::paintChar(int pos)
{
	int xs = 20;
	int ys = mheight;
	int xpos = x+ 20+ pos* xs;
	int ypos = y+ hheight+ 25;

	string ch = "";
	if(pos<(int)strlen(value))
		ch = *(value+pos);

	int color = COL_MENUCONTENT;
	if (pos==selected)
		color = COL_MENUCONTENTSELECTED;

	g_FrameBuffer->paintBoxRel(xpos, ypos, xs, ys, COL_MENUCONTENT+ 4);
	g_FrameBuffer->paintBoxRel(xpos+ 1, ypos+ 1, xs- 2, ys- 2, color);

	int xfpos = xpos + ((xs- g_Fonts->menu->getRenderWidth(ch.c_str()))>>1);

	g_Fonts->menu->RenderString(xfpos,ypos+ys, width, ch.c_str(), color);
}

CStringInputSMS::CStringInputSMS(string Name, char* Value, int Size, string Hint_1 = "", string Hint_2 = "", char* Valid_Chars= "", CChangeObserver* Observ = NULL)
		: CStringInput(Name, Value, Size, Hint_1, Hint_2, Valid_Chars, Observ)
{
	Chars[1] = "1.,:!?";
	Chars[2] = "abc2ä";
	Chars[3] = "def3";
	Chars[4] = "ghi4";
	Chars[5] = "jkl5";
	Chars[6] = "mno6ö";
	Chars[7] = "pqrs7ß";
	Chars[8] = "tuv8ü";
	Chars[9] = "wxyz9";
	Chars[0] = "0 -/()<>=";

	for(int i=0; i<10; i++)
	{
		arraySizes[i] = strlen(Chars[i]);
	}
	height+=260;
	y = ((500-height)>>1);
}


void CStringInputSMS::key0_9Pressed(int key)
{
	if (lastKey != key)
	{
		keyCounter = 0;
	}
	keyCounter = keyCounter % strlen(Chars[key]);
	value[selected] = Chars[key][keyCounter];
	paintChar(selected);
	keyCounter++;
	lastKey = key;
}

void CStringInputSMS::keyRedPressed()
{
	if ((value[selected]>='a') && (value[selected]<='z'))
	{
		value[selected] -= 32;
	}
	else if ((value[selected]>='A') && (value[selected]<='Z'))
	{
		value[selected] += 32;
	}
	paintChar(selected);
}

void CStringInputSMS::keyUpPressed()
{}

void CStringInputSMS::keyDownPressed()
{}

void CStringInputSMS::keyLeftPressed()
{
	keyCounter=0;
	CStringInput::keyLeftPressed();
}

void CStringInputSMS::keyRightPressed()
{
	keyCounter=0;
	CStringInput::keyRightPressed();
}

void CStringInputSMS::paint()
{
	g_FrameBuffer->paintBoxRel(x, y, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x+ 10, y+ hheight, width, g_Locale->getText(name).c_str(), COL_MENUHEAD);
	g_FrameBuffer->paintBoxRel(x, y+ hheight, width, height- hheight, COL_MENUCONTENT);

	if ( hint_1.length()> 0 )
	{
		g_Fonts->menu_info->RenderString(x+ 20, y+ hheight+ mheight+ iheight+ 40, width- 20, g_Locale->getText(hint_1).c_str(), COL_MENUCONTENT);
		if ( hint_2.length()> 0 )
			g_Fonts->menu_info->RenderString(x+ 20, y+ hheight+ mheight+ iheight* 2+ 40, width- 20, g_Locale->getText(hint_2).c_str(), COL_MENUCONTENT);
	}
	g_FrameBuffer->paintIcon("numericpad.raw", x+20+140, y+ hheight+ mheight+ iheight* 3+ 30, COL_MENUCONTENT);

	for (int count=0;count<size;count++)
		paintChar(count);

	g_FrameBuffer->paintBoxRel(x,y+height-25, width,25, COL_MENUHEAD);
	g_FrameBuffer->paintHLine(x, x+width,  y+height-25, COL_INFOBAR_SHADOW);

	g_FrameBuffer->paintIcon("rot.raw", x+8, y+height-25+1);
	g_Fonts->infobar_small->RenderString(x+38, y+height-25+24 - 2, width, g_Locale->getText("stringinput.caps").c_str(), COL_INFOBAR);

}
