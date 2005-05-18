/*
	NeutrinoNG  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

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
	std::string result;
	std::string::size_type pos = 0;
	while(true)
	{
		std::string::size_type next = s.find(to_find, pos);
		result.append(s, pos, next-pos);
		if( next != std::string::npos )
		{
			result.append(repl_with);
			pos = next + to_find.size();
		}
		else
		{
			break;  // exit loop
		}
	}
	s.swap(result);
}
