AC_INIT(tuxbox-zapit,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-zapit,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY
TUXBOX_APPS_PKGCONFIG
TUXBOX_BOXTYPE

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

if test "$BOXTYPE" != "tripledragon"; then
if test "$BOXTYPE" != "generic"; then
TUXBOX_APPS_DRIVER
fi
TUXBOX_APPS_DVB
fi

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

AC_ARG_ENABLE(lirc,
       AS_HELP_STRING(--enable-lirc,include lirc support),
       ,[enable_lirc=no])
AM_CONDITIONAL(ENABLE_LIRC,test "$enable_lirc" = "yes")

if test "$enable_lirc" = "yes"; then
TUXBOX_APPS_LIB_PKGCONFIG(LIRCDCLIENT,tuxbox-lircdclient)
AC_DEFINE(ENABLE_LIRC,1,[include lirc support])
else
LIRCDCLIENT_LIBS=""
LIRCDCLIENT_CFLAGS=""
AC_SUBST(LIRCDCLIENT_LIBS)
AC_SUBST(LIRCDCLIENT_CFLAGS)
fi

AC_OUTPUT([
Makefile
include/Makefile
include/controldclient/Makefile
include/zapit/Makefile
include/zapit/client/Makefile
lib/Makefile
src/Makefile
src/zapci/Makefile
src/zapost/Makefile
src/zapsi/Makefile
tuxbox-zapit.pc
])

if test "$enable_lirc" = "yes"; then
AC_OUTPUT([
src/irsend/Makefile
])
fi
