AC_INIT(tuxbox-apps-tuxbox-libs,0.0.1,,libs)
AM_INIT_AUTOMAKE(libs,0.0.1)

TUXBOX_APPS

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_HEADER_STDC

TUXBOX_APPS_DRIVER
TUXBOX_APPS_LIB_CONFIG_CHECK([FREETYPE],[freetype-config])

AC_OUTPUT([
Makefile
libavs/Makefile
liblcddisplay/Makefile
])

