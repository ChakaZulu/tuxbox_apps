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
#include <time.h>
#include <string>
#include <map>

const char * getISO639Description(const char * const iso);

typedef std::map<std::string, std::string> mapLocaleData;

typedef const char * neutrino_locale_t;

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
		const char * getText(const neutrino_locale_t keyName) const;

		static neutrino_locale_t getMonth  (const struct tm * struct_tm_p);
		static neutrino_locale_t getWeekday(const struct tm * struct_tm_p);
};

#define LOCALE_EPGMENU_EPGPLUS                     "EPGMenu.epgplus"
#define LOCALE_EPGMENU_EVENTINFO                   "EPGMenu.eventinfo"
#define LOCALE_EPGMENU_EVENTLIST                   "EPGMenu.eventlist"
#define LOCALE_EPGMENU_HEAD                        "EPGMenu.head"
#define LOCALE_EPGMENU_STREAMINFO                  "EPGMenu.streaminfo"
#define LOCALE_EPGPLUS_HEAD                        "EPGPlus.head"
#define LOCALE_EPGPLUS_RECORD                      "EPGPlus.record"
#define LOCALE_EPGPLUS_REFRESH_EPG                 "EPGPlus.refresh_epg"
#define LOCALE_EPGPLUS_REMIND                      "EPGPlus.remind"
#define LOCALE_EPGPLUS_SCROLL_MODE                 "EPGPlus.scroll_mode"
#define LOCALE_EPGPLUS_STRETCH_MODE                "EPGPlus.stretch_mode"
#define LOCALE_GENRE_ARTS_0                        "GENRE.ARTS.0"
#define LOCALE_GENRE_ARTS_1                        "GENRE.ARTS.1"
#define LOCALE_GENRE_ARTS_10                       "GENRE.ARTS.10"
#define LOCALE_GENRE_ARTS_11                       "GENRE.ARTS.11"
#define LOCALE_GENRE_ARTS_2                        "GENRE.ARTS.2"
#define LOCALE_GENRE_ARTS_3                        "GENRE.ARTS.3"
#define LOCALE_GENRE_ARTS_4                        "GENRE.ARTS.4"
#define LOCALE_GENRE_ARTS_5                        "GENRE.ARTS.5"
#define LOCALE_GENRE_ARTS_6                        "GENRE.ARTS.6"
#define LOCALE_GENRE_ARTS_7                        "GENRE.ARTS.7"
#define LOCALE_GENRE_ARTS_8                        "GENRE.ARTS.8"
#define LOCALE_GENRE_ARTS_9                        "GENRE.ARTS.9"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_0        "GENRE.CHILDRENs_PROGRAMMES.0"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_1        "GENRE.CHILDRENs_PROGRAMMES.1"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_2        "GENRE.CHILDRENs_PROGRAMMES.2"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_3        "GENRE.CHILDRENs_PROGRAMMES.3"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_4        "GENRE.CHILDRENs_PROGRAMMES.4"
#define LOCALE_GENRE_CHILDRENS_PROGRAMMES_5        "GENRE.CHILDRENs_PROGRAMMES.5"
#define LOCALE_GENRE_DOCUS_MAGAZINES_0             "GENRE.DOCUS_MAGAZINES.0"
#define LOCALE_GENRE_DOCUS_MAGAZINES_1             "GENRE.DOCUS_MAGAZINES.1"
#define LOCALE_GENRE_DOCUS_MAGAZINES_2             "GENRE.DOCUS_MAGAZINES.2"
#define LOCALE_GENRE_DOCUS_MAGAZINES_3             "GENRE.DOCUS_MAGAZINES.3"
#define LOCALE_GENRE_DOCUS_MAGAZINES_4             "GENRE.DOCUS_MAGAZINES.4"
#define LOCALE_GENRE_DOCUS_MAGAZINES_5             "GENRE.DOCUS_MAGAZINES.5"
#define LOCALE_GENRE_DOCUS_MAGAZINES_6             "GENRE.DOCUS_MAGAZINES.6"
#define LOCALE_GENRE_DOCUS_MAGAZINES_7             "GENRE.DOCUS_MAGAZINES.7"
#define LOCALE_GENRE_MOVIE_0                       "GENRE.MOVIE.0"
#define LOCALE_GENRE_MOVIE_1                       "GENRE.MOVIE.1"
#define LOCALE_GENRE_MOVIE_2                       "GENRE.MOVIE.2"
#define LOCALE_GENRE_MOVIE_3                       "GENRE.MOVIE.3"
#define LOCALE_GENRE_MOVIE_4                       "GENRE.MOVIE.4"
#define LOCALE_GENRE_MOVIE_5                       "GENRE.MOVIE.5"
#define LOCALE_GENRE_MOVIE_6                       "GENRE.MOVIE.6"
#define LOCALE_GENRE_MOVIE_7                       "GENRE.MOVIE.7"
#define LOCALE_GENRE_MOVIE_8                       "GENRE.MOVIE.8"
#define LOCALE_GENRE_MUSIC_DANCE_0                 "GENRE.MUSIC_DANCE.0"
#define LOCALE_GENRE_MUSIC_DANCE_1                 "GENRE.MUSIC_DANCE.1"
#define LOCALE_GENRE_MUSIC_DANCE_2                 "GENRE.MUSIC_DANCE.2"
#define LOCALE_GENRE_MUSIC_DANCE_3                 "GENRE.MUSIC_DANCE.3"
#define LOCALE_GENRE_MUSIC_DANCE_4                 "GENRE.MUSIC_DANCE.4"
#define LOCALE_GENRE_MUSIC_DANCE_5                 "GENRE.MUSIC_DANCE.5"
#define LOCALE_GENRE_MUSIC_DANCE_6                 "GENRE.MUSIC_DANCE.6"
#define LOCALE_GENRE_NEWS_0                        "GENRE.NEWS.0"
#define LOCALE_GENRE_NEWS_1                        "GENRE.NEWS.1"
#define LOCALE_GENRE_NEWS_2                        "GENRE.NEWS.2"
#define LOCALE_GENRE_NEWS_3                        "GENRE.NEWS.3"
#define LOCALE_GENRE_NEWS_4                        "GENRE.NEWS.4"
#define LOCALE_GENRE_SHOW_0                        "GENRE.SHOW.0"
#define LOCALE_GENRE_SHOW_1                        "GENRE.SHOW.1"
#define LOCALE_GENRE_SHOW_2                        "GENRE.SHOW.2"
#define LOCALE_GENRE_SHOW_3                        "GENRE.SHOW.3"
#define LOCALE_GENRE_SOCIAL_POLITICAL_0            "GENRE.SOZIAL_POLITICAL.0"
#define LOCALE_GENRE_SOCIAL_POLITICAL_1            "GENRE.SOZIAL_POLITICAL.1"
#define LOCALE_GENRE_SOCIAL_POLITICAL_2            "GENRE.SOZIAL_POLITICAL.2"
#define LOCALE_GENRE_SOCIAL_POLITICAL_3            "GENRE.SOZIAL_POLITICAL.3"
#define LOCALE_GENRE_SPORTS_0                      "GENRE.SPORTS.0"
#define LOCALE_GENRE_SPORTS_1                      "GENRE.SPORTS.1"
#define LOCALE_GENRE_SPORTS_10                     "GENRE.SPORTS.10"
#define LOCALE_GENRE_SPORTS_11                     "GENRE.SPORTS.11"
#define LOCALE_GENRE_SPORTS_2                      "GENRE.SPORTS.2"
#define LOCALE_GENRE_SPORTS_3                      "GENRE.SPORTS.3"
#define LOCALE_GENRE_SPORTS_4                      "GENRE.SPORTS.4"
#define LOCALE_GENRE_SPORTS_5                      "GENRE.SPORTS.5"
#define LOCALE_GENRE_SPORTS_6                      "GENRE.SPORTS.6"
#define LOCALE_GENRE_SPORTS_7                      "GENRE.SPORTS.7"
#define LOCALE_GENRE_SPORTS_8                      "GENRE.SPORTS.8"
#define LOCALE_GENRE_SPORTS_9                      "GENRE.SPORTS.9"
#define LOCALE_GENRE_TRAVEL_HOBBIES_0              "GENRE.TRAVEL_HOBBIES.0"
#define LOCALE_GENRE_TRAVEL_HOBBIES_1              "GENRE.TRAVEL_HOBBIES.1"
#define LOCALE_GENRE_TRAVEL_HOBBIES_2              "GENRE.TRAVEL_HOBBIES.2"
#define LOCALE_GENRE_TRAVEL_HOBBIES_3              "GENRE.TRAVEL_HOBBIES.3"
#define LOCALE_GENRE_TRAVEL_HOBBIES_4              "GENRE.TRAVEL_HOBBIES.4"
#define LOCALE_GENRE_TRAVEL_HOBBIES_5              "GENRE.TRAVEL_HOBBIES.5"
#define LOCALE_GENRE_TRAVEL_HOBBIES_6              "GENRE.TRAVEL_HOBBIES.6"
#define LOCALE_GENRE_TRAVEL_HOBBIES_7              "GENRE.TRAVEL_HOBBIES.7"
#define LOCALE_GENRE_UNKNOWN                       "GENRE.UNKNOWN"
#define LOCALE_APIDS_HINT_1                        "apids.hint_1"
#define LOCALE_APIDS_HINT_2                        "apids.hint_2"
#define LOCALE_APIDSELECTOR_HEAD                   "apidselector.head"
#define LOCALE_AUDIOMENU_PCMOFFSET                 "audiomenu.PCMOffset"
#define LOCALE_AUDIOMENU_ANALOGOUT                 "audiomenu.analogout"
#define LOCALE_AUDIOMENU_AVS                       "audiomenu.avs"
#define LOCALE_AUDIOMENU_AVS_CONTROL               "audiomenu.avs_control"
#define LOCALE_AUDIOMENU_DOLBYDIGITAL              "audiomenu.dolbydigital"
#define LOCALE_AUDIOMENU_HEAD                      "audiomenu.head"
#define LOCALE_AUDIOMENU_LIRC                      "audiomenu.lirc"
#define LOCALE_AUDIOMENU_MONOLEFT                  "audiomenu.monoleft"
#define LOCALE_AUDIOMENU_MONORIGHT                 "audiomenu.monoright"
#define LOCALE_AUDIOMENU_OST                       "audiomenu.ost"
#define LOCALE_AUDIOMENU_STEREO                    "audiomenu.stereo"
#define LOCALE_BOOKMARKMANAGER_DELETE              "bookmarkmanager.delete"
#define LOCALE_BOOKMARKMANAGER_NAME                "bookmarkmanager.name"
#define LOCALE_BOOKMARKMANAGER_RENAME              "bookmarkmanager.rename"
#define LOCALE_BOOKMARKMANAGER_SELECT              "bookmarkmanager.select"
#define LOCALE_BOUQUETEDITOR_ADD                   "bouqueteditor.add"
#define LOCALE_BOUQUETEDITOR_BOUQUETNAME           "bouqueteditor.bouquetname"
#define LOCALE_BOUQUETEDITOR_DELETE                "bouqueteditor.delete"
#define LOCALE_BOUQUETEDITOR_DISCARDINGCHANGES     "bouqueteditor.discardingchanges"
#define LOCALE_BOUQUETEDITOR_HIDE                  "bouqueteditor.hide"
#define LOCALE_BOUQUETEDITOR_LOCK                  "bouqueteditor.lock"
#define LOCALE_BOUQUETEDITOR_MOVE                  "bouqueteditor.move"
#define LOCALE_BOUQUETEDITOR_NAME                  "bouqueteditor.name"
#define LOCALE_BOUQUETEDITOR_NEWBOUQUETNAME        "bouqueteditor.newbouquetname"
#define LOCALE_BOUQUETEDITOR_RENAME                "bouqueteditor.rename"
#define LOCALE_BOUQUETEDITOR_RETURN                "bouqueteditor.return"
#define LOCALE_BOUQUETEDITOR_SAVECHANGES           "bouqueteditor.savechanges?"
#define LOCALE_BOUQUETEDITOR_SAVINGCHANGES         "bouqueteditor.savingchanges"
#define LOCALE_BOUQUETEDITOR_SWITCH                "bouqueteditor.switch"
#define LOCALE_BOUQUETEDITOR_SWITCHMODE            "bouqueteditor.switchmode"
#define LOCALE_BOUQUETLIST_HEAD                    "bouquetlist.head"
#define LOCALE_CABLESETUP_PROVIDER                 "cablesetup.provider"
//#define LOCALE_CAM_WRONG                           "cam.wrong"                                  /* FIXME: unused */
#define LOCALE_CHANNELLIST_HEAD                    "channellist.head"
#define LOCALE_CHANNELLIST_NONEFOUND               "channellist.nonefound"
#define LOCALE_CHANNELLIST_SINCE                   "channellist.since"
#define LOCALE_COLORCHOOSER_ALPHA                  "colorchooser.alpha"
#define LOCALE_COLORCHOOSER_BLUE                   "colorchooser.blue"
#define LOCALE_COLORCHOOSER_GREEN                  "colorchooser.green"
#define LOCALE_COLORCHOOSER_RED                    "colorchooser.red"
#define LOCALE_COLORMENU_BACKGROUND                "colormenu.background"
#define LOCALE_COLORMENU_BACKGROUND_HEAD           "colormenu.background_head"
#define LOCALE_COLORMENU_FADE                      "colormenu.fade"
#define LOCALE_COLORMENU_FONT                      "colormenu.font"
#define LOCALE_COLORMENU_GTX_ALPHA                 "colormenu.gtx_alpha"
#define LOCALE_COLORMENU_HEAD                      "colormenu.head"
#define LOCALE_COLORMENU_MENUCOLORS                "colormenu.menucolors"
#define LOCALE_COLORMENU_STATUSBAR                 "colormenu.statusbar"
#define LOCALE_COLORMENU_TEXTCOLOR                 "colormenu.textcolor"
#define LOCALE_COLORMENU_TEXTCOLOR_HEAD            "colormenu.textcolor_head"
#define LOCALE_COLORMENU_THEMESELECT               "colormenu.themeselect"
#define LOCALE_COLORMENU_TIMING                    "colormenu.timing"
#define LOCALE_COLORMENUSETUP_HEAD                 "colormenusetup.head"
#define LOCALE_COLORMENUSETUP_MENUCONTENT          "colormenusetup.menucontent"
#define LOCALE_COLORMENUSETUP_MENUCONTENT_INACTIVE "colormenusetup.menucontent_inactive"
#define LOCALE_COLORMENUSETUP_MENUCONTENT_SELECTED "colormenusetup.menucontent_selected"
#define LOCALE_COLORMENUSETUP_MENUHEAD             "colormenusetup.menuhead"
#define LOCALE_COLORSTATUSBAR_HEAD                 "colorstatusbar.head"
#define LOCALE_COLORSTATUSBAR_TEXT                 "colorstatusbar.text"
#define LOCALE_COLORTHEMEMENU_CLASSIC_THEME        "colorthememenu.classic_theme"
#define LOCALE_COLORTHEMEMENU_HEAD                 "colorthememenu.head"
#define LOCALE_COLORTHEMEMENU_NEUTRINO_THEME       "colorthememenu.neutrino_theme"
#define LOCALE_DATE_APR                            "date.Apr"
#define LOCALE_DATE_AUG                            "date.Aug"
#define LOCALE_DATE_DEC                            "date.Dec"
#define LOCALE_DATE_FEB                            "date.Feb"
#define LOCALE_DATE_FRI                            "date.Fri"
#define LOCALE_DATE_JAN                            "date.Jan"
#define LOCALE_DATE_JUL                            "date.Jul"
#define LOCALE_DATE_JUN                            "date.Jun"
#define LOCALE_DATE_MAR                            "date.Mar"
#define LOCALE_DATE_MAY                            "date.May"
#define LOCALE_DATE_MON                            "date.Mon"
#define LOCALE_DATE_NOV                            "date.Nov"
#define LOCALE_DATE_OCT                            "date.Oct"
#define LOCALE_DATE_SAT                            "date.Sat"
#define LOCALE_DATE_SEP                            "date.Sep"
#define LOCALE_DATE_SUN                            "date.Sun"
#define LOCALE_DATE_THU                            "date.Thu"
#define LOCALE_DATE_TUE                            "date.Tue"
#define LOCALE_DATE_WED                            "date.Wed"
#define LOCALE_EPGLIST_HEAD                        "epglist.head"
#define LOCALE_EPGLIST_NOEVENTS                    "epglist.noevents"
#define LOCALE_EPGVIEWER_MORE_SCREENINGS           "epgviewer.More_Screenings"
#define LOCALE_EPGVIEWER_NODETAILED                "epgviewer.nodetailed"
#define LOCALE_EPGVIEWER_NOTFOUND                  "epgviewer.notfound"
#define LOCALE_EVENTLISTBAR_CHANNELSWITCH          "eventlistbar.channelswitch"
#define LOCALE_EVENTLISTBAR_EVENTSORT              "eventlistbar.eventsort"
#define LOCALE_EVENTLISTBAR_RECORDEVENT            "eventlistbar.recordevent"
#define LOCALE_FAVORITES_ADDCHANNEL                "favorites.addchannel"
#define LOCALE_FAVORITES_BOUQUETNAME               "favorites.bouquetname"
#define LOCALE_FAVORITES_BQCREATED                 "favorites.bqcreated"
#define LOCALE_FAVORITES_CHADDED                   "favorites.chadded"
#define LOCALE_FAVORITES_CHALREADYINBQ             "favorites.chalreadyinbq"
#define LOCALE_FAVORITES_FINALHINT                 "favorites.finalhint"
#define LOCALE_FAVORITES_MENUEADD                  "favorites.menueadd"
#define LOCALE_FAVORITES_NOBOUQUETS                "favorites.nobouquets"


