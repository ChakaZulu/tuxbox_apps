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

			if (
			    (strncmp(keystr, "file", 4) != 0) &&  // filebrowser.head (FIXME)
			    (strncmp(keystr, "flashupdate.title", 17) != 0) &&
			    (strncmp(keystr, "flashupdate.updatemode_", 23) != 0) &&
			    (strncmp(keystr, "lcdmenu.statusline.", 19) != 0) &&
			    (strncmp(keystr, "mainmenu.recording_", 19) != 0) &&
			    (strncmp(keystr, "mp3player", 9) != 0) &&
			    (strncmp(keystr, "nfs.t", 5) != 0) &&
			    (strncmp(keystr, "parentallock.lockage1", 21) != 0) &&
			    (strncmp(keystr, "pictureviewer", 13) != 0) &&
			    (strncmp(keystr, "recordingmenu.o", 15) != 0) &&
			    (strncmp(keystr, "recordingmenu.v", 15) != 0) &&
			    (strncmp(keystr, "satsetup.mi", 11) != 0) &&
			    (strncmp(keystr, "satsetup.n", 10) != 0) &&
			    (strncmp(keystr, "satsetup.diseqc1", 16) != 0) &&
			    (strncmp(keystr, "streaminfo", 10) != 0) &&
			    (strncmp(keystr, "streamingmenu.o", 15) != 0) &&
			    (strncmp(keystr, "timerlist.d", 11) != 0) &&
			    (strncmp(keystr, "timerlist.modi", 14) != 0) &&
			    (strncmp(keystr, "timerlist.n", 11) != 0) &&
			    (strncmp(keystr, "timerlist.p", 11) != 0) &&
			    (strncmp(keystr, "timerlist.standby.", 18) != 0) &&
			    (strncmp(keystr, "timerlist.type.", 15) != 0) &&
			    (strncmp(keystr, "videomenu.videoformat_", 22) != 0) &&
			    (strncmp(keystr, "videomenu.videosignal_", 22) != 0) &&
			    ((strncmp(keystr, "timerlist.r", 11) != 0) || (strcmp(keystr, "timerlist.repeat") == 0)) &&
			    ((strncmp(keystr, "options", 7) != 0) || (strcmp(keystr, "options.default") == 0)) &&
			    ((strncmp(keystr, "scants", 6) != 0) || ((strcmp(keystr, "scants.bouquet") == 0) || (strncmp(keystr, "scants.s", 8) == 0))) &&
			    (strcmp(keystr, "audiomenu.avs") != 0) &&
			    (strcmp(keystr, "audiomenu.lirc") != 0) &&
			    (strcmp(keystr, "audiomenu.monoleft") != 0) &&
			    (strcmp(keystr, "audiomenu.monoright") != 0) &&
			    (strcmp(keystr, "audiomenu.ost") != 0) &&
			    (strcmp(keystr, "audiomenu.stereo") != 0) &&
			    (strcmp(keystr, "bouquetlist.head") != 0) &&
			    (strcmp(keystr, "channellist.head") != 0) &&
			    (strcmp(keystr, "epglist.head") != 0) &&
			    (strcmp(keystr, "flashupdate.head") != 0) &&
			    (strcmp(keystr, "keybindingmenu.allchannels_on_ok") != 0) &&
			    (strcmp(keystr, "keybindingmenu.bouquetchannels_on_ok") != 0) &&
			    (strcmp(keystr, "keybindingmenu.bouquetlist_on_ok") != 0) &&
			    (strcmp(keystr, "messagebox.no") != 0) &&
			    (strcmp(keystr, "messagebox.yes") != 0) &&
			    (strcmp(keystr, "parentallock.changetolocked") != 0) &&
			    (strcmp(keystr, "parentallock.never") != 0) &&
			    (strcmp(keystr, "parentallock.onsignal") != 0) &&
			    (strcmp(keystr, "parentallock.onstart") != 0) &&
			    (strcmp(keystr, "recordingmenu.server") != 0)
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
