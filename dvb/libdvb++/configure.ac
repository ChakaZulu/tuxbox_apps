AC_INIT([libdvb++],[0.2],[obi@saftware.de],[libdvb++])
AM_INIT_AUTOMAKE([dist-bzip2 no-define nostdinc])

AC_PROG_CXX
AC_PROG_LIBTOOL
TUXBOX_APPS_LIB_PKGCONFIG([LIBDVBSIPP],[libdvbsi++])
AC_CHECK_HEADER([linux/dvb/version.h],[],
	[AC_MSG_ERROR([linux dvb api headers are required to build libdvb++])])

LT_RELEASE=$PACKAGE_VERSION
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
