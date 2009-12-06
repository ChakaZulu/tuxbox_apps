AC_INIT(tuxbox-tools,0.0.1,,tuxbox-tools)
AM_INIT_AUTOMAKE(tuxbox-tools,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX

TUXBOX_APPS_DRIVER
TUXBOX_APPS_DVB
TUXBOX_BOXTYPE

# TUXBOX_APPS_LIB_PKGCONFIG(PLUGINS,tuxbox-plugins)
TUXBOX_APPS_LIB_PKGCONFIG(TUXBOX,tuxbox)
TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)

# needed by aformat
TUXBOX_APPS_LIB_PKGCONFIG(ZAPIT,tuxbox-zapit)
TUXBOX_APPS_LIB_PKGCONFIG(CONNECTION,tuxbox-connection)
TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)

AC_OUTPUT([
Makefile
aformat/Makefile
camd/Makefile
misc/Makefile
satfind/Makefile
test/Makefile
tuxinfo/Makefile
graphics/Makefile
kb2rcd/Makefile
cdkVcInfo/Makefile
msgbox/Makefile
])
