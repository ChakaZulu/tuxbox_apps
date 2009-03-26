AC_INIT(tuxbox-tools,0.0.1,,tuxbox-tools)
AM_INIT_AUTOMAKE(tuxbox-tools,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX

TUXBOX_APPS_DRIVER
TUXBOX_APPS_DVB
TUXBOX_BOXTYPE

AC_ARG_WITH(boxtype,
	[  --with-boxtype=NAME box type [[dbox2,tripledragon,dm7000,dm500,dm56x0,dm600pvr...]]],
	[BOXTYPE="$withval"],[BOXTYPE="dbox2"])

AC_SUBST(BOXTYPE)
AM_CONDITIONAL(BOXTYPE_DBOX2, test "$BOXTYPE" = "dbox2")
AM_CONDITIONAL(BOXTYPE_TRIPLE, test "$BOXTYPE" = "tripledragon")
AM_CONDITIONAL(BOXTYPE_DREAMBOX, test "$BOXTYPE" != "dbox2" -a "$BOXTYPE" != "tripledragon")

if test "$BOXTYPE" = "dbox2"; then
	AC_DEFINE(HAVE_DBOX_HARDWARE, 1, [building for a dbox2])
elif test "$BOXTYPE" = "tripledragon"; then
	AC_DEFINE(HAVE_TRIPLEDRAGON, 1, [building for a tripledragon])
else	# many dreamboxes, don't list them one by one...
	AC_DEFINE(HAVE_DREAMBOX_HARDWARE, 1, [building for a dreambox])
fi

# TUXBOX_APPS_LIB_PKGCONFIG(PLUGINS,tuxbox-plugins)
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
