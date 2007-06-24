AC_INIT(tuxbox-zapit,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-zapit,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY
TUXBOX_APPS_DRIVER

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

AC_ARG_WITH(boxtype,
	[  --with-boxtype=NAME box type [[dbox2,dm7000,dm500,dm56x0,dm600pvr...]]],
	[BOXTYPE="$withval"],[BOXTYPE="dbox2"])

AC_SUBST(BOXTYPE)
AM_CONDITIONAL(BOXTYPE_DBOX2, test "$BOXTYPE" = "dbox2")
AM_CONDITIONAL(BOXTYPE_DREAMBOX, test "$BOXTYPE" != "dbox2")

TUXBOX_APPS_DVB
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
TUXBOX_APPS_LIB_PKGCONFIG(CONNECTION,tuxbox-connection)
TUXBOX_APPS_LIB_PKGCONFIG(EVENTSERVER,tuxbox-eventserver)
CPPFLAGS="$CPPFLAGS $EVENTSERVER_CFLAGS -Werror"
CXXFLAGS="$CXXFLAGS -fno-rtti -fno-exceptions"
LDFLAGS="$LDFLAGS $EVENTSERVER_LIBS"

AC_LANG(C++)

AC_ARG_ENABLE(pid-cache,
	[  --enable-pid-cache      do not read pids after every channel change],
	[AC_DEFINE(USE_PID_CACHE,1,[do not read pids after every channel change])])

AC_ARG_ENABLE(use-libxml2,
	[  --enable-use-libxml2    use libxml2 instead of libxmltree],
	[AC_DEFINE(USE_LIBXML,1,[use libxml2 instead of libxmltree])
         TUXBOX_APPS_LIB_PKGCONFIG(XML,libxml-2.0)],
        [TUXBOX_APPS_LIB_PKGCONFIG(XML,tuxbox-xmltree)])

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

