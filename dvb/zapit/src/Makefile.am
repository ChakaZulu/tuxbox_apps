SUBDIRS = zapci zapost zapsi

INCLUDES = \
	-I$(top_srcdir)/include \
	@CONFIGFILE_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@XMLTREE_CFLAGS@

bin_PROGRAMS = pzapit zapit

pzapit_SOURCES = \
	pzapit.cpp

pzapit_LDADD = \
	@CONNECTION_LIBS@ \
	$(top_builddir)/lib/libzapitclient.la

zapit_SOURCES = \
	bouquets.cpp channel.cpp dvbstring.cpp \
	getservices.cpp scan.cpp xmlinterface.cpp \
	zapit.cpp

zapit_LDADD = \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@ \
	@XMLTREE_LIBS@ \
	$(top_builddir)/lib/libzapitclient.la \
	zapci/libzapci.la \
	zapost/libzapost.la \
	zapsi/libzapsi.la \
	-lpthread

AM_CXXFLAGS = -fno-rtti -fno-exceptions

install-data-local:
	$(INSTALL) -d $(CONFIGDIR)/zapit

