AC_DEFUN([TUXBOX_APPS],
[AC_ARG_WITH(target,
	[  --with-target=TARGET    target for compilation [[native,cdk]]],
	[TARGET="$with_target"],[TARGET="native"])

AC_ARG_WITH(targetprefix,
	[  --with-targetprefix=PATH prefix relative to target root [[PREFIX[for native], /[for cdk]]]],
	[targetprefix="$with_target"],[targetprefix="NONE"])

AC_ARG_WITH(debug,
	[  --without-debug         disable debugging code],
	[if [$with_target != "yes"]; then DEBUG="-g3"; AC_DEFINE(DEBUG,1,[Enable debug messages]); fi],[DEBUG="-g3"])

if [ "$TARGET" = "native" ]; then
	if [ "$CFLAGS" = "" -a "$CXXFLAGS" = "" ]; then
		CFLAGS="-Wall -O2 -pipe $DEBUG"
		CXXFLAGS="-Wall -O2 -pipe $DEBUG"
	fi
	if [ "$prefix" = "NONE" ]; then
		prefix=/usr/local
	fi
	if [ "$targetprefix" = "NONE" ]; then
		targetprefix="\${prefix}"
		_targetprefix="${prefix}"
	else
		_targetprefix="$targetprefix"
	fi
elif [ "$TARGET" = "cdk" -o "$TARGET" = "cdkflash" ]; then
	if test "$CC" = "" -a "$CXX" = ""; then
		CC=powerpc-tuxbox-linux-gnu-gcc CXX=powerpc-tuxbox-linux-gnu-g++
	fi
	if [ "$CFLAGS" = "" -a "$CXXFLAGS" = "" ]; then
		CFLAGS="-Wall -Os -mcpu=823 -pipe $DEBUG"
		CXXFLAGS="-Wall -Os -mcpu=823 -pipe $DEBUG"
	fi
	if [ "$prefix" = "NONE" ]; then
		prefix=/dbox2/cdkroot
	fi
	if [ "$targetprefix" = "NONE" ]; then
		targetprefix=""
		_targetprefix=""
	else
		_targetprefix="$targetprefix"
	fi
	if [ "$host_alias" = "" ]; then
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

AC_HEADER_STDC

AC_CHECK_HEADERS(endian.h)

AC_C_BIGENDIAN])

AC_DEFUN([TUXBOX_APPS_LIBTOOL],
[AC_DISABLE_STATIC
AM_PROG_LIBTOOL])
