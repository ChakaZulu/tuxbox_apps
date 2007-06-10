INCLUDES = -I$(top_srcdir)/libxmltree
lib_LTLIBRARIES = libtuxbox-upnpclient.la

libtuxbox_upnpclient_la_SOURCES = \
	UPNPSocket.cpp \
	UPNPDevice.cpp \
	UPNPService.cpp

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = tuxbox-upnpclient.pc
pkgincludedir = ${includedir}/tuxbox/upnpclient
pkginclude_HEADERS = upnpclient.h

libtuxbox_upnpclient_la_LIBADD = $(top_builddir)/libxmltree/libtuxbox-xmltree.la

$(top_builddir)/libxmltree/libtuxbox-xmltree.la:
	$(MAKE) -C $(top_builddir)/libxmltree
