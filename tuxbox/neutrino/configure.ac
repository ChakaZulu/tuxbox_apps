AC_INIT(tuxbox-apps-tuxbox-neutrino,0.0.1,,neutrino)
AM_INIT_AUTOMAKE(neutrino,0.0.1)

TUXBOX_APPS

AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL

TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER
TUXBOX_APPS_LIB_PKGCONFIG(EVENTSERVER,tuxbox-eventserver)

AC_OUTPUT([
Makefile
daemons/Makefile
daemons/controld/Makefile
daemons/controld/driver/Makefile
daemons/lcdd/Makefile
daemons/sections/Makefile
daemons/timerd/Makefile
libs/Makefile
libs/libcontroldclient/Makefile
libs/liblcddclient/Makefile
])

