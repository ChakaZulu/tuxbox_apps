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
#include <zapit/client/zapitclient.h> /* CZapitClient::Utf8_to_Latin1 */

#include <fstream>
#include <iostream>
#include <map>
#include <string>

static const char * iso639filename = "/share/iso-codes/iso-639.tab";
static std::map<std::string, std::string> iso639;

void initialize_iso639_map(void)
{
	std::string s, t, u, v;
	std::ifstream in(iso639filename);
	if (in.is_open())
	{
		while (in.peek() == '#')
			getline(in, s);
		while (in >> s >> t >> u)
		{
			getline(in, v);
			iso639[s] = v;
			if (s != t)
				iso639[t] = v;
		}
	}
	else
		std::cout << "Loading " << iso639filename << " failed." << std::endl;
}

const char * getISO639Description(const char *iso)
{
	std::map<std::string, std::string>::const_iterator it = iso639.find(std::string(iso));
	if (it == iso639.end())
		return iso;
	else
		return it->second.c_str();
}

void CLocaleManager::loadLocale(std::string locale)
{
	initialize_iso639_map();
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

#warning NFS/CIFS is missing in locales and should be UTF-8 encoded 
			if (
			    (strncmp(keystr, "apidselector", 12) != 0) &&
			    (strncmp(keystr, "audiomenu", 9) != 0) &&
			    (strncmp(keystr, "bouqueteditor", 13) != 0) &&
			    (strncmp(keystr, "cablesetup", 10) != 0) &&
			    (strncmp(keystr, "colormenu", 9) != 0) &&
			    (strncmp(keystr, "colorstatusbar", 14) != 0) &&
			    (strncmp(keystr, "colorthememenu", 14) != 0) &&
			    (strncmp(keystr, "file", 4) != 0) &&
			    (strncmp(keystr, "fontmenu", 8) != 0) &&
			    (strncmp(keystr, "fontsize", 8) != 0) &&
			    (strncmp(keystr, "k", 1) != 0) &&
			    (strncmp(keystr, "languagesetup", 13) != 0) &&
			    (strncmp(keystr, "lcdcontroler", 12) != 0) &&
			    (strncmp(keystr, "lcdmenu", 7) != 0) &&
			    (strncmp(keystr, "mainmenu", 8) != 0) &&
			    (strncmp(keystr, "messagebox", 10) != 0) &&
			    (strncmp(keystr, "miscsettings", 12) != 0) &&
			    (strncmp(keystr, "mp3", 3) != 0) &&
			    (strncmp(keystr, "networkmenu", 11) != 0) &&
			    (strncmp(keystr, "nfs", 3) != 0) &&
			    (strncmp(keystr, "nvodselector", 12) != 0) &&
			    (strncmp(keystr, "options", 7) != 0) &&
			    (strncmp(keystr, "parentallock", 12) != 0) &&
			    (strncmp(keystr, "pictureviewer", 13) != 0) &&
			    (strncmp(keystr, "recordingmenu", 13) != 0) &&
			    (strncmp(keystr, "satsetup", 8) != 0) &&
			    (strncmp(keystr, "scants", 6) != 0) &&
			    (strncmp(keystr, "streamfeatures", 14) != 0) &&
			    (strncmp(keystr, "streaminfo", 10) != 0) &&
			    (strncmp(keystr, "streamingmenu", 13) != 0) &&
			    (strncmp(keystr, "timersettings", 13) != 0) &&
			    (strncmp(keystr, "timing", 6) != 0) &&
			    (strncmp(keystr, "v", 1) != 0) &&
			    ((strncmp(keystr, "flashupdate", 11) != 0) || 
			     ((strcmp(keystr, "flashupdate.actionreadflash") == 0) ||
			      (strcmp(keystr, "flashupdate.getinfofile") == 0) ||
			      (strcmp(keystr, "flashupdate.getupdatefile") == 0) ||
			      (strcmp(keystr, "flashupdate.md5check") == 0) ||
			      (strncmp(keystr, "flashupdate.msgbox", 18) == 0) ||
			      (strcmp(keystr, "flashupdate.ready") == 0) ||
			      (strcmp(keystr, "flashupdate.reallyflashmtd") == 0) ||
			      (strcmp(keystr, "flashupdate.savesuccess") == 0) ||
			      (strcmp(keystr, "flashupdate.versioncheck") == 0))) &&
			    ((strncmp(keystr, "mainsettings", 12) != 0) || (strcmp(keystr, "mainsettings.savesettingsnow_hint") == 0)) &&
			    ((strncmp(keystr, "servicemenu", 11) != 0) || (strcmp(keystr, "servicemenu.reload_hint") == 0)) &&
			    ((strncmp(keystr, "timerlist", 9) != 0) || (strncmp(keystr, "timerlist.weekdays.hint", 23) == 0)) &&
			    (strcmp(keystr, "bouquetlist.head") != 0) &&
			    (strcmp(keystr, "channellist.head") != 0) &&
			    (strcmp(keystr, "dhcp") != 0) &&
			    (strcmp(keystr, "epglist.head") != 0) &&
			    (strcmp(keystr, "menu.back") != 0) &&
			    (strcmp(keystr, "sleeptimerbox.title") != 0) &&
			    (strcmp(keystr, "ucodecheck.head") != 0)
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
