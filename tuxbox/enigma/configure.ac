AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(tuxbox, 0.0.1)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ARG_WITH(target,
	[  --with-target=TARGET    target for compilation [[dbox2,dbox2flash,native]]],
	[TARGET="$with_target"],[TARGET="dbox2"])

AM_CONDITIONAL(NATIVE_BUILD, [test x"$TARGET" = xnative])

AC_ARG_WITH(targetprefix,
	[  --with-targetprefix=PATH prefix relative of target root [[/[for dbox2/dbox2flash], PREFIX[for native]]]],
	[targetprefix="$with_target"],[targetprefix="NONE"])

AC_ARG_WITH(driver,
	[  --with-driver=PATH      were to find the driver sources [[PREFIX/../driver]]],
	[DRIVER="$with_driver"],[DRIVER=\${prefix}/../driver])

AC_SUBST(DRIVER)

AM_CONDITIONAL(HAVE_DVB, test "$DRIVER" != "no")

AM_CONDITIONAL(HAVE_DBOX, test "$DRIVER" != "no")

if test "$TARGET" = "dbox2"; then
	if test "$CC" = "" -a "$CXX" = ""; then
		CC=powerpc-tuxbox-linux-gnu-gcc CXX=powerpc-tuxbox-linux-gnu-g++
	fi
	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -O2 -g3 -mcpu=823 -pipe"
		CXXFLAGS="-Wall -O2 -g3 -mcpu=823 -pipe"
	fi
	if test "$prefix" = "NONE"; then
		prefix=/dbox2/cdkroot
	fi
	if test "$targetprefix" = "NONE"; then
		targetprefix=""
		_targetprefix=""
	else
		_targetprefix="$targetprefix"
	fi
	if test "$host_alias" = ""; then
		cross_compiling=yes
		host_alias=powerpc-tuxbox-linux-gnu
	fi
	AC_DEFINE(__DEBUG__,1,[debug])
elif test "$TARGET" = "dbox2flash"; then
	if test "$CC" = "" -a "$CXX" = ""; then
		CC=powerpc-tuxbox-linux-gnu-gcc CXX=powerpc-tuxbox-linux-gnu-g++
	fi
	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -Os -mcpu=823 -pipe"
		CXXFLAGS="-Wall -Os -mcpu=823 -pipe"
	fi
	if test "$LDFLAGS" = ""; then
		LDFLAGS="-s"
	fi
	if test "$prefix" = "NONE"; then
		prefix=/dbox2/flashroot
	fi
	if test "$targetprefix" = "NONE"; then
		targetprefix=""
		_targetprefix=""
	else
		_targetprefix="$targetprefix"
	fi
	if test "$host_alias" = ""; then
		cross_compiling=yes
		host_alias=powerpc-tuxbox-linux-gnu
	fi
elif test "$TARGET" = "native"; then
	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -O2 -g3 -pipe"
		CXXFLAGS="-Wall -O2 -g3 -pipe"
	fi
	if test "$prefix" = "NONE"; then
		prefix=/usr/local
	fi
	if test "$targetprefix" = "NONE"; then
		targetprefix="\${prefix}"
		_targetprefix="${prefix}"
	else
		_targetprefix="$targetprefix"
	fi
else
	AC_MSG_ERROR([invalid target $TARGET, choose on from dbox2,dbox2flash,native]);
fi

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

targetdatadir="\${targetprefix}/share"
_targetdatadir="${_targetprefix}/share"
targetsysconfdir="\${targetprefix}/etc"
_targetsysconfdir="${_targetprefix}/etc"
targetlocalstatedir="\${targetprefix}/var"
_targetlocalstatedir="${_targetprefix}/var"
targetlibdir="\${targetprefix}/lib"
_targetlibdir="${_targetprefix}/lib"
AC_SUBST(targetprefix)
AC_SUBST(targetdatadir)
AC_SUBST(targetsysconfdir)
AC_SUBST(targetlocalstatedir)

CONFIGDIR="\${localstatedir}/tuxbox/config"
_CONFIGDIR="${_targetlocalstatedir}/tuxbox/config"
AC_SUBST(CONFIGDIR)
AC_DEFINE_UNQUOTED(CONFIGDIR,"$_CONFIGDIR",[where to find the config files])

DATADIR="\${datadir}/tuxbox"
_DATADIR="${_targetdatadir}/tuxbox"
AC_SUBST(DATADIR)
AC_DEFINE_UNQUOTED(DATADIR,"$_DATADIR",[where to find data like icons])

