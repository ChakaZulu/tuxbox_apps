/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: helper.h,v 1.12 2006/03/29 15:31:55 yjogol Exp $

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


#ifndef __nhttpd_helper_h__
#define __nhttpd_helper_h__

// c++
#include <string>

std::string b64decode(char *s);
std::string itoa(unsigned int conv);
std::string itoh(unsigned int conv);

bool ySplitString(std::string str, std::string delimiter, std::string& left, std::string& right);
bool nocase_compare (char c1, char c2);

#endif /* __nhttpd_helper_h__ */
