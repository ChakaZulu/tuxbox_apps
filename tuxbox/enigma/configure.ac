AC_INIT(tuxbox-enigma,0.0.1)
AM_INIT_AUTOMAKE(tuxbox-enigma,0.0.1)

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AC_ARG_WITH(boxtype,
	[  --with-boxtype=NAME box type [[dbox2,dm7000,dm500,dm56x0,...]]],
	[BOXTYPE="$withval"],[BOXTYPE="dbox2"])
	
AC_SUBST(BOXTYPE)
AM_CONDITIONAL(BOXTYPE_DBOX2, test "$BOXTYPE" = "dbox2")
AM_CONDITIONAL(BOXTYPE_DREAMBOX, test "$BOXTYPE" != "dbox2")

TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER

TUXBOX_APPS_LIB_CONFIG(FREETYPE,freetype-config)
TUXBOX_APPS_LIB_PKGCONFIG(ID3TAG,id3tag)
TUXBOX_APPS_LIB_PKGCONFIG(MAD,mad)
TUXBOX_APPS_LIB_PKGCONFIG(MD5SUM,tuxbox-md5sum)
TUXBOX_APPS_LIB_PKGCONFIG(PLUGINS,tuxbox-plugins)
TUXBOX_APPS_LIB_PKGCONFIG(PNG,libpng)
TUXBOX_APPS_LIB_PKGCONFIG(SIGC,sigc++-1.2)
TUXBOX_APPS_LIB_PKGCONFIG(XMLTREE,tuxbox-xmltree)
#TUXBOX_APPS_LIB_PKGCONFIG(TUXTXT,tuxbox-tuxtxt)
if test "$BOXTYPE" = "dbox2" ; then
	TUXBOX_APPS_LIB_PKGCONFIG(TUXBOX,tuxbox)
	TUXBOX_APPS_LIB_PKGCONFIG(CONFIGFILE,tuxbox-configfile)
fi

if test "$BOXTYPE" = "dbox2" ; then
	CPPFLAGS="$CPPFLAGS -fno-exceptions -fno-rtti -D_REENTRANT -DENABLE_DYN_ROTOR -DENABLE_DYN_MOUNT -DENABLE_DYN_CONF -DENABLE_DYN_FLASH -DENABLE_FLASHTOOL -DENABLE_PRIVATE_EPG"
else
	CPPFLAGS="$CPPFLAGS -fno-exceptions -fno-rtti -D_REENTRANT -DDISABLE_LIRC -DENABLE_KEYBOARD -DUSE_EXTERNAL_FLASHTOOL -DDEBUG"
fi

if test "$BOXTYPE" = "dm56x0" ; then
	CPPFLAGS="$CPPFLAGS -DENABLE_RFMOD"
elif test "$BOXTYPE" = "dm7020" ; then
	TUXBOX_APPS_LIB_PKGCONFIG(NET,tuxbox-net)
	CPPFLAGS="$CPPFLAGS -DENABLE_RFMOD -DUSE_IFUPDOWN $NET_CFLAGS"
fi
    
CPPFLAGS="$CPPFLAGS $FREETYPE_CFLAGS $ID3TAG_CFLAGS $MAD_CFLAGS $MD5SUM_CFLAGS"
CPPFLAGS="$CPPFLAGS $PLUGINS_CFLAGS $PNG_CFLAGS $SIGC_CFLAGS $TUXBOX_CFLAGS $XMLTREE_CFLAGS"
LDFLAGS="-pthread $LDFLAGS"

TUXBOX_APPS_GETTEXT

AC_OUTPUT([
Makefile
data/Makefile
data/fonts/Makefile
data/htdocs/Makefile
data/htdocs/stylesheets/Makefile
data/pictures/Makefile
data/pictures/chk_stone/Makefile
data/pictures/chk_stone/mainmenu/Makefile
data/pictures/chk_silver/Makefile
data/pictures/chk_silver/mainmenu/Makefile
data/pictures/chk_v2/Makefile
data/pictures/countries/Makefile
data/pictures/neutrino/Makefile
data/pictures/mainmenu/Makefile
data/pictures/shortcut/Makefile
data/pictures/keypics/Makefile
data/pictures/small/Makefile
data/pictures/carbon/Makefile
data/pictures/blueqt/Makefile
data/resources/Makefile
data/skins/Makefile
data/sysconfig/Makefile
data/templates/Makefile
data/lirc/Makefile
doc/Makefile
hotplug/Makefile
include/Makefile
include/lib/Makefile
include/lib/base/Makefile
include/lib/codecs/Makefile
include/lib/driver/Makefile
include/lib/dvb/Makefile
include/lib/gdi/Makefile
include/lib/gui/Makefile
include/lib/socket/Makefile
include/lib/system/Makefile
lib/Makefile
lib/base/Makefile
lib/driver/Makefile
lib/codecs/Makefile
lib/dvb/Makefile
lib/dvb/lowlevel/Makefile
lib/gdi/Makefile
lib/gui/Makefile
lib/socket/Makefile
lib/system/Makefile
lib/picviewer/Makefile
po/Makefile
src/Makefile
enigma.pc
])
