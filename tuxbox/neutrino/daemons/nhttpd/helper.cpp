/*
	webserver  -   DBoxII-Project

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

	// Revision 1.1  11.02.2002 20:20  dirch

*/

#include "helper.h"


void TString::Set(char * t, int len)
{
	laenge=len;
	if(laenge > 0)
		if((text = (char *) malloc(laenge+1)) != NULL){
			strncpy(text,t,laenge);
			text[laenge] = 0;
		}else perror("Kein Speicher in TString\n");	
}

//-------------------------------------------------------------------------
