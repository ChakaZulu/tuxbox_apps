SUBDIRS = xml zapci zapost zapsi

INCLUDES = -I$(top_srcdir)/lib @CONFIGFILE_CFLAGS@ @LCDDCLIENT_CFLAGS@

bin_PROGRAMS = pzapit zapit

pzapit_SOURCES = \
	pzapit.cpp

pzapit_LDADD = \
	$(top_builddir)/lib/libzapitclient.la

zapit_SOURCES = \
	zapit.cpp transponder.cpp channel.cpp bouquets.cpp getservices.cpp scan.cpp $(DBOX2)

zapit_LDADD = \
	@CONFIGFILE_LIBS@ \
	$(top_builddir)/lib/libzapitclient.la \
	xml/libxml.a \
	zapci/libzapci.a \
	zapost/libzapost.a \
	zapsi/libzapsi.a \
	-lpthread

if DBOX2
DBOX2 = lcddclient.cpp
lcddclient.cpp:
	ln -sf @LCDDCLIENT_SOURCE@
endif

install-data-local:
	$(INSTALL) -d $(CONFIGDIR)/zapit

