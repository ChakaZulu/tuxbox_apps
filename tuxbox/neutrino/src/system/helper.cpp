/*
	NeutrinoNG  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/
	
	$Id: helper.cpp,v 1.4 2009/02/24 19:27:59 seife Exp $
	
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


#include "helper.h"

void StrSearchReplace( std::string &s, const std::string &to_find, const std::string& repl_with )
{
	std::string::size_type location = s.find(to_find);
	if ( location == std::string::npos )
	{
		return;
	}
	while ( location != std::string::npos )
	{
		s.erase(location,to_find.size());
		s.insert(location,repl_with);
		location = s.find(to_find, location);
	}
}
