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


#include <config.h>

#include "localize.h"
#include "iso639.h"
#include <zapit/client/zapitclient.h> /* CZapitClient::Utf8_to_Latin1 */


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

void CLocaleManager::loadLocale(std::string locale)
{
	std::string filename[] = {"/var/tuxbox/config/locale/" + locale + ".locale",DATADIR  "/neutrino/locale/" + locale + ".locale"};
	FILE* fd = fopen(filename[0].c_str(), "r");
	if(!fd)
	{
		fd = fopen(filename[1].c_str(), "r");
		if(!fd)
		{		
			perror("cannot read locale");
			return;
		}
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

			std::string text= valstr;

			int pos;
			do
			{
				pos = text.find("\\n");
				if ( pos!=-1 )
				{
					text.replace(pos, 2, "\n", 1);
				}
			} while ( ( pos != -1 ) );

			if (
			    (strncmp(keystr, "file", 4) != 0) &&  // filebrowser.head (FIXME)
			    (strncmp(keystr, "mp3player", 9) != 0) &&
			    (strncmp(keystr, "streaminfo", 10) != 0) &&
			    (strcmp(keystr, "epglist.head") != 0) &&
			    (strcmp(keystr, "channellist.head") != 0) &&
			    (strcmp(keystr, "flashupdate.head") != 0) &&
			    (strncmp(keystr, "flashupdate.title", 17) != 0) &&
			    (strncmp(keystr, "pictureviewer", 13) != 0) &&
			    (strncmp(keystr, "scants.f", 8) != 0)
			    )
				text = CZapitClient::Utf8_to_Latin1(text);
			localeData[keystr] = text;
		}
	}
	fclose(fd);
}

std::string CLocaleManager::getText(std::string keyName)
{
	std::string erg = localeData[keyName];
	if (erg == "")
		return keyName;
	else
		return erg;
}
