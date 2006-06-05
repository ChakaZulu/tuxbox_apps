#ifndef __RADIOBOX_H__
#define __RADIOBOX_H__
/***************************************************************************
 *            radiobox.h
 *
 *  Wed Apr 13 17:23:25 2005
 *  Copyright  2005  User
 *  Email
 ****************************************************************************/

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "audioplay.h"
#include <vector>
#include <string>
#include <list>
#include <rcinput.h>
#include <playlist.h>

typedef std::vector<std::string> strarray;

class CStateHandler;


class CRadioBox
{
public:
	
	enum KEYS
	{
		POWER = 0,
		OPEN,
		TITLE,
		DISPLAY,
		SELECT,
		MENU,
		ZOOM,
		RETURN,
		UP,
		LEFT,
		RIGHT,
		DOWN,
		PLAY,
		STOP,
		PREV,
		NEXT,
		REW,
		FF,
		SUBTITLE,
		AUDIO,
		ANGLE,
		SEARCH,
		PROGRAM,
		REPEAT,
		AB,
		TIME,
		ONE,
		TWO,
		THREE,
		CLEAR,
		FOUR,
		FIVE,
		SIX,
		TEN,
		SEVEN,
		EIGHT,
		NINE,
		ZERO,

		UNKNOWN,
		NOKEY
	};

private:
	
	CStateHandler* statehandler;
	std::list<CStateHandler*> handlers;

	void PushHandler( CStateHandler* _handler );
	CStateHandler* PopHandler();

	CRadioBox();

	CAudioPlayer* audioplayer;


/// RC INput interface for event comm channel
	CRCInput	rcinput;

/// Lircd interface 
	int lircd;
	void ReadFromLircd();
	void OpenLircd();

	void HandleKeys();
	void ReadKeys();
	KEYS TranslateKey( std::string _key );
	KEYS TranslateKey( int _key );

	KEYS	key;
	bool	keypressed; // pressed or not

/*****************************/

	CPlayList* playlist;

public:

	static CRadioBox* GetInstance();
	
	void Run();
	
};

#endif //__RADIOBOX_H__
