AC_INIT([dvbsnoop],[1.1.55],[rasc@users.sourceforge.net])
AM_INIT_AUTOMAKE([dvbsnoop],[1.1.55])
#AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_RANLIB

AC_CHECK_HEADER([linux/dvb/dmx.h])
AC_CHECK_HEADER([ost/dmx.h])

if test "$ac_cv_header_linux_dvb_dmx_h" = yes; then
	AC_DEFINE([HAVE_LINUX_DVB_DMX_H])
elif test "$ac_cv_header_ost_dmx_h" = yes; then
	AC_DEFINE([HAVE_OST_DMX_H])
else
	AC_MSG_ERROR([linux dvb api headers are required to build dvbsnoop])
fi

AC_OUTPUT([
Makefile
src/Makefile
src/datacarousel/Makefile
src/descriptors/Makefile
src/dvb_api/Makefile
src/pes/Makefile
src/sections/Makefile
src/ts/Makefile
src/testdata/Makefile
src/strings/Makefile
src/misc/Makefile
])

