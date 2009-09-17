/**
  tuxclockd
  =========
  Version: 0.03 by Blazej Bartyzel "blesb", at 2009-05-27
  Print page format: landscape
  program construct is based on TuxCal daemon rev 1.10, Robert "robspr1" Spreitzer 2006

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

#define SCKFILE "/tmp/tuxclockd.sock"                            // socket-file, connection to daemon
#define PIDFILE "/tmp/tuxclockd.pid"                             // PID file
#define CFGPATH "/var/tuxbox/config/"                            // config-path
#define CFGFILE "tuxclock.conf"                                  // config-file

// defines for setting the output
#define MAXCLKLEN 10                                             // "HH:MM:SS" mind. 8 char plus "\0"

// definitions for string-rendering and size
char *clkfmt[] = {
   "%H:%M", "%H:%M:%S"
};

#define FONT FONTDIR "/pakenham.ttf"
FT_Library        library;
FTC_Manager       manager;
FTC_SBitCache     cache;
FTC_SBit          sbit;
#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
FTC_ImageDesc     desc;
#else
FTC_ImageTypeRec  desc;
#endif
FT_Face           face;
FT_UInt           prev_glyphindex;
FT_Bool           use_kerning;

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
#define SHOWDATENORM     SHOWDATEON

// config parameter, all set as uninitialised
int show_clock =        -1;                                      // 1: enable, 0: disable
int startdelay =        -1;                                      // in sec: wait time before first loop
int startx =            -1;                                      // X position of clock string
int starty =            -1;                                      // Y position of clock string
int intervall =         -1;                                      // after <interval> sec repaint clock
int char_size =         -1;                                      // character size
int char_color =        -1;                                      // character color
int char_bgcolor =      -2;                                      // -1 not use BGColor in ShowClock
int disp_format =       -1;                                      // 1: "HH:MM:SS", 0: "HH:MM"
int fb_color_set =      -1;                                      // Predef: white and black not set
int print_colortab =     0;                                      // switch for printf of colortab
int disp_date =         -1;                                      // future switch
int clksize;                                                     // work string size 00:00[:00] for backgroud paint
char info[MAXCLKLEN];                                            // std. string
// variables from tuxcal
int iFB =                0;                                      // is the framebuffer initialized?
int fbdev;                                                       // Frame Buffer device
int slog =               0;                                      // 1: logging to syslog, 0: logging to console
int pid;                                                         // the pid number
int sock;                                                        // socket
int intervall;                                                   // repaint every x seconds
FILE *fd_pid;                                                    // PID file
struct tm *at;                                                   // for actual time
time_t tt;
char versioninfo_d[12] = "?.??";                                 // daemon version
unsigned char *lfb =     0;                                      // pointer to FB
struct fb_var_screeninfo var_screeninfo;                         // frame buffer screen parameter
struct fb_fix_screeninfo fix_screeninfo;

char *loginfos[] = {
   "daemon start",                                                // 0
   "daemon terminate",                                            // 1
   "kill-sig: set clock visible",                                 // 2
   "kill-sig: set clock unvisible",                               // 3
};
