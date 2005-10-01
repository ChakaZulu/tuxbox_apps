SUBDIRS = zapci zapost zapsi

INCLUDES = \
	-I$(top_srcdir)/include \
	@CONFIGFILE_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@XML_CFLAGS@

bin_PROGRAMS = pzapit zapit
sbin_PROGRAMS = udpstreampes

pzapit_SOURCES = \
	pzapit.cpp

pzapit_LDADD = \
	$(top_builddir)/lib/libzapitclient.la \
	@CONNECTION_LIBS@

udpstreampes_SOURCES = \
	udpstreampes.cpp

udpstreampes_LDADD = \
	$(top_builddir)/lib/libzapitclient.la \
	@CONNECTION_LIBS@ \
	-lpthread

zapit_SOURCES = \
	bouquets.cpp channel.cpp dvbstring.cpp \
	getservices.cpp scan.cpp xmlinterface.cpp \
	zapit.cpp

zapit_LDADD = \
	$(top_builddir)/lib/libzapitclient.la \
	$(top_builddir)/src/zapci/libzapci.la \
	$(top_builddir)/src/zapost/libzapost.la \
	$(top_builddir)/src/zapsi/libzapsi.la \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@ \
	@XML_LIBS@ \
	-lpthread

installdir = $(CONFIGDIR)/zapit

install_DATA = 

AM_CPPFLAGS = -DSKIP_CA_STATUS
