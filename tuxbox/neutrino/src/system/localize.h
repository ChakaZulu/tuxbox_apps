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


#ifndef __locale__
#define __locale__

#include_next<locale.h>
#include <string>
#include <map>

const char * getISO639Description(const char * const iso);

typedef std::map<std::string, std::string> mapLocaleData;

class CLocaleManager
{
	private:
		mapLocaleData localeData;
		
	public:
		enum loadLocale_ret_t
			{
				ISO_8859_1_FONT =  0,
				UNICODE_FONT    =  1,
				NO_SUCH_LOCALE  = -1
			};

		loadLocale_ret_t loadLocale(const char * const locale);

		const char * getText(const std::string & keyName) const;
		const char * getText(const char * const keyName) const;
};

typedef const char * neutrino_locale_t;

#define LOCALE_EPGMENU_EPGPLUS              "EPGMenu.epgplus"
#define LOCALE_EPGMENU_EVENTINFO            "EPGMenu.eventinfo"
#define LOCALE_EPGMENU_EVENTLIST            "EPGMenu.eventlist"
#define LOCALE_EPGMENU_HEAD                 "EPGMenu.head"
#define LOCALE_EPGMENU_STREAMINFO           "EPGMenu.streaminfo"
#define LOCALE_EPGPLUS_HEAD                 "EPGPlus.head"
#define LOCALE_EPGPLUS_RECORD               "EPGPlus.record"
#define LOCALE_EPGPLUS_REFRESH_EPG          "EPGPlus.refresh_epg"
#define LOCALE_EPGPLUS_REMIND               "EPGPlus.remind"
#define LOCALE_EPGPLUS_SCROLL_MODE          "EPGPlus.scroll_mode"
#define LOCALE_EPGPLUS_STRETCH_MODE         "EPGPlus.stretch_mode"
#define LOCALE_GENRE_ARTS_0                 "GENRE.ARTS.0"
#define LOCALE_GENRE_ARTS_1                 "GENRE.ARTS.1"
#define LOCALE_GENRE_ARTS_10                "GENRE.ARTS.10"
#define LOCALE_GENRE_ARTS_11                "GENRE.ARTS.11"
#define LOCALE_GENRE_ARTS_2                 "GENRE.ARTS.2"
#define LOCALE_GENRE_ARTS_3                 "GENRE.ARTS.3"
#define LOCALE_GENRE_ARTS_4                 "GENRE.ARTS.4"
#define LOCALE_GENRE_ARTS_5                 "GENRE.ARTS.5"
#define LOCALE_GENRE_ARTS_6                 "GENRE.ARTS.6"
#define LOCALE_GENRE_ARTS_7                 "GENRE.ARTS.7"
#define LOCALE_GENRE_ARTS_8                 "GENRE.ARTS.8"
#define LOCALE_GENRE_ARTS_9                 "GENRE.ARTS.9"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_0 "GENRE.CHILDRENs_PROGRAMMES.0"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_1 "GENRE.CHILDRENs_PROGRAMMES.1"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_2 "GENRE.CHILDRENs_PROGRAMMES.2"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_3 "GENRE.CHILDRENs_PROGRAMMES.3"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_4 "GENRE.CHILDRENs_PROGRAMMES.4"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_5 "GENRE.CHILDRENs_PROGRAMMES.5"
#define LOCALE_GENRE_DOCUS_MAGAZINES_0      "GENRE.DOCUS_MAGAZINES.0"
#define LOCALE_GENRE_DOCUS_MAGAZINES_1      "GENRE.DOCUS_MAGAZINES.1"
#define LOCALE_GENRE_DOCUS_MAGAZINES_2      "GENRE.DOCUS_MAGAZINES.2"
#define LOCALE_GENRE_DOCUS_MAGAZINES_3      "GENRE.DOCUS_MAGAZINES.3"
#define LOCALE_GENRE_DOCUS_MAGAZINES_4      "GENRE.DOCUS_MAGAZINES.4"
#define LOCALE_GENRE_DOCUS_MAGAZINES_5      "GENRE.DOCUS_MAGAZINES.5"
#define LOCALE_GENRE_DOCUS_MAGAZINES_6      "GENRE.DOCUS_MAGAZINES.6"
#define LOCALE_GENRE_DOCUS_MAGAZINES_7      "GENRE.DOCUS_MAGAZINES.7"
#define LOCALE_GENRE_MOVIE_0                "GENRE.MOVIE.0"
#define LOCALE_GENRE_MOVIE_1                "GENRE.MOVIE.1"
#define LOCALE_GENRE_MOVIE_2                "GENRE.MOVIE.2"
#define LOCALE_GENRE_MOVIE_3                "GENRE.MOVIE.3"
#define LOCALE_GENRE_MOVIE_4                "GENRE.MOVIE.4"
#define LOCALE_GENRE_MOVIE_5                "GENRE.MOVIE.5"
#define LOCALE_GENRE_MOVIE_6                "GENRE.MOVIE.6"
#define LOCALE_GENRE_MOVIE_7                "GENRE.MOVIE.7"
#define LOCALE_GENRE_MOVIE_8                "GENRE.MOVIE.8"
#define LOCALE_GENRE_MUSIC_DANCE_0          "GENRE.MUSIC_DANCE.0"
#define LOCALE_GENRE_MUSIC_DANCE_1          "GENRE.MUSIC_DANCE.1"
#define LOCALE_GENRE_MUSIC_DANCE_2          "GENRE.MUSIC_DANCE.2"
#define LOCALE_GENRE_MUSIC_DANCE_3          "GENRE.MUSIC_DANCE.3"
#define LOCALE_GENRE_MUSIC_DANCE_4          "GENRE.MUSIC_DANCE.4"
#define LOCALE_GENRE_MUSIC_DANCE_5          "GENRE.MUSIC_DANCE.5"
#define LOCALE_GENRE_MUSIC_DANCE_6          "GENRE.MUSIC_DANCE.6"
#define LOCALE_GENRE_NEWS_0                 "GENRE.NEWS.0"
#define LOCALE_GENRE_NEWS_1                 "GENRE.NEWS.1"
#define LOCALE_GENRE_NEWS_2                 "GENRE.NEWS.2"
#define LOCALE_GENRE_NEWS_3                 "GENRE.NEWS.3"
#define LOCALE_GENRE_NEWS_4                 "GENRE.NEWS.4"
#define LOCALE_GENRE_SHOW_0                 "GENRE.SHOW.0"
#define LOCALE_GENRE_SHOW_1                 "GENRE.SHOW.1"
#define LOCALE_GENRE_SHOW_2                 "GENRE.SHOW.2"
#define LOCALE_GENRE_SHOW_3                 "GENRE.SHOW.3"
#define LOCALE_GENRE_SOCIAL_POLITICAL_0     "GENRE.SOZIAL_POLITICAL.0"
#define LOCALE_GENRE_SOCIAL_POLITICAL_1     "GENRE.SOZIAL_POLITICAL.1"
#define LOCALE_GENRE_SOCIAL_POLITICAL_2     "GENRE.SOZIAL_POLITICAL.2"
#define LOCALE_GENRE_SOCIAL_POLITICAL_3     "GENRE.SOZIAL_POLITICAL.3"
#define LOCALE_GENRE_SPORTS_0               "GENRE.SPORTS.0"
#define LOCALE_GENRE_SPORTS_1               "GENRE.SPORTS.1"
#define LOCALE_GENRE_SPORTS_10              "GENRE.SPORTS.10"
#define LOCALE_GENRE_SPORTS_11              "GENRE.SPORTS.11"
#define LOCALE_GENRE_SPORTS_2               "GENRE.SPORTS.2"
#define LOCALE_GENRE_SPORTS_3               "GENRE.SPORTS.3"
#define LOCALE_GENRE_SPORTS_4               "GENRE.SPORTS.4"
#define LOCALE_GENRE_SPORTS_5               "GENRE.SPORTS.5"
#define LOCALE_GENRE_SPORTS_6               "GENRE.SPORTS.6"
#define LOCALE_GENRE_SPORTS_7               "GENRE.SPORTS.7"
#define LOCALE_GENRE_SPORTS_8               "GENRE.SPORTS.8"
#define LOCALE_GENRE_SPORTS_9               "GENRE.SPORTS.9"
#define LOCALE_GENRE_TRAVEL_HOBBIES_0       "GENRE.TRAVEL_HOBBIES.0"
#define LOCALE_GENRE_TRAVEL_HOBBIES_1       "GENRE.TRAVEL_HOBBIES.1"
#define LOCALE_GENRE_TRAVEL_HOBBIES_2       "GENRE.TRAVEL_HOBBIES.2"
#define LOCALE_GENRE_TRAVEL_HOBBIES_3       "GENRE.TRAVEL_HOBBIES.3"
#define LOCALE_GENRE_TRAVEL_HOBBIES_4       "GENRE.TRAVEL_HOBBIES.4"
#define LOCALE_GENRE_TRAVEL_HOBBIES_5       "GENRE.TRAVEL_HOBBIES.5"
#define LOCALE_GENRE_TRAVEL_HOBBIES_6       "GENRE.TRAVEL_HOBBIES.6"
#define LOCALE_GENRE_TRAVEL_HOBBIES_7       "GENRE.TRAVEL_HOBBIES.7"
#define LOCALE_GENRE_UNKNOWN                "GENRE.UNKNOWN"

#endif
