AC_INIT(tuxbox-apps-dvb-tools,0.0.1,,dvb-tools)
AM_INIT_AUTOMAKE(dvb-zapit,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC

TUXBOX_APPS_DVB

AM_CONDITIONAL(BOXTYPE_DREAMBOX, 0)

TUXBOX_APPS_LIB_PKGCONFIG(MPEGTOOLS,tuxbox-mpegtools)

AC_OUTPUT([
Makefile
dvb_test/Makefile
dvbnetctrl/Makefile
skyscan/Makefile
stream/Makefile
])

