INCLUDES = -I$(top_srcdir)/libmd5sum

lib_LTLIBRARIES = libtuxbox-ucodes.la

libtuxbox_ucodes_la_SOURCES = libucodes.c
libtuxbox_ucodes_la_LIBADD = $(top_builddir)/libmd5sum/libtuxbox-md5sum.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tuxbox-ucodes.pc
pkgincludedir = ${includedir}/tuxbox
pkginclude_HEADERS = libucodes.h
