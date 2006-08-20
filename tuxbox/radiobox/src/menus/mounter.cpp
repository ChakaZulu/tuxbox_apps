/*
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

// we implement here CMountSetup from statehandler.h
// and CMounter from mounter.h

#include <statehandler.h>
#include <mounter.h>
#include <system/fsmounter.h>
#include <global.h>
#include <lcdd.h>


#include <sstream>

CMountSetup::CMountSetup()
{
	int i;
	title = "Mounts";
	for( i = 0; i < NETWORK_NFS_NR_OF_ENTRIES; i++ )
	{
		std::stringstream ss;
		ss << i+1 << ": " << CMounter::getMount( i ).getLocalDir();
		entries.push_back( ss.str() );
	}
}

void CMountSetup::DoAction( std::string _action )
{
	this->subhandler = new CMountEdit( 0 );

}

#define ME_CURRENT(c) ( current == c )?cursor:-2
void CMountEdit::Show()
{
	static bool flag = true;
	if( flag )
	{
		CLCD::getInstance()->clear();
		flag = false;
	}
	CLCD::getInstance()->showEditBox( "192.168.002.019", 0, ME_CURRENT( me_ip ) );
	CLCD::getInstance()->showEditBox( "/home/georg/media/music", 1, ME_CURRENT( me_dir ) );
	CLCD::getInstance()->showEditBox( "/mnt/base/music", 2, ME_CURRENT( me_ldir ) );
	CLCD::getInstance()->showEditBox( "", 3, ME_CURRENT( me_username ) );
	CLCD::getInstance()->showEditBox( "", 4, ME_CURRENT( me_password ) );
}


void CMountEdit::HandleKeys( CRBXInput::KEYS _key, bool _pressed )
{
	if( _pressed ) return; /* handle it only if key released is */

	switch( _key )
	{
		case CRBXInput::SELECT:
			break;
		case CRBXInput::LEFT:
			MoveCursor( -1 );
			break;
		case CRBXInput::RIGHT:
			MoveCursor( 1 );
			break;
		case CRBXInput::UP:
			Next( -1 );
			break;
		case CRBXInput::DOWN:
			Next( 1 );
			break;
		case CRBXInput::MENU:
			remove = true;
			break;
		default:
			break;
	}

}

void CMountEdit::MoveCursor( int _d )
{
	cursor += _d;
}

void CMountEdit::ChangeLetter( int _d )
{

}

void CMountEdit::Next( int _d )
{
	switch( current )
	{
	case me_ip: current = (_d == -1)?current:me_dir; break;
	case me_dir: current = (_d == -1)?me_ip:me_ldir; break;
	case me_ldir: current = (_d == -1)?me_dir:me_username; break;
	case me_username: current = (_d == -1)?me_ldir:me_password; break;
	case me_password: current = (_d == -1)?me_username:current; break;
	}
}

