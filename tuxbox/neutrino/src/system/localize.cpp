/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Kommentar:

	Diese GUI wurde von Grund auf neu programmiert und sollte nun vom
	Aufbau und auch den Ausbaumoeglichkeiten gut aussehen. Neutrino basiert
	auf der Client-Server Idee, diese GUI ist also von der direkten DBox-
	Steuerung getrennt. Diese wird dann von Daemons uebernommen.


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

/*
$Id: localize.cpp,v 1.1 2002/04/28 23:00:49 McClean Exp $


$Log: localize.cpp,v $
Revision 1.1  2002/04/28 23:00:49  McClean
renamed localize

Revision 1.13  2002/04/14 08:34:30  Simplex
return key when not in string-table

Revision 1.12  2002/02/28 17:34:10  McClean
repair locale-api

Revision 1.11  2002/02/28 15:03:55  field
Weiter Updates :)

Revision 1.10  2002/01/03 20:03:20  McClean
cleanup

Revision 1.9  2001/12/03 19:09:10  McClean
fixed install targets

Revision 1.8  2001/11/26 02:34:04  McClean
include (.../../stuff) changed - correct unix-formated files now

Revision 1.7  2001/11/15 11:42:41  McClean
gpl-headers added

Revision 1.6  2001/10/14 14:30:47  rasc
-- EventList Darstellung ueberarbeitet
-- kleiner Aenderungen und kleinere Bugfixes
-- locales erweitert..



*/



#include "locale.h"
#include <config.h>
#include "iso639.h"

char* getISO639Description(char *iso)
{
	unsigned int i;
	for (i=0; i<sizeof(iso639)/sizeof(*iso639); ++i)
	{
		if (!strcmp(iso639[i].iso639foreign, iso))
			return iso639[i].description1;
		if (!strcmp(iso639[i].iso639int, iso))
			return iso639[i].description1;
	}
	return iso;
}

void CLocaleManager::loadLocale(string locale)
{
	string filename = DATADIR  "/neutrino/locale/" + locale + ".locale";
	FILE* fd = fopen(filename.c_str(), "r");
	if(!fd)
	{
		perror("cannot read locale");
		return;
	}

	//	printf("read locale: %s\n", locale.c_str() );
	localeData.clear();

	char buf[1000];
	char keystr[1000];
	char valstr[1000];

	while(!feof(fd))
	{
		if(fgets(buf,sizeof(buf),fd)!=NULL)
		{
			char* tmpptr=buf;
			char* key= (char*) &keystr;
			char* val= (char*) &valstr;
			bool keyfound = false;
			for(; (*tmpptr!=10) && (*tmpptr!=13);tmpptr++)
			{
				if((*tmpptr==' ') && (!keyfound))
				{
					keyfound=true;
				}
				else
				{
					if(!keyfound)
					{
						*key = *tmpptr;
						key++;
					}
					else
					{
						*val = *tmpptr;
						val++;
					}
				}
			}
			*val = 0;
			*key = 0;

			string text= valstr;

			int pos;
			do
			{
				pos = text.find("\\n");
				if ( pos!=-1 )
				{
					text.replace(pos, 2, "\n", 1);
				}
			} while ( ( pos != -1 ) );

			localeData[keystr] = text;
		}
	}
	fclose(fd);
}


string CLocaleManager::getText(string keyName)
{
	string erg = localeData[keyName];
	if (erg == "")
		return keyName;
	else
		return erg;
}
