/**
  tuxclock
  =========
  Version: 0.01 alpha by Blazej Bartyzel "blesb", at 2009-05-28
  Print page format: landscape
  program construct is based on TuxMail vom Thomas "LazyT" 2005

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
  If this software will be used in commertial products and source code are
  not included or not offer for customer, please contact FSF or me over
  forum http://tuxbox-forum.dreambox-fan.de/forum/.

*/

#include "config.h"
#if !defined(HAVE_DVB_API_VERSION) && defined(HAVE_OST_DMX_H)
#define HAVE_DVB_API_VERSION 1
#endif
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <time.h>
#include <syslog.h>
#include <linux/fb.h>
#include <zlib.h>
#include <malloc.h>
#if HAVE_DVB_API_VERSION == 3
#include <linux/input.h>
#endif
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H
#include <plugin.h>

#define SCKFILE "/tmp/tuxclockd.sock"                            // socket-file, connection to daemon
#define CFGPATH "/var/tuxbox/config/"                            // config-path
#define CFGFILE "tuxclock.conf"                                  // config-file

#if HAVE_DVB_API_VERSION == 1
#define   RC1_0       0x5C00
#define   RC1_1       0x5C01
#define   RC1_2       0x5C02
#define   RC1_3       0x5C03
#define   RC1_4       0x5C04
#define   RC1_5       0x5C05
#define   RC1_6       0x5C06
#define   RC1_7       0x5C07
#define   RC1_8       0x5C08
#define   RC1_9       0x5C09
#define   RC1_STANDBY 0x5C0C
#define   RC1_UP      0x5C0E
#define   RC1_DOWN    0x5C0F
#define   RC1_PLUS    0x5C16
#define   RC1_MINUS   0x5C17
#define   RC1_HOME    0x5C20
#define   RC1_DBOX    0x5C27
#define   RC1_MUTE    0x5C28
#define   RC1_RED     0x5C2D
#define   RC1_RIGHT   0x5C2E
#define   RC1_LEFT    0x5C2F
#define   RC1_OK      0x5C30
#define   RC1_BLUE    0x5C3B
#define   RC1_YELLOW  0x5C52
#define   RC1_GREEN   0x5C55
#define   RC1_HELP    0x5C82
#endif
#define   RC_0        0x00
#define   RC_1        0x01
#define   RC_2        0x02
#define   RC_3        0x03
#define   RC_4        0x04
#define   RC_5        0x05
#define   RC_6        0x06
#define   RC_7        0x07
#define   RC_8        0x08
#define   RC_9        0x09
#define   RC_RIGHT    0x0A
#define   RC_LEFT     0x0B
#define   RC_UP       0x0C
#define   RC_DOWN     0x0D
#define   RC_OK       0x0E
#define   RC_MUTE     0x0F
#define   RC_STANDBY  0x10
#define   RC_GREEN    0x11
#define   RC_YELLOW   0x12
#define   RC_RED      0x13
#define   RC_BLUE     0x14
#define   RC_PLUS     0x15
#define   RC_MINUS    0x16
#define   RC_HELP     0x17
#define   RC_DBOX     0x18
#define   RC_HOME     0x1F
#define   FONT FONTDIR "/pakenham.ttf"

FT_Library       library;
FTC_Manager      manager;
FTC_SBitCache    cache;
FTC_SBit         sbit;
#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
FTC_ImageDesc    desc;
#else
FTC_ImageTypeRec desc;
#endif
FT_Face          face;
FT_UInt          prev_glyphindex;
FT_Bool          use_kerning;

int skin  = 1;
int fb, rc;
unsigned char *lfb = 0, *lbb = 0;
struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;

// string allign mode
enum {LEFT, RIGHT};
// box paint mode
enum {FILL, GRID};
// colortab
enum {TRANSP, WHITE, SKIN0, SKIN1, SKIN2, ORANGE, GREEN, YELLOW, RED};
// daemon commands
enum {GET_STATUS, GET_ID, SET_HIDDEN, SET_VISIBLE, SET_RESTART};
// daemon status
enum {DAEMON_OFF, DAEMON_ON};

