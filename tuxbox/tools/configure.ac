AC_INIT(tuxbox-tools,0.0.1,,tuxbox-tools)
AM_INIT_AUTOMAKE(tuxbox-tools,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX

AC_ARG_WITH(boxtype,
	[  --with-boxtype=NAME box type [[dbox2,dm7000,dm500,dm56x0,dm600pvr...]]],
	[BOXTYPE="$withval"],[BOXTYPE="dbox2"])

AC_SUBST(BOXTYPE)
AM_CONDITIONAL(BOXTYPE_DBOX2, test "$BOXTYPE" = "dbox2")
AM_CONDITIONAL(BOXTYPE_DREAMBOX, test "$BOXTYPE" != "dbox2")

TUXBOX_APPS_DRIVER
TUXBOX_APPS_DVB

TUXBOX_APPS_LIB_PKGCONFIG(PLUGINS,tuxbox-plugins)
TUXBOX_APPS_LIB_PKGCONFIG(TUXBOX,tuxbox)

AC_OUTPUT([
Makefile
camd/Makefile
misc/Makefile
satfind/Makefile
test/Makefile
tuxinfo/Makefile
graphics/Makefile
kb2rcd/Makefile
cdkVcInfo/Makefile
])

