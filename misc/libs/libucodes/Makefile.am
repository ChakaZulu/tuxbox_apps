INCLUDES = \
        -I$(top_srcdir)/libmd5sum

lib_LTLIBRARIES = libtuxbox-ucodes.la

tuxboxincludedir = ${includedir}/tuxbox
tuxboxinclude_HEADERS = libucodes.h

libtuxbox_ucodes_la_SOURCES = libucodes.c
libtuxbox_ucodes_la_LIBADD = $(top_builddir)/libmd5sum/libtuxbox-md5sum.la
