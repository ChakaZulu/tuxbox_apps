AC_INIT(tuxbox-apps-misc-tools,0.0.1,,tuxbox-misc-tools)
AM_INIT_AUTOMAKE(tuxbox-misc-tools,0.0.1)

TUXBOX_APPS
TUXBOX_BOXTYPE

AC_PROG_CC
AM_PROG_LIBTOOL

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

AC_OUTPUT([
Makefile
etherwake/Makefile
fbshot/Makefile
mtd/Makefile
rtc/Makefile
devinit/Makefile
dboxshot/Makefile
makedevices/Makefile
])
