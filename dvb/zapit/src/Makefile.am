SUBDIRS = zapci zapost zapsi irsend

INCLUDES = \
	-I$(top_srcdir)/include \
	@CONFIGFILE_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@LIRCDCLIENT_CFLAGS@ \
	@XML_CFLAGS@

bin_PROGRAMS = pzapit zapit controldc
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
	getservices.cpp scan.cpp \
	eventwatchdog.cpp controld.cpp \
	zapit.cpp

zapit_LDADD = \
	$(top_builddir)/lib/libzapitclient.la \
	$(top_builddir)/src/zapci/libzapci.la \
	$(top_builddir)/src/zapost/libzapost.la \
	$(top_builddir)/src/zapsi/libzapsi.la \
	$(top_builddir)/src/irsend/libirsend.la \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@ \
	@LIRCDCLIENT_LIBS@ \
	@XML_LIBS@ \
	-lpthread

if BOXTYPE_DBOX2
zapit_LDADD += \
	-ltuxbox
endif

controldc_SOURCES = \
	controldc.cpp

controldc_LDADD = $(top_builddir)/lib/libcontroldclient.la @CONNECTION_LIBS@

installdir = $(CONFIGDIR)/zapit

install_DATA = 

if BOXTYPE_DBOX2
install-data-local:
	install -d $(CONFIGDIR)
	install -m 644 $(srcdir)/scart.conf $(CONFIGDIR)
endif
