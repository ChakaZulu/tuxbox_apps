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

#include <playlist.h>
#include "audioplay.h"
#include <vector>
#include <string>
#include <list>

typedef std::vector<std::string> strarray;

class CStateHandler;


class CRadioBox
{
public:
	

private:
	bool working;	
	CStateHandler* statehandler;
	std::list<CStateHandler*> handlers;

	void PushHandler( CStateHandler* _handler );
	CStateHandler* PopHandler();

	CRadioBox();

	CAudioPlayer* audioplayer;

	//////////////////////////
	void HandleKeys();
	//////////////////////////

	CPlayList* playlist;

public:

	static CRadioBox* GetInstance();
	
	void Run();
	
};

#endif //__RADIOBOX_H__
