AC_INIT(tuxbox-zapit,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-zapit,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

TUXBOX_APPS_DVB
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
TUXBOX_APPS_LIB_PKGCONFIG(EVENTSERVER,tuxbox-eventserver)
TUXBOX_APPS_LIB_PKGCONFIG(XMLTREE,tuxbox-xmltree)
CPPFLAGS="$CPPFLAGS $EVENTSERVER_CFLAGS"
LDFLAGS="$LDFLAGS $EVENTSERVER_LIBS"

AC_LANG(C++)

AC_ARG_ENABLE(pid-cache,
	[  --enable-pid-cache      do not read pids after every channel change],
	[AC_DEFINE(USE_PID_CACHE,1,[do not read pids after every channel change])])

AC_ARG_ENABLE(tunning-retries,
	[  --enable-tuning-retries  retry tuning on failure],
	[AC_DEFINE(EXCESSIVE_TUNING_RETRIES,1,[retry tuning on failure])])

AC_ARG_WITH(neutrino-source,
	[  --with-neutrino-source=PATH  where to find the neutrino sources (workaround for circular reference)],
	[NEUTRINO_SOURCE="$withval"])

if test $NEUTRINO_SOURCE; then
	orig_CFLAGS=$CFLAGS
	orig_CPPFLAGS=$CPPFLAGS
        CFLAGS="-I$NEUTRINO_SOURCE/libs/liblcddclient"
        CPPFLAGS="-I$NEUTRINO_SOURCE/libs/liblcddclient"
	AC_CHECK_HEADERS(lcddclient.h,[LCDDCLIENT_CFLAGS=$CFLAGS])
	LCDDCLIENT_SOURCE=$NEUTRINO_SOURCE/libs/liblcddclient/lcddclient.cpp
	CFLAGS="$orig_CFLAGS"
	CPPFLAGS="$orig_CPPFLAGS"
fi

if test $LCDDCLIENT_CFLAGS; then
	AC_DEFINE(DBOX2,1,[enable dbox2 specific code])
fi

AM_CONDITIONAL(DBOX2,test $LCDDCLIENT_CFLAGS)

AC_SUBST(LCDDCLIENT_CFLAGS)
AC_SUBST(LCDDCLIENT_SOURCE)

AC_OUTPUT([
Makefile
include/Makefile
include/zapit/Makefile
include/zapit/xml/Makefile
include/zapit/zapci/Makefile
include/zapit/zapost/Makefile
include/zapit/zapsi/Makefile
lib/Makefile
src/Makefile
src/xml/Makefile
src/zapci/Makefile
src/zapost/Makefile
src/zapsi/Makefile
tuxbox-zapit.pc
])

