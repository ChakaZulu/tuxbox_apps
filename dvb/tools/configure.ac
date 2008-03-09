AC_INIT(tuxbox-apps-dvb-tools,0.0.1,,dvb-tools)
AM_INIT_AUTOMAKE(dvb-zapit,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC

AC_PROG_CXX

TUXBOX_APPS_DVB

AC_ARG_WITH(boxtype,
	[  --with-boxtype=NAME dreambox/dbox2 [[dbox2,dreambox]]],
	[BOXTYPE="$withval"],[BOXTYPE="dbox2"])
AC_SUBST(BOXTYPE)
AM_CONDITIONAL(BOXTYPE_DREAMBOX,test "$BOXTYPE" != "dbox2")
AM_CONDITIONAL(BOXTYPE_DBOX2,test "$BOXTYPE" = "dbox2")

TUXBOX_APPS_LIB_PKGCONFIG(MPEGTOOLS,tuxbox-mpegtools)

AC_OUTPUT([
Makefile
dvb_test/Makefile
dvbnetctrl/Makefile
skyscan/Makefile
stream/Makefile
])