FONTDIR="\${datadir}/fonts"
_FONTDIR="${_targetdatadir}/fonts"
AC_SUBST(FONTDIR)
AC_DEFINE_UNQUOTED(FONTDIR,"$_FONTDIR",[where to find the fonts])

GAMESDIR="\${localstatedir}/tuxbox/games"
_GAMESDIR="${_targetlocalstatedir}/tuxbox/games"
AC_SUBST(GAMESDIR)
AC_DEFINE_UNQUOTED(GAMESDIR,"$_GAMESDIR",[where games data is stored])

LIBDIR="\${libdir}/tuxbox"
_LIBDIR="${_targetlibdir}/tuxbox"
AC_SUBST(LIBDIR)
AC_SUBST(_LIBDIR)
AC_DEFINE_UNQUOTED(LIBDIR,"$_LIBDIR",[where to find the internal libs])

PLUGINDIR="\${libdir}/tuxbox/plugins"
_PLUGINDIR="${_targetlibdir}/tuxbox/plugins"
AC_SUBST(PLUGINDIR)
AC_DEFINE_UNQUOTED(PLUGINDIR,"$_PLUGINDIR",[where to find the plugins])

UCODEDIR="\${localstatedir}/tuxbox/ucodes"
_UCODEDIR="${_targetlocalstatedir}/tuxbox/ucodes"
AC_SUBST(UCODEDIR)
AC_DEFINE_UNQUOTED(UCODEDIR,"$_UCODEDIR",[where to find the dbox2 ucodes (firmware)])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL

AC_DISABLE_STATIC
AM_PROG_LIBTOOL

AC_PROG_RANLIB

ALL_LINGUAS=""
AM_GNU_GETTEXT

AC_HEADER_STDC

AC_CHECK_HEADERS(endian.h)

AC_C_BIGENDIAN

check_path () {
	return `perl -e "if(\"$1\"=~m#^/usr/(local/)?bin#){print "0"}else{print "1";}"`
}

AC_PATH_PROG(CURL_CONFIG,curl-config,no)

if test "$CURL_CONFIG" = "no"; then
	AC_MSG_WARN([could not find curl-config]);
else
	if test "$TARGET" = "dbox2" -o "$TARGET" = "dbox2flash" && check_path "$CURL_CONFIG"; then
		AC_MSG_WARN([could not find a suitable version of curl-config]);
	else
		LIBCURL_CFLAGS="`$CURL_CONFIG --cflags`"
		LIBCURL_LIBS="`$CURL_CONFIG --libs`"
	fi
fi

AC_SUBST(LIBCURL_CFLAGS)
AC_SUBST(LIBCURL_LIBS)
AM_CONDITIONAL(HAVE_LIBCURL,test "$CURL_CONFIG" != "no")

AC_PATH_PROG(FREETYPE_CONFIG,freetype-config,no)

if test "$FREETYPE_CONFIG" = "no"; then
	AC_MSG_WARN([could not find freetype-config]);
else
	if test "$TARGET" = "dbox2" -o "$TARGET" = "dbox2flash" && check_path "$FREETYPE_CONFIG"; then
		AC_MSG_WARN([could not find a suitable version of freetype-config]);
	else
		LIBFREETYPE_CFLAGS="`$FREETYPE_CONFIG --cflags`"
		LIBFREETYPE_LIBS="`$FREETYPE_CONFIG --libtool`"
	fi
fi

AC_SUBST(LIBFREETYPE_CFLAGS)
AC_SUBST(LIBFREETYPE_LIBS)
AM_CONDITIONAL(HAVE_LIBFREETYPE,test "$FREETYPE_CONFIG" != "no")

AC_PATH_PROG(COMMONCPLUSPLUS_CONFIG,ccgnu2-config,no)

if test "$COMMONCPLUSPLUS_CONFIG" = "no"; then
	AC_MSG_WARN([could not find commonc++-config]);
else
	if test "$TARGET" = "dbox2" -o "$TARGET" = "dbox2flash" && check_path "$COMMONCPLUSPLUS_CONFIG"; then
		AC_MSG_WARN([could not find a suitable version of commonc++-config]);
	else
		LIBCOMMONCPLUSPLUS_CFLAGS="`$COMMONCPLUSPLUS_CONFIG --flags`"
		LIBCOMMONCPLUSPLUS_LIBS="`$COMMONCPLUSPLUS_CONFIG --libs`"
		LIBCOMMONCPLUSPLUS_INCLUDE="`$COMMONCPLUSPLUS_CONFIG --includes`"
	fi
fi

