AC_INIT(tuxbox-enigma,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-enigma,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AM_PROG_LIBTOOL

TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER

TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)
TUXBOX_APPS_LIB_PKGCONFIG(PLUGINS,tuxbox-plugins)
TUXBOX_APPS_LIB_PKGCONFIG(PNG,libpng)
TUXBOX_APPS_LIB_PKGCONFIG(SIGC,sigc++-1.2)
TUXBOX_APPS_LIB_PKGCONFIG(XMLTREE,tuxbox-xmltree)

CPPFLAGS="$CPPFLAGS $FREETYPE_CFLAGS $PLUGINS_CFLAGS $SIGC_CFLAGS $XMLTREE_CFLAGS -fno-exceptions -fno-rtti"

TUXBOX_APPS_GETTEXT

AC_OUTPUT([
Makefile
data/Makefile
data/fonts/Makefile
data/htdocs/Makefile
data/pictures/Makefile
data/pictures/chk/Makefile
data/pictures/chk_v2/Makefile
data/pictures/decoration/Makefile
data/pictures/mainmenu/Makefile
data/resources/Makefile
data/skins/Makefile
data/templates/Makefile
doc/Makefile
include/Makefile
include/lib/Makefile
include/lib/base/Makefile
include/lib/driver/Makefile
include/lib/dvb/Makefile
include/lib/gdi/Makefile
include/lib/gui/Makefile
include/lib/socket/Makefile
include/lib/system/Makefile
lib/Makefile
lib/base/Makefile
lib/driver/Makefile
lib/dvb/Makefile
lib/dvb/lowlevel/Makefile
lib/gdi/Makefile
lib/gui/Makefile
lib/socket/Makefile
lib/system/Makefile
po/Makefile
src/Makefile
])

