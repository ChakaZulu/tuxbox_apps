AC_INIT(tuxbox-zapit,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-zapit,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

TUXBOX_APPS_DVB
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
TUXBOX_APPS_LIB_PKGCONFIG(CONNECTION,tuxbox-connection)
TUXBOX_APPS_LIB_PKGCONFIG(EVENTSERVER,tuxbox-eventserver)
TUXBOX_APPS_LIB_PKGCONFIG(XMLTREE,tuxbox-xmltree)
CPPFLAGS="$CPPFLAGS $EVENTSERVER_CFLAGS"
LDFLAGS="$LDFLAGS $EVENTSERVER_LIBS"

AC_LANG(C++)

AC_ARG_ENABLE(pid-cache,
	[  --enable-pid-cache      do not read pids after every channel change],
	[AC_DEFINE(USE_PID_CACHE,1,[do not read pids after every channel change])])

AC_OUTPUT([
Makefile
include/Makefile
include/zapit/Makefile
include/zapit/client/Makefile
lib/Makefile
src/Makefile
src/zapci/Makefile
src/zapost/Makefile
src/zapsi/Makefile
tuxbox-zapit.pc
])

