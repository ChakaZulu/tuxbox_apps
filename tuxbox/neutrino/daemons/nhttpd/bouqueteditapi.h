/*      
        nhttpd  -  DBoxII-Project

        Copyright (C) 2001/2002 Dirk Szymanski 'Dirch'

        $Id: bouqueteditapi.h,v 1.4 2003/03/14 07:20:01 obi Exp $

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

#ifndef __nhttpd_bouqueteditapi_h__
#define __nhttpd_bouqueteditapi_h__

#include "request.h"
#include "webdbox.h"

class CBouqueteditAPI
{
	protected:
		CWebDbox *Parent;

		bool showBouquets(CWebserverRequest *request);
		bool addBouquet(CWebserverRequest *request);
		bool moveBouquet(CWebserverRequest *request);
		bool deleteBouquet(CWebserverRequest *request);
		bool saveBouquet(CWebserverRequest *request);
		bool renameBouquet(CWebserverRequest *request);
		bool editBouquet(CWebserverRequest *request);
		bool changeBouquet(CWebserverRequest *request);
		bool setBouquet(CWebserverRequest *request);

	public:
		CBouqueteditAPI(CWebDbox *parent) { Parent = parent; };
		bool Execute(CWebserverRequest *request);
};

#endif /* __nhttpd_bouqueteditapi_h__ */
