AC_INIT([libdvbsi++],[0.3.3],[obi@saftware.de],[libdvbsi++])
AM_INIT_AUTOMAKE([dist-bzip2])

AC_PROG_CXX
AC_PROG_LIBTOOL
AC_CHECK_HEADERS([byteswap.h endian.h])
AC_PATH_PROG(CXXTEST, cxxtestgen.pl, no)
AM_CONDITIONAL(HAVE_CXXTEST, test "$CXXTEST" != "no")
AC_PATH_PROG(CVS2CL, cvs2cl, no)
AM_CONDITIONAL(HAVE_CVS2CL, test "$CVS2CL" != "no")
AC_PATH_PROG(GNUPG, gpg, no)
AM_CONDITIONAL(HAVE_GNUPG, test "$GNUPG" != "no")
AC_PATH_PROG(MD5SUM, md5sum, no)
AM_CONDITIONAL(HAVE_MD5SUM, test "$MD5SUM" != "no")

dnl - If any interfaces have been added, removed, or changed since the last
dnl   update, increment current, and set revision to 0.
LT_CURRENT=0
AC_SUBST([LT_CURRENT])
dnl - If the library source code has changed at all since the last update,
dnl   then increment revision (c:r:a becomes c:r+1:a).
LT_REVISION=0
AC_SUBST([LT_REVISION])
dnl - If any interfaces have been added since the last public release, then
dnl   increment age.
dnl - If any interfaces have been removed since the last public release, then
dnl   set age to 0.
LT_AGE=0
AC_SUBST([LT_AGE])

AC_OUTPUT([
Makefile
include/Makefile
include/dvbsi++/Makefile
src/Makefile
qa/Makefile
libdvbsi++.pc
])
