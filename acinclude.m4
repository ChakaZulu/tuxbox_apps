AC_DEFUN_ONCE([TUXBOX_APPS],[
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_ARG_WITH(target,
	[  --with-target=TARGET    target for compilation [[native,cdk]]],
	[TARGET="$withval"],[TARGET="native"])

AC_ARG_WITH(targetprefix,
	[  --with-targetprefix=PATH prefix relative to target root [[PREFIX[for native], /[for cdk]]]],
	[targetprefix="$withval"],[targetprefix="NONE"])

AC_ARG_WITH(debug,
	[  --without-debug         disable debugging code],
	[DEBUG="$withval"],[DEBUG="yes"])

if test "$DEBUG" = "yes"; then
	DEBUG_CFLAGS="-g3"
	AC_DEFINE(DEBUG,1,[Enable debug messages])
fi

if test "$TARGET" = "native"; then
	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
		CXXFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
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
elif test "$TARGET" = "cdk"; then
	if test "$CC" = "" -a "$CXX" = ""; then
		CC=powerpc-tuxbox-linux-gnu-gcc CXX=powerpc-tuxbox-linux-gnu-g++
	fi
	if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
		CFLAGS="-Wall -Os -mcpu=823 -pipe $DEBUG_CFLAGS"
		CXXFLAGS="-Wall -Os -mcpu=823 -pipe $DEBUG_CFLAGS"
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
else
	AC_MSG_ERROR([invalid target $TARGET, choose on from native,cdk]);
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

check_path () {
	return `perl -e "if(\"$1\"=~m#^/usr/(local/)?bin#){print \"0\"}else{print \"1\";}"`
}

])

AC_DEFUN_ONCE([TUXBOX_APPS_DIRECTORY],[
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
AC_DEFINE_UNQUOTED(UCODEDIR,"$_UCODEDIR",[where to find the ucodes (firmware)])
])

AC_DEFUN_ONCE([TUXBOX_APPS_ENDIAN],[
AC_CHECK_HEADERS(endian.h)
AC_C_BIGENDIAN
])

AC_DEFUN_ONCE([TUXBOX_APPS_DRIVER],[
AC_ARG_WITH(driver,
	[  --with-driver=PATH      path for driver sources[[NONE]]],
	[DRIVER="$withval"],[DRIVER=""])

if test -z "$DRIVER"; then
	AC_MSG_ERROR([can't find driver sources])
fi
AC_SUBST(DRIVER)
])

AC_DEFUN_ONCE([TUXBOX_APPS_DVB],[
AC_ARG_WITH(dvbincludes,
	[  --with-dvbincludes=PATH path for dvb includes[[NONE]]],
	[DVBINCLUDES="$withval"],[DVBINCLUDES=""])

orig_CFLAGS=$CFLAGS
orig_CPPFLAGS=$CPPFLAGS
if test "$DVBINCLUDES"; then
	CFLAGS="-I$DVBINCLUDES"
	CPPFLAGS="-I$DVBINCLUDES"
else
	CFLAGS=""
	CPPFLAGS=""
fi
AC_CHECK_HEADERS(dvb/version.h,[DVB_VERSION_H="yes"])
AC_CHECK_HEADERS(ost/dmx.h,[OST_DMX_H="yes"])
if test "$DVB_VERSION_H"; then
	AC_MSG_NOTICE([found dvb version 2 or later])
elif test "$OST_DMX_H"; then
	AC_MSG_NOTICE([found dvb version 1])
else
	AC_MSG_ERROR([can't find dvb headers])
fi
DVB_VERSION_H=
OST_DMX_H=
CFLAGS="$orig_CFLAGS -I$DVBINCLUDES"
CPPFLAGS="$orig_CPPFLAGS -I$DVBINCLUDES"
CXXFLAGS="$CXXFLAGS -I$DVBINCLUDES"
])

AC_DEFUN([_TUXBOX_APPS_LIB_CONFIG_CHECK],[
AC_PATH_PROG($2_CONFIG,$3,no)

if test "$$2_CONFIG" = "no"; then
	AC_MSG_$1([could not find $3]);
else
	if test "$TARGET" = "cdk" && check_path "$$2_CONFIG"; then
		AC_MSG_$1([could not find a suitable version of $3]);
	else
		$2_CFLAGS=`$$2_CONFIG --cflags`
		$2_LIBS=`$$2_CONFIG --libtool`
	fi
fi

AC_SUBST($2_CFLAGS)
AC_SUBST($2_LIBS)
])

AC_DEFUN([TUXBOX_APPS_LIB_CONFIG_CHECK],[
_TUXBOX_APPS_LIB_CONFIG_CHECK(ERROR,$1,$2)
])

AC_DEFUN([TUXBOX_APPS_LIB_CONFIG_CHECK_WARN],[
_TUXBOX_APPS_LIB_CONFIG_CHECK(WARN,$1,$2)
])

AC_DEFUN([_TUXBOX_APPS_LIB_PKGCONFIG_CHECK],[
if test -z "$PKG_CONFIG"; then
	AC_PATH_PROG(PKG_CONFIG, pkg-config,no)
fi

if test "$PKG_CONFIG" = "no" ; then
	AC_MSG_$1([could not find pkg-config]);
else
	AC_MSG_CHECKING(for $3)
	if $PKG_CONFIG --exists "$3" ; then
		AC_MSG_RESULT(yes)
		$2_CFLAGS=`$PKG_CONFIG --cflags "$3"`
		$2_LIBS=`$PKG_CONFIG --libs "$3"`
	else
		AC_MSG_RESULT(no)
		AC_MSG_$1([could not find $3]);
	fi
fi

AC_SUBST($2_CFLAGS)
AC_SUBST($2_LIBS)
])