AC_SUBST(LIBCOMMONCPLUSPLUS_CFLAGS)
AC_SUBST(LIBCOMMONCPLUSPLUS_LIBS)
AC_SUBST(LIBCOMMONCPLUSPLUS_INCLUDE)
AM_CONDITIONAL(HAVE_LIBCOMMONCPLUSPLUS,test "$COMMONCPLUSPLUS_CONFIG" != "no")

AC_CHECK_LIB(png,png_init_io,[HAVE_LIBPNG="yes"],[HAVE_LIBPNG="no"])

if test "$HAVE_LIBPNG" = "no"; then
	AC_MSG_WARN([could not find libpng]);
else
	LIBPNG_LIBS="-lpng"
fi

AC_SUBST(LIBPNG_LIBS)
AM_CONDITIONAL(HAVE_LIBPNG,test "$HAVE_LIBPNG" != "no")

AC_PATH_PROG(PKG_CONFIG,pkg-config,no)
if test "$PKG_CONFIG" = "no"; then
	AC_MSG_WARN([could not find pkg-config]);
else
	if ! PKG_CONFIG_PATH=${prefix}/lib/pkgconfig $PKG_CONFIG sigc++-1.2 --exists; then
		AC_MSG_WARN([could not find sigc++-1.2]);
		AM_CONDITIONAL(HAVE_LIBSIGC,false)
	else
		LIBSIGC_CFLAGS="`PKG_CONFIG_PATH=${prefix}/lib/pkgconfig $PKG_CONFIG sigc++-1.2 --cflags`"
		LIBSIGC_LIBS="`PKG_CONFIG_PATH=${prefix}/lib/pkgconfig $PKG_CONFIG sigc++-1.2 --libs`"
		AM_CONDITIONAL(HAVE_LIBSIGC,true)
	fi
fi

AC_SUBST(LIBSIGC_CFLAGS)
AC_SUBST(LIBSIGC_LIBS)

AC_PATH_PROG(XML2_CONFIG,xml2-config,no)

if test "$XML2_CONFIG" = "no"; then
	AC_MSG_WARN([could not find xml2-config]);
else
	if test "$TARGET" = "dbox2" -o "$TARGET" = "dbox2flash" && check_path "$XML2_CONFIG"; then
		AC_MSG_WARN([could not find a suitable version of xml2-config]);
	else
		LIBXML2_CFLAGS="`$XML2_CONFIG --cflags`"
		LIBXML2_LIBS="`$XML2_CONFIG --libs`"
	fi
fi

AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)
AM_CONDITIONAL(HAVE_LIBXML2,test "$XML2_CONFIG" != "no")

AC_PATH_PROG(MOC,moc,no)

if test "$MOC" = "no"; then
	AC_MSG_WARN([could not find moc]);
fi

AM_CONDITIONAL(HAVE_MOC,test "$MOC" != "no")

AC_PATH_PROG(DOXYGEN,doxygen,no)

if test "$DOXYGEN" = "no"; then
	AC_MSG_WARN([could not find doxygen]);
fi

AM_CONDITIONAL(HAVE_DOXYGEN,test "$DOXYGEN" != "no")

#
# summary
#
AC_MSG_RESULT()
AC_MSG_RESULT(Summary:)
AC_MSG_RESULT([Target:		$TARGET])
AC_MSG_RESULT([Prefix:		$prefix])
AC_MSG_RESULT([Prefix on target: $_targetprefix])
AC_MSG_RESULT([Driver:		$DRIVER])
AC_MSG_RESULT([CONFIGDIR:	$_CONFIGDIR])
AC_MSG_RESULT([DATADIR:	$_DATADIR])
AC_MSG_RESULT([FONTDIR:	$_FONTDIR])
AC_MSG_RESULT([PLUGINDIR:	$_PLUGINDIR])
AC_MSG_RESULT([UCODEDIR:	$_UCODEDIR])
AC_MSG_RESULT([GAMESDIR:	$_GAMESDIR])
AC_MSG_RESULT()

AC_OUTPUT([
Makefile
doc/Makefile
fonts/Makefile
htdocs/Makefile
intl/Makefile
pictures/chk/Makefile
pictures/chk_v2/Makefile
pictures/decoration/Makefile
pictures/mainmenu/Makefile
pictures/Makefile
po/Makefile
resources/Makefile
skins/Makefile
src/Makefile
src/apps/Makefile
src/apps/enigma/Makefile
src/core/Makefile
src/core/base/Makefile
src/core/driver/Makefile
src/core/dvb/Makefile
src/core/dvb/lowlevel/Makefile
src/core/gdi/Makefile
src/core/gui/Makefile
src/core/system/Makefile
src/core/xml/Makefile
src/core/socket/Makefile
templates/Makefile
])

