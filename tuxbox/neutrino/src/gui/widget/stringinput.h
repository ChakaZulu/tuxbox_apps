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

#ifndef __stringinput__
#define __stringinput__

#include <stdio.h>
#include "driver/framebuffer.h"
#include "driver/fontrenderer.h"
#include "driver/rcinput.h"

#include "menue.h"
#include "color.h"

#include <string>

using namespace std;

class CStringInput : public CMenuTarget
{
	protected:
		int x;
		int y;
		int width;
		int height;
		int hheight, mheight, iheight; // head/menu font height

		string	name;
		string  hint_1, hint_2;
		string	iconfile;
		char*   validchars;
		char*	value;
		int		size;
		int		selected;
		CChangeObserver*   observ;

		virtual void paint();
		virtual void paintChar(int pos);

		virtual void key0_9Pressed(int key);
		virtual void keyRedPressed();
		virtual void keyUpPressed();
		virtual void keyDownPressed();
		virtual void keyLeftPressed();
		virtual void keyRightPressed();

		virtual int handleOthers( uint msg, uint data );

	public:

		CStringInput(string Name, char* Value, int Size, string Hint_1 = "", string Hint_2 = "", char* Valid_Chars= "0123456789. ", CChangeObserver* Observ = NULL, string Icon="" );

		void hide();
		int exec( CMenuTarget* parent, string actionKey );

};

class CStringInputSMS : public CStringInput
{
		bool	capsMode;
		int 	arraySizes[10];
		char*	Chars[10];

		int keyCounter;
		int lastKey;

		virtual void key0_9Pressed(int key);
		virtual void keyRedPressed();
		virtual void keyUpPressed();
		virtual void keyDownPressed();
		virtual void keyLeftPressed();
		virtual void keyRightPressed();

		virtual void paint();

	public:
		CStringInputSMS(string Name, char* Value, int Size, string Hint_1 = "", string Hint_2 = "", char* Valid_Chars= "", CChangeObserver* Observ = NULL, string Icon="");
};

class CPINInput : public CStringInput
{
	protected:
		virtual void paintChar(int pos);
	public:
		CPINInput(string Name, char* Value, int Size, string Hint_1 = "", string Hint_2 = "", char* Valid_Chars= "0123456789", CChangeObserver* Observ = NULL)
		 : CStringInput(Name, Value, Size, Hint_1, Hint_2, Valid_Chars, Observ, "lock.raw") {};

		 int exec( CMenuTarget* parent, string actionKey );
};

class CPLPINInput : public CPINInput
{
	protected:
		int	fsk;

		virtual int handleOthers( uint msg, uint data );
	public:
		CPLPINInput(string Name, char* Value, int Size, string Hint_1, int FSK )
		 : CPINInput(Name, Value, Size, " ", Hint_1) { fsk= FSK; };

		int exec( CMenuTarget* parent, string actionKey );
};

class CPINChangeWidget : public CStringInput
{
	public:
		CPINChangeWidget(string Name, char* Value, int Size, string Hint_1 = "", string Hint_2 = "", char* Valid_Chars= "0123456789", CChangeObserver* Observ = NULL)
		: CStringInput(Name, Value, Size, Hint_1, Hint_2, Valid_Chars, Observ){};
};

#endif


