AC_INIT(tuxbox-apps-dvb-zapit,0.0.1,,zapit)
AM_INIT_AUTOMAKE(zapit,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

TUXBOX_APPS_DVB
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
TUXBOX_APPS_LIB_PKGCONFIG(EVENTSERVER,tuxbox-eventserver)
CPPFLAGS="$CPPFLAGS $EVENTSERVER_CFLAGS"
LDFLAGS="$LDFLAGS $EVENTSERVER_LIBS"

AC_OUTPUT([
Makefile
lib/Makefile
src/Makefile
src/xml/Makefile
src/zapci/Makefile
src/zapost/Makefile
src/zapsi/Makefile
zapitclient.pc
])

