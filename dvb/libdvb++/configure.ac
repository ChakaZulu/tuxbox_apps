AC_INIT([libdvb++],[0.2],[obi@saftware.de])
VERSION=0.2

AM_INIT_AUTOMAKE([libdvb++],[$VERSION])

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_HEADER([linux/dvb/version.h],[],
	[AC_MSG_ERROR([linux dvb api headers are required to build libdvb++])])

PKG_CHECK_MODULES([LIBDVBSIPP],[libdvbsi++])

LT_RELEASE=$VERSION
AC_SUBST([LT_RELEASE])

AC_OUTPUT([
Makefile
include/Makefile
include/dvb/Makefile
include/dvb/channel/Makefile
include/dvb/debug/Makefile
include/dvb/hardware/Makefile
include/dvb/pool/Makefile
qa/Makefile
qa/capmt/Makefile
src/Makefile
src/channel/Makefile
src/hardware/Makefile
src/pool/Makefile
libdvb++.pc
])
