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

#define ARE_LOCALES_EQUAL(a,b) (strcmp(a,b) == 0)

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
#define LOCALE_FILEBROWSER_DELETE                  "filebrowser.delete"
#define LOCALE_FILEBROWSER_DODELETE1               "filebrowser.dodelete1"
#define LOCALE_FILEBROWSER_DODELETE2               "filebrowser.dodelete2"
#define LOCALE_FILEBROWSER_FILTER_ACTIVE           "filebrowser.filter.active"
#define LOCALE_FILEBROWSER_FILTER_INACTIVE         "filebrowser.filter.inactive"
#define LOCALE_FILEBROWSER_HEAD                    "filebrowser.head"
#define LOCALE_FILEBROWSER_MARK                    "filebrowser.mark"
#define LOCALE_FILEBROWSER_NEXTPAGE                "filebrowser.nextpage"
#define LOCALE_FILEBROWSER_PREVPAGE                "filebrowser.prevpage"
#define LOCALE_FILEBROWSER_SCAN                    "filebrowser.scan"
#define LOCALE_FILEBROWSER_SELECT                  "filebrowser.select"
#define LOCALE_FILEBROWSER_SHOWRIGHTS              "filebrowser.showrights"
#define LOCALE_FILEBROWSER_SORT_DATE               "filebrowser.sort.date"
#define LOCALE_FILEBROWSER_SORT_NAME               "filebrowser.sort.name"
#define LOCALE_FILEBROWSER_SORT_NAMEDIRSFIRST      "filebrowser.sort.namedirsfirst"
#define LOCALE_FILEBROWSER_SORT_SIZE               "filebrowser.sort.size"
#define LOCALE_FILEBROWSER_SORT_TYPE               "filebrowser.sort.type"


#define LOCALE_IPSETUP_HINT_1                      "ipsetup.hint_1"
#define LOCALE_IPSETUP_HINT_2                      "ipsetup.hint_2"