unsigned short rd1[] = {0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0xFF<<8};
unsigned short gn1[] = {0xFF<<8, 0x00<<8, 0x40<<8, 0x80<<8, 0xC0<<8, 0xFF<<8, 0xFF<<8, 0x00<<8};
unsigned short bl1[] = {0xFF<<8, 0x80<<8, 0x80<<8, 0xFF<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short tr1[] = {0x0000,  0x0A00,  0x0A00,  0x0A00,  0x0000,  0x0000,  0x0000,  0x0000 };

unsigned short rd2[] = {0xFF<<8, 0x25<<8, 0x4A<<8, 0x97<<8, 0xFF<<8, 0x00<<8, 0xFF<<8, 0xFF<<8};
unsigned short gn2[] = {0xFF<<8, 0x3A<<8, 0x63<<8, 0xAC<<8, 0xC0<<8, 0xFF<<8, 0xFF<<8, 0x00<<8};
unsigned short bl2[] = {0xFF<<8, 0x4D<<8, 0x77<<8, 0xC1<<8, 0x00<<8, 0x00<<8, 0x00<<8, 0x00<<8};
unsigned short tr2[] = {0x0000,  0x0A00,  0x0A00,  0x0A00,  0x0000,  0x0000,  0x0000,  0x0000 };

struct fb_cmap colormap1 = {1, 8, rd1, gn1, bl1, tr1};
struct fb_cmap colormap2 = {1, 8, rd2, gn2, bl2, tr2};

int startx, starty, sx, ex, sy, ey;

#if HAVE_DVB_API_VERSION == 3
struct input_event ev;
#endif

unsigned short rccode;

// Range parameter for config paramter variables
#define FBMAXX         720
#define FBMAXY         576
// --
#define SHOWCLOCKON      1
#define SHOWCLOCKOFF     0
#define SHOWCLOCKNORM    SHOWCLOCKON
// --
#define STARTDELAYMIN    1
#define STARTDELAYMAX  120
#define STARTDELAYNORM  45
// --
#define STARTXMIN        0
#define STARTXMAX      720
#define STARTXNORM     580
// --
#define STARTYMIN        0
#define STARTYMAX      576
#define STARTYNORM      30
// --
#define INTERVALLMIN     1
#define INTERVALLMAX    60
#define INTERVALLNORM    1
// --
#define CHARSIZEMIN     12
#define CHARSIZEMAX     60
#define CHARSIZENORM    28
// --
#define CHARCOLORMIN     0
#define CHARCOLORMAX   255
// --
#define CHARBGCOLORMIN  -1
#define CHARBGCOLORMAX 255
// --
#define CHAR_TRANSP    255                                       // transparency (i hope it)
// --
#define FORMATHHMM       0
#define FORMATHHMMSS     1
#define FORMATNORM       FORMATHHMM
// --
#define SHOWDATEON       1                                       // for screensaver future function
#define SHOWDATEOFF      0
#define SHOWDATENORM     SHOWDATEOFF

#define TCCHARSIZE      24
#define TCROWSIZE       28

// config parameter, all set as uninitialised
int show_clock =        -1;                                      // 1: enable, 0: disable
int startdelay =        -1;                                      // in sec: wait time before first loop
int start_x =           -1;                                      // X position of clock string
int start_y =           -1;                                      // Y position of clock string
int intervall =         -1;                                      // after <interval> sec repaint clock
int char_size =         -1;                                      // character size
int char_color =        -1;                                      // character color
int char_bgcolor =      -2;                                      // -1 not use BGColor in ShowClock
int disp_format =       -1;                                      // 1: "HH:MM:SS", 0: "HH:MM"
int fb_color_set =      -1;                                      // Predef: white and black not set
int disp_date =         -1;                                      // future switch
int visible =            0;                                      // ret val from daemon: is d. visible?
int socket_status =      DAEMON_ON;                              // Status of communication to daemon
int lang =               1;                                      // 0=eng, 1=ger

// variables form tux[mail|cal]
int fbdev;                                                       // Frame Buffer device
int slog =               0;                                      // 1: logging to syslog, 0: logging to console
int sock;                                                        // socket
char versioninfo_d[12] = "?.??";                                 // daemon version

#define MAXOSD 2 // language: 0=eng, 1=ger

char *mrows[][MAXOSD] = {
   { "tuxclock Control",       "Steuerung von tuxclock" },       //  0
   { "Show clock",             "Uhr anzeigen" },                 //  1
   { "X clock position",       "X-Position" },                   //  2
   { "Y clock position",       "Y-Position" },                   //  3
   { "Character size",         "Schriftgrad" },                  //  4
   { "Char color",             "Schriftfarbe" },                 //  5
   { "BGround color",          "Hintergrundfarbe" },             //  6
   { "Clock type",             "Uhrtyp" },                       //  7
   { "Screesaver with date",   "Bildschirmschoner mit Datum" },  //  8
   { "Screensaver start now",  "Bildschirmschoner starten" },    //  9
   { "[OK] select  [DBOX] save  [HOME] exit", " " }              // 10
};

char *onoff[][MAXOSD] = {
   { "OFF", "AUS" },                                             // 0
   { "ON",  "EIN" }                                              // 1
};

char *yesno[][MAXOSD] = {
   { "NO",  "NEIN" },                                            // 0
   { "YES", "JA" }                                               // 1
};

char *timefmt[] = {
   "HH:MM",                                                      // 0
   "HH:MM:SS"                                                    // 1
};

char *errormsg[] = {
   "TuxClock Plugin is started",                                 // 00
   "parameter missing",                                          // 01
   "FBIOGET_FSCREENINFO init error",                             // 02
   "FBIOGET_VSCREENINFO init error",                             // 03
   "mapping of FB failed",                                       // 04
   "FT_Init_FreeType failed",                                    // 05
   "FTC_Manager_New failed",                                     // 06
   "FTC_SBitCache_New failed",                                   // 07
   "FTC_Manager_Lookup_Face failed",                             // 08
   "config not found, using defaults",                           // 09
   "config file write error",                                    // 10
   "font load is failed",                                        // 11
   "FT_Get_Char_Index undefined char",                           // 12
   "FTC_SBitCache_Lookup failed",                                // 13
   "FBIOPUTCMAP failed",                                         // 14
   "allocating of Backbuffer failed",                            // 15
   "socket error: create failed",                                // 16
   "socket error: connect failed"                                // 17
};

char *deamonmsg[][MAXOSD] = {
   { "hidden",            "versteckt" },                         //  0
   { "visible",           "sichtbar" },                          //  1
   { "deamon not active", "Deamon ist inaktiv" }                 //  2
};
