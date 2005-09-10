/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: yapi.h,v 1.1 2005/09/10 12:28:32 yjogol Exp $

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

#ifndef __nhttpd_yapi_h__
#define __nhttpd_yapi_h__

// c++
#include <map>
#include <string>

// tuxbox
//#include <driver/yd.h>
#include <global.h>
#include <neutrino.h>
#include <system/settings.h>

//#include <ydisplay/ydisplay.h>
//#include <ydisplay/fontrenderer.h>

#include <dbox/fp.h>
#include <fcntl.h>
//#include <unistd.h>

// nhttpd
#include "helper.h"
#include "request.h"

class CWebserver;
class CWebserverRequest;
class CControlAPI;


#include "controlapi.h"

//-------------------------------------------------------------------------
// ycgi : command escapes
//-------------------------------------------------------------------------
#define YCGI_ESCAPE_START "{="
#define YCGI_ESCAPE_END "=}"

#define yHTML_SEARCHFOLDER_MOUNT "/mnt/httpd"

//-------------------------------------------------------------------------


class CyAPI
{
	CWebDbox			*Parent;

	std::string YWeb_cgi_get_ini(std::string filename, std::string varname);
	void YWeb_cgi_set_ini(std::string filename, std::string varname);
	std::string YWeb_cgi_cmd(CWebserverRequest* request, std::string ycmd);
	std::string cgi_cmd_parsing(CWebserverRequest* request, std::string html_template, bool ydebug);
	std::string YWeb_cgi_func(CWebserverRequest* request, std::string ycmd);

public:

//-------------------------------------------------------------------------
// Search folders for html files
//-------------------------------------------------------------------------
	static const unsigned int HTML_DIR_COUNT = 3;
 	std::string HTML_DIRS[HTML_DIR_COUNT];

	CyAPI(CWebDbox *webdbox);

	// Execute calls
	bool Execute(CWebserverRequest* request);
	bool cgi(CWebserverRequest *request);

	friend class CControlAPI;

};

#endif /* __nhttpd_yapi_h__ */
