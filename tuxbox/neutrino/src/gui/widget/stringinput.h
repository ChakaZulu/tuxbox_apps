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

#include "menue.h"

#include <driver/framebuffer.h>

#include <string>

class CStringInput : public CMenuTarget
{
	protected:
		CFrameBuffer	*frameBuffer;
		int x;
		int y;
		int width;
		int height;
		int hheight; // head font height
		int mheight; // menu font height
		int iheight;

		std::string name;
		std::string hint_1, hint_2;
		std::string iconfile;
		const char * validchars;
		char *       value;
		int          size;
		int          selected;
		CChangeObserver * observ;

		virtual void paint();
		virtual void paintChar(int pos, char c);
		virtual void paintChar(int pos);

		virtual void NormalKeyPressed(const neutrino_msg_t key);
		virtual void keyBackspacePressed(void);
		virtual void keyRedPressed();
		virtual void keyYellowPressed();
		virtual void keyUpPressed();
		virtual void keyDownPressed();
		virtual void keyLeftPressed();
		virtual void keyRightPressed();

		virtual int handleOthers(const neutrino_msg_t msg, const neutrino_msg_data_t data);

	public:

		// Name, Hint_1, Hint_2: UTF-8 encoded
		CStringInput(const char * const Name, char* Value, int Size, const char * const Hint_1 = NULL, const char * const Hint_2 = NULL, const char * const Valid_Chars= "0123456789. ", CChangeObserver* Observ = NULL, const char * const Icon = NULL);

		void hide();
		int exec( CMenuTarget* parent, const std::string & actionKey );

};

class CStringInputSMS : public CStringInput
{
		bool	capsMode;
		int 	arraySizes[10];
		char	Chars[10][9];					// maximal 9 character in one CharList entry!

		int keyCounter;
		int last_digit;

		virtual void NormalKeyPressed(const neutrino_msg_t key);
		virtual void keyBackspacePressed(void);
		virtual void keyRedPressed();
		virtual void keyYellowPressed();
		virtual void keyUpPressed();
		virtual void keyDownPressed();
		virtual void keyLeftPressed();
		virtual void keyRightPressed();

		virtual void paint();

	public:
		CStringInputSMS(const char * const Name, char* Value, int Size, const char * const Hint_1, const char * const Hint_2, const char * const Valid_Chars, CChangeObserver* Observ = NULL, const char * const Icon = NULL);
};

class CPINInput : public CStringInput
{
	protected:
		virtual void paintChar(int pos);
	public:
		CPINInput(const char * const Name, char* Value, int Size, const char * const Hint_1 = NULL, const char * const Hint_2 = NULL, char* Valid_Chars= "0123456789", CChangeObserver* Observ = NULL)
		 : CStringInput(Name, Value, Size, Hint_1, Hint_2, Valid_Chars, Observ, "lock.raw") {};

		 int exec( CMenuTarget* parent, const std::string & actionKey );
};

class CPLPINInput : public CPINInput
{
	protected:
		int	fsk;

		virtual int handleOthers(const neutrino_msg_t msg, const neutrino_msg_data_t data);
	public:
		CPLPINInput(const char * const Name, char* Value, int Size, const char * const Hint_1, int FSK )
		 : CPINInput(Name, Value, Size, " ", Hint_1) { fsk= FSK; };

		int exec( CMenuTarget* parent, const std::string & actionKey );
};

class CPINChangeWidget : public CStringInput
{
	public:
		CPINChangeWidget(const char * const Name, char* Value, int Size, const char * const Hint_1 = NULL, const char * const Hint_2 = NULL, char* Valid_Chars= "0123456789", CChangeObserver* Observ = NULL)
		: CStringInput(Name, Value, Size, Hint_1, Hint_2, Valid_Chars, Observ){};
};


#endif
