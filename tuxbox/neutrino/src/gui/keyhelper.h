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
*/


#ifndef __keyhelper__
#define __keyhelper__


#include "widget/icons.h"
//#include <driver/rcinput.h>


#define BUTTONMAX 4
static const neutrino_msg_t key_helper_msg_def[BUTTONMAX]=
{
	CRCInput::RC_red,
	CRCInput::RC_green,
	CRCInput::RC_yellow,
	CRCInput::RC_blue
};

static const char * key_helper_icon_def[BUTTONMAX]=
{
	NEUTRINO_ICON_BUTTON_RED,
	NEUTRINO_ICON_BUTTON_GREEN,
	NEUTRINO_ICON_BUTTON_YELLOW,
	NEUTRINO_ICON_BUTTON_BLUE}
;

// USERMENU
// This is just a quick helper for the usermenu only. I already made it a class for future use.

class CKeyHelper
{
	private:
		int number_key;
		bool color_key_used[BUTTONMAX];
	public:
		CKeyHelper(){reset();};
		void reset(void)
		{
			number_key = 1;
			for(int i= 0; i < BUTTONMAX; i++ )
				color_key_used[i] = false;
		};

		/* Returns the next available button, to be used in menu as 'direct' keys. Appropriate
		 * definitions are returnd in msp and icon
		 * A color button could be requested as prefered button (other buttons are not supported yet). 
		 * If the appropriate button is already in used, the next number_key button is returned instead 
		 * (first 1-9 and than 0). */
		bool get(neutrino_msg_t* msg, const char** icon, neutrino_msg_t prefered_key = CRCInput::RC_nokey)
		{
			bool result = false;
			int button = -1;
			if(prefered_key == CRCInput::RC_red)
				button = 0;
			if(prefered_key == CRCInput::RC_green)
				button = 1;
			if(prefered_key == CRCInput::RC_yellow)
				button = 2;
			if(prefered_key == CRCInput::RC_blue)
				button = 3;
		
			if((button >= 0) && (button < BUTTONMAX) && (color_key_used[button] == true)) {
				button = -1;
				for(int i = 0; i < BUTTONMAX; i++)
				{ // try to get color button
					if( color_key_used[i] == false)
					{
						button = i;
						break;
					}
				}
			}
			*msg = CRCInput::RC_nokey;
			*icon = "";
			if(button >= 0 && button < BUTTONMAX)
			{ // try to get color button
				if( color_key_used[button] == false) 
				{
					color_key_used[button] = true;
					*msg = key_helper_msg_def[button];
					*icon = key_helper_icon_def[button];
					result = true;
				}
			}
			
			if( result == false && number_key < 10) // no key defined yet, at least try to get a numbered key
			{
				// there is still a available number_key
				*msg = CRCInput::convertDigitToKey(number_key);
				*icon = "";
				if(number_key == 9)
					number_key = 0;
				else if(number_key == 0)
					number_key = 10;
				else 
					number_key++;
					
				result = true;
			}
			return (result);
		};
};


#endif
