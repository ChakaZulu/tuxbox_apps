SUBDIRS = xml zapci zapost zapsi

INCLUDES = -I$(top_srcdir)/lib @CONFIGFILE_CFLAGS@

bin_PROGRAMS = pzapit zapit

pzapit_SOURCES = \
	pzapit.cpp

pzapit_LDADD = \
	$(top_builddir)/lib/libzapitclient.la

zapit_SOURCES = \
	zapit.cpp transponder.cpp channel.cpp bouquets.cpp getservices.cpp scan.cpp


zapit_LDADD = \
	@CONFIGFILE_LIBS@ \
	$(top_builddir)/lib/libzapitclient.la \
	xml/libxml.a \
	zapci/libzapci.a \
	zapost/libzapost.a \
	zapsi/libzapsi.a \
	-lpthread

#
# used CPPFLAGS are:
#
#  DBOX2 - enable dbox2 specific code (see Makefile.am in every subdir, too)
#  USE_PID_CACHE - do not read pids after every channel change
#  EXCESSIVE_TUNING_RETRIES - retry tuning on failure

install-data-local:
	$(INSTALL) -d $(CONFIGDIR)/zapit

