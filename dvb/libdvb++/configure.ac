AC_INIT([libdvb++],[0.1],[obi@saftware.de])
VERSION=0.1

AM_INIT_AUTOMAKE([libdvb++],[$VERSION])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AC_PROG_LIBTOOL

AC_CHECK_HEADER([linux/dvb/version.h],[],
	[AC_MSG_ERROR([linux dvb api headers are required to build libdvb++])])

LT_RELEASE=$VERSION
AC_SUBST([LT_RELEASE])

AC_OUTPUT([
Makefile
include/Makefile
include/dvb/Makefile
include/dvb/channel/Makefile
include/dvb/debug/Makefile
include/dvb/descriptor/Makefile
include/dvb/hardware/Makefile
include/dvb/id/Makefile
include/dvb/pool/Makefile
include/dvb/table/Makefile
src/Makefile
src/channel/Makefile
src/descriptor/Makefile
src/hardware/Makefile
src/pool/Makefile
src/table/Makefile
libdvb++.pc
])

