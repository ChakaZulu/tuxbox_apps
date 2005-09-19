AC_INIT(tuxbox-plugins,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-plugins,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_ARG_WITH(boxtype,
	[  --with-boxtype=NAME dreambox/dbox2 [[dbox2,dreambox]]],
	[BOXTYPE="$withval"],[BOXTYPE="dbox2"])
AC_SUBST(BOXTYPE)
AM_CONDITIONAL(BOXTYPE_DREAMBOX,test "$BOXTYPE" != "dbox2")
AM_CONDITIONAL(BOXTYPE_DBOX2,test "$BOXTYPE" = "dbox2")

TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER
TUXBOX_APPS_CAPTURE

TUXBOX_APPS_LIB_CONFIG(CURL,curl-config)
TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)
TUXBOX_APPS_LIB_PKGCONFIG(PNG,libpng)
TUXBOX_APPS_LIB_PKGCONFIG(SIGC,sigc++-1.2)
if test "$BOXTYPE" = "dbox2"; then
TUXBOX_APPS_LIB_PKGCONFIG(TUXBOX,tuxbox)
fi
TUXBOX_APPS_LIB_PKGCONFIG(XMLTREE,tuxbox-xmltree)
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
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
tuxtxt/Makefile
tuxtxt/font/Makefile
tuxcom/Makefile
vncviewer/Makefile
vncviewer/rfb/Makefile
enigma/Makefile
enigma/demo/Makefile
enigma/dreamdata/Makefile
enigma/ngrabstart/Makefile
enigma/ngrabstop/Makefile
enigma/dslconnect/Makefile
enigma/dsldisconnect/Makefile
enigma/script/Makefile
tuxbox-plugins.pc
])

