INCLUDES = -I$(srcdir)/xmltok

lib_LTLIBRARIES = libtuxbox-xmltree.la

libtuxbox_xmltree_la_SOURCES = \
	hashtab.c xmlparse.cpp xmlrole.c xmltok.c xmltree.cpp xmlinterface.cpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tuxbox-xmltree.pc
pkgincludedir = ${includedir}/tuxbox/xmltree
pkginclude_HEADERS = hashtab.h xmlparse.h xmlrole.h xmltok.h xmltree.h xmlinterface.h
