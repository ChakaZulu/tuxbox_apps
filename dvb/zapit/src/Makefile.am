SUBDIRS = clientlib xml zapci zapost zapsi

INCLUDES = \
	-I$(top_srcdir)/libconfigfile \
	-I$(top_srcdir)/mczap/lcdd/clientlib \
	-I$(top_srcdir)/mczap/libevent \
	-I$(DRIVER)/include

bin_PROGRAMS = pzapit zapit

pzapit_SOURCES = \
	pzapit.cpp

pzapit_LDADD = \
	$(top_builddir)/zapit/clientlib/libzapitclient.la

zapit_SOURCES = \
	zapit.cpp transponder.cpp channel.cpp bouquets.cpp getservices.cpp scan.cpp


zapit_LDADD = \
	$(top_builddir)/libconfigfile/libconfigfile.la \
	$(top_builddir)/mczap/lcdd/clientlib/liblcddclient.la \
	$(top_builddir)/mczap/libevent/libeventserver.la \
	$(top_builddir)/zapit/clientlib/libzapitclient.la \
	$(top_builddir)/zapit/xml/libxml.a \
	$(top_builddir)/zapit/zapci/libzapci.a \
	$(top_builddir)/zapit/zapost/libzapost.a \
	$(top_builddir)/zapit/zapsi/libzapsi.a \
	-lpthread

#
# used CPPFLAGS are:
#
#  DBOX2 - enable dbox2 specific code (see Makefile.am in every subdir, too)
#  USE_PID_CACHE - do not read pids after every channel change
#  EXCESSIVE_TUNING_RETRIES - retry tuning on failure
#
AM_CPPFLAGS = -DDBOX2

install-data-local:
	$(INSTALL) -d $(CONFIGDIR)/zapit