#define LOCALE_LANGUAGESETUP_HEAD                  "languagesetup.head"
#define LOCALE_LANGUAGESETUP_SELECT                "languagesetup.select"
#define LOCALE_LCDCONTROLER_BRIGHTNESS             "lcdcontroler.brightness"
#define LOCALE_LCDCONTROLER_BRIGHTNESSSTANDBY      "lcdcontroler.brightnessstandby"
#define LOCALE_LCDCONTROLER_CONTRAST               "lcdcontroler.contrast"
#define LOCALE_LCDCONTROLER_HEAD                   "lcdcontroler.head"
#define LOCALE_LCDMENU_AUTODIMM                    "lcdmenu.autodimm"
#define LOCALE_LCDMENU_HEAD                        "lcdmenu.head"
#define LOCALE_LCDMENU_INVERSE                     "lcdmenu.inverse"
#define LOCALE_LCDMENU_LCDCONTROLER                "lcdmenu.lcdcontroler"
#define LOCALE_LCDMENU_POWER                       "lcdmenu.power"
#define LOCALE_LCDMENU_STATUSLINE                  "lcdmenu.statusline"
#define LOCALE_LCDMENU_STATUSLINE_BOTH             "lcdmenu.statusline.both"
#define LOCALE_LCDMENU_STATUSLINE_PLAYTIME         "lcdmenu.statusline.playtime"
#define LOCALE_LCDMENU_STATUSLINE_VOLUME           "lcdmenu.statusline.volume"
#define LOCALE_MAINMENU_GAMES                      "mainmenu.games"
#define LOCALE_MAINMENU_HEAD                       "mainmenu.head"
#define LOCALE_MAINMENU_MOVIEPLAYER                "mainmenu.movieplayer"
#define LOCALE_MAINMENU_MP3PLAYER                  "mainmenu.mp3player"
#define LOCALE_MAINMENU_PAUSESECTIONSD             "mainmenu.pausesectionsd"
#define LOCALE_MAINMENU_PICTUREVIEWER              "mainmenu.pictureviewer"
#define LOCALE_MAINMENU_RADIOMODE                  "mainmenu.radiomode"
#define LOCALE_MAINMENU_RECORDING                  "mainmenu.recording"
#define LOCALE_MAINMENU_RECORDING_START            "mainmenu.recording_start"
#define LOCALE_MAINMENU_RECORDING_STOP             "mainmenu.recording_stop"
#define LOCALE_MAINMENU_SCARTMODE                  "mainmenu.scartmode"
#define LOCALE_MAINMENU_SERVICE                    "mainmenu.service"
#define LOCALE_MAINMENU_SETTINGS                   "mainmenu.settings"
#define LOCALE_MAINMENU_SHUTDOWN                   "mainmenu.shutdown"
#define LOCALE_MAINMENU_SLEEPTIMER                 "mainmenu.sleeptimer"
#define LOCALE_MAINMENU_TVMODE                     "mainmenu.tvmode"
#define LOCALE_MAINSETTINGS_AUDIO                  "mainsettings.audio"
#define LOCALE_MAINSETTINGS_COLORS                 "mainsettings.colors"
#define LOCALE_MAINSETTINGS_HEAD                   "mainsettings.head"
#define LOCALE_MAINSETTINGS_KEYBINDING             "mainsettings.keybinding"
#define LOCALE_MAINSETTINGS_LANGUAGE               "mainsettings.language"
#define LOCALE_MAINSETTINGS_LCD                    "mainsettings.lcd"
#define LOCALE_MAINSETTINGS_MISC                   "mainsettings.misc"
#define LOCALE_MAINSETTINGS_NETWORK                "mainsettings.network"
//#define LOCALE_MAINSETTINGS_PLUGINS                "mainsettings.plugins"                       /* FIXME: unused */
#define LOCALE_MAINSETTINGS_RECORDING              "mainsettings.recording"
#define LOCALE_MAINSETTINGS_SAVESETTINGSNOW        "mainsettings.savesettingsnow"
#define LOCALE_MAINSETTINGS_SAVESETTINGSNOW_HINT   "mainsettings.savesettingsnow_hint"
//#define LOCALE_MAINSETTINGS_SCAN                   "mainsettings.scan"                          /* FIXME: unused */
#define LOCALE_MAINSETTINGS_STREAMING              "mainsettings.streaming"
#define LOCALE_MAINSETTINGS_VIDEO                  "mainsettings.video"
#define LOCALE_MENU_BACK                           "menu.back"
#define LOCALE_MESSAGEBOX_BACK                     "messagebox.back"
#define LOCALE_MESSAGEBOX_CANCEL                   "messagebox.cancel"
#define LOCALE_MESSAGEBOX_DISCARD                  "messagebox.discard"
#define LOCALE_MESSAGEBOX_ERROR                    "messagebox.error"
#define LOCALE_MESSAGEBOX_INFO                     "messagebox.info"
#define LOCALE_MESSAGEBOX_NO                       "messagebox.no"
#define LOCALE_MESSAGEBOX_YES                      "messagebox.yes"
#define LOCALE_MISCSETTINGS_BOOTINFO               "miscsettings.bootinfo"
//#define LOCALE_MISCSETTINGS_BOXTYPE                "miscsettings.boxtype"                       /* FIXME: unused */
#define LOCALE_MISCSETTINGS_DRIVER_BOOT            "miscsettings.driver_boot"
#define LOCALE_MISCSETTINGS_FB_DESTINATION         "miscsettings.fb_destination"
#define LOCALE_MISCSETTINGS_GENERAL                "miscsettings.general"
#define LOCALE_MISCSETTINGS_HEAD                   "miscsettings.head"
#define LOCALE_MISCSETTINGS_HWSECTIONS             "miscsettings.hwsections"
#define LOCALE_MISCSETTINGS_INFOBAR_SAT_DISPLAY    "miscsettings.infobar_sat_display"
#define LOCALE_MISCSETTINGS_SHUTDOWN_REAL          "miscsettings.shutdown_real"
#define LOCALE_MISCSETTINGS_SHUTDOWN_REAL_RCDELAY  "miscsettings.shutdown_real_rcdelay"
#define LOCALE_MISCSETTINGS_SPTSMODE               "miscsettings.sptsmode"
#define LOCALE_MISCSETTINGS_STARTBHDRIVER          "miscsettings.startbhdriver"                   /* only for HAVE_DVB_API_VERSION == 1 */
#define LOCALE_MOTORCONTROL_HEAD                   "motorcontrol.head"
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


