AC_INIT([libdvbsi++],[0.3],[obi@saftware.de],[libdvbsi++])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_CHECK_HEADERS([byteswap.h endian.h])
AC_CHECK_PROG(HAVE_CXXTEST, cxxtestgen.pl, yes, no)
AC_SUBST(HAVE_CXXTEST)

LT_RELEASE=$PACKAGE_VERSION
AC_SUBST([LT_RELEASE])

AC_OUTPUT([
Makefile
include/Makefile
include/dvbsi++/Makefile
src/Makefile
qa/Makefile
libdvbsi++.pc
])