#define LOCALE_MOVIEPLAYER_BOOKMARK                "movieplayer.bookmark"
#define LOCALE_MOVIEPLAYER_BOOKMARKNAME            "movieplayer.bookmarkname"
#define LOCALE_MOVIEPLAYER_BOOKMARKNAME_HINT1      "movieplayer.bookmarkname_hint1"
#define LOCALE_MOVIEPLAYER_BOOKMARKNAME_HINT2      "movieplayer.bookmarkname_hint2"
#define LOCALE_MOVIEPLAYER_BUFFERING               "movieplayer.buffering"
//#define LOCALE_MOVIEPLAYER_CHOOSEPES               "movieplayer.choosepes"                      /* FIXME: unused */
//#define LOCALE_MOVIEPLAYER_CHOOSEPS                "movieplayer.chooseps"                       /* FIXME: unused */
//#define LOCALE_MOVIEPLAYER_CHOOSESTREAM            "movieplayer.choosestream"                   /* FIXME: unused */
//#define LOCALE_MOVIEPLAYER_CHOOSESTREAMDVD         "movieplayer.choosestreamdvd"                /* FIXME: unused */
//#define LOCALE_MOVIEPLAYER_CHOOSESTREAMFILE        "movieplayer.choosestreamfile"               /* FIXME: unused */
//#define LOCALE_MOVIEPLAYER_CHOOSESTREAMSVCD        "movieplayer.choosestreamsvcd"               /* FIXME: unused */
//#define LOCALE_MOVIEPLAYER_CHOOSETS                "movieplayer.choosets"                       /* FIXME: unused */
#define LOCALE_MOVIEPLAYER_DEFDIR                  "movieplayer.defdir"
#define LOCALE_MOVIEPLAYER_DVDPLAYBACK             "movieplayer.dvdplayback"
#define LOCALE_MOVIEPLAYER_FILEPLAYBACK            "movieplayer.fileplayback"
#define LOCALE_MOVIEPLAYER_GOTO                    "movieplayer.goto"
#define LOCALE_MOVIEPLAYER_GOTO_H1                 "movieplayer.goto.h1"
#define LOCALE_MOVIEPLAYER_GOTO_H2                 "movieplayer.goto.h2"
#define LOCALE_MOVIEPLAYER_HEAD                    "movieplayer.head"
//#define LOCALE_MOVIEPLAYER_NAME                    "movieplayer.name"                           /* FIXME: unused */
#define LOCALE_MOVIEPLAYER_NOSTREAMINGSERVER       "movieplayer.nostreamingserver"
#define LOCALE_MOVIEPLAYER_PESPLAYBACK             "movieplayer.pesplayback"
//#define LOCALE_MOVIEPLAYER_PLAY                    "movieplayer.play"                           /* FIXME: unused */
#define LOCALE_MOVIEPLAYER_PLEASEWAIT              "movieplayer.pleasewait"
#define LOCALE_MOVIEPLAYER_TOOMANYBOOKMARKS        "movieplayer.toomanybookmarks"
#define LOCALE_MOVIEPLAYER_TSHELP                  "movieplayer.tshelp"
#define LOCALE_MOVIEPLAYER_TSPLAYBACK              "movieplayer.tsplayback"
#define LOCALE_MOVIEPLAYER_VCDPLAYBACK             "movieplayer.vcdplayback"
#define LOCALE_MOVIEPLAYER_VLCHELP                 "movieplayer.vlchelp"
#define LOCALE_MOVIEPLAYER_WRONGVLCVERSION         "movieplayer.wrongvlcversion"


#define LOCALE_TIMING_CHANLIST                     "timing.chanlist"
#define LOCALE_TIMING_EPG                          "timing.epg"
#define LOCALE_TIMING_FILEBROWSER                  "timing.filebrowser"
#define LOCALE_TIMING_HEAD                         "timing.head"
#define LOCALE_TIMING_HINT_1                       "timing.hint_1"
#define LOCALE_TIMING_HINT_2                       "timing.hint_2"
#define LOCALE_TIMING_INFOBAR                      "timing.infobar"
#define LOCALE_TIMING_MENU                         "timing.menu"



/* #warning fix usage of locales in system/setting_helpers.cpp */

#endif