#define LOCALE_NETWORKMENU_BROADCAST               "networkmenu.broadcast"
#define LOCALE_NETWORKMENU_GATEWAY                 "networkmenu.gateway"
#define LOCALE_NETWORKMENU_HEAD                    "networkmenu.head"
#define LOCALE_NETWORKMENU_IPADDRESS               "networkmenu.ipaddress"
#define LOCALE_NETWORKMENU_NAMESERVER              "networkmenu.nameserver"
#define LOCALE_NETWORKMENU_NETMASK                 "networkmenu.netmask"
#define LOCALE_NETWORKMENU_SETUPNOW                "networkmenu.setupnow"
#define LOCALE_NETWORKMENU_SETUPONSTARTUP          "networkmenu.setuponstartup"
#define LOCALE_NETWORKMENU_SHOW                    "networkmenu.show"
#define LOCALE_NETWORKMENU_TEST                    "networkmenu.test"
#define LOCALE_NFS_ALREADYMOUNTED                  "nfs.alreadymounted"
#define LOCALE_NFS_AUTOMOUNT                       "nfs.automount"
#define LOCALE_NFS_DIR                             "nfs.dir"
#define LOCALE_NFS_IP                              "nfs.ip"
#define LOCALE_NFS_LOCALEDIR                       "nfs.localdir"
#define LOCALE_NFS_MOUNT                           "nfs.mount"
#define LOCALE_NFS_MOUNT_OPTIONS                   "nfs.mount_options"
#define LOCALE_NFS_MOUNTERROR                      "nfs.mounterror"
#define LOCALE_NFS_MOUNTERROR_NOTSUP               "nfs.mounterror_notsup"
#define LOCALE_NFS_MOUNTNOW                        "nfs.mountnow"
#define LOCALE_NFS_MOUNTTIMEOUT                    "nfs.mounttimeout"
#define LOCALE_NFS_PASSWORD                        "nfs.password"
#define LOCALE_NFS_REMOUNT                         "nfs.remount"
#define LOCALE_NFS_TYPE                            "nfs.type"
#define LOCALE_NFS_TYPE_CIFS                       "nfs.type_cifs"
#define LOCALE_NFS_TYPE_NFS                        "nfs.type_nfs"
#define LOCALE_NFS_UMOUNT                          "nfs.umount"
#define LOCALE_NFS_UMOUNTERROR                     "nfs.umounterror"
#define LOCALE_NFS_USERNAME                        "nfs.username"
#define LOCALE_NFSMENU_HEAD                        "nfsmenu.head"
#define LOCALE_NVOD_PERCENTAGE                     "nvod.proz"
#define LOCALE_NVOD_STARTING                       "nvod.starting"
#define LOCALE_NVODSELECTOR_DIRECTORMODE           "nvodselector.directormode"
#define LOCALE_NVODSELECTOR_HEAD                   "nvodselector.head"
#define LOCALE_NVODSELECTOR_SUBSERVICE             "nvodselector.subservice"
#define LOCALE_OPTIONS_DEFAULT                     "options.default"
#define LOCALE_OPTIONS_FB                          "options.fb"
#define LOCALE_OPTIONS_NULL                        "options.null"
#define LOCALE_OPTIONS_OFF                         "options.off"
#define LOCALE_OPTIONS_ON                          "options.on"
#define LOCALE_OPTIONS_SERIAL                      "options.serial"


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
