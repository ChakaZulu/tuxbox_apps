SUBDIRS = driver

INCLUDES = 
	-I$(top_srcdir)/libs/libcontroldclient \
	-I$(top_srcdir)/libs/liblcddclient \
	@EVENTSERVER_CFLAGS@

bin_PROGRAMS = controld

controld_LDADD =
	driver/libcontrold_driver.a \
	$(top_builddir)/libs/liblcddclient/liblcddclient.la \
	@EVENTSERVER_LIBS@
	$(top_builddir)/zapit/clientlib/libzapitclient.la \
	-lpthread

controld_SOURCES = controld.cpp eventwatchdog.cpp

$(top_builddir)/mczap/lcdd/clientlib/liblcddclient.la:
	$(MAKE) -C $(top_builddir)/mczap/lcdd/clientlib

$(top_builddir)/mczap/libevent/libeventserver.la:
	$(MAKE) -C $(top_builddir)/mczap/libevent

$(top_builddir)/zapit/clientlib/libzapitclient.la:
	$(MAKE) -C $(top_builddir)/zapit/clientlib

install-data-local:
	install -d $(CONFIGDIR)
	install -m 644 $(srcdir)/scart.conf $(CONFIGDIR)

