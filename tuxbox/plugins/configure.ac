AC_INIT(tuxbox-plugins,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-plugins,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER
TUXBOX_APPS_CAPTURE

TUXBOX_BOXTYPE

# needed by enigma/movieplayer
TUXBOX_APPS_LIB_CONFIG(CURL,curl-config)

# needed by tuxmail
TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)
#TUXBOX_APPS_LIB_PKGCONFIG(PNG,libpng)

# needed by enigma-demo
TUXBOX_APPS_LIB_PKGCONFIG(SIGC,sigc++-1.2)
if test "$BOXTYPE" = "dbox2"; then
TUXBOX_APPS_LIB_PKGCONFIG(TUXBOX,tuxbox)
fi

# needed by enigma-demo
TUXBOX_APPS_LIB_PKGCONFIG(XMLTREE,tuxbox-xmltree)

# needed by vncviewer
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)

# needed by tuxtxt
TUXBOX_APPS_LIB_PKGCONFIG(TUXTXT,tuxbox-tuxtxt)

CPPFLAGS="$CPPFLAGS $TUXTXT_CFLAGS"
CFLAGS="$CFLAGS $TUXTXT_CFLAGS"

AC_OUTPUT([
Makefile
include/Makefile
fx2/Makefile
fx2/lemm/Makefile
fx2/master/Makefile
fx2/pac/Makefile
fx2/tank/Makefile
fx2/yahtzee/Makefile
fx2/bouquet/Makefile
fx2/lib/Makefile
fx2/mines/Makefile
fx2/outdoor/Makefile
fx2/snake/Makefile
fx2/sokoban/Makefile
fx2/solitair/Makefile
fx2/tetris/Makefile
fx2/lcdcirc/Makefile
fx2/satfind/Makefile
fx2/vierg/Makefile
fx2/sol/Makefile
tuxmail/Makefile
tuxmail/daemon/Makefile
tuxcal/Makefile
tuxcal/daemon/Makefile
tuxtxt/Makefile
tuxtxt/font/Makefile
tuxcom/Makefile
tuxclock/Makefile
tuxclock/daemon/Makefile
vncviewer/Makefile
vncviewer/rfb/Makefile
dvbsub/Makefile
pip/Makefile
mosaic/Makefile
enigma/Makefile
enigma/bitrate/Makefile
enigma/demo/Makefile
enigma/dreamdata/Makefile
enigma/ngrabstart/Makefile
enigma/ngrabstop/Makefile
enigma/dslconnect/Makefile
enigma/dsldisconnect/Makefile
enigma/rss/Makefile
enigma/script/Makefile
enigma/getset/Makefile
enigma/dbswitch/Makefile
enigma/weather/Makefile
enigma/weather/icons/Makefile
enigma/dreamflash/Makefile
enigma/movieplayer/Makefile
enigma/scummvm/Makefile
pluginx/Makefile
tuxbox-plugins.pc
])

