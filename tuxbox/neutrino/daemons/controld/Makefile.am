SUBDIRS = driver

INCLUDES = \
	-I$(top_srcdir)/lib \
	@EVENTSERVER_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@

bin_PROGRAMS = controld

controld_LDADD = \
	driver/libcontrold_driver.a \
	$(top_builddir)/lib/lcddclient/liblcddclient.la
	@EVENTSERVER_LIBS@ \
	@ZAPITCLIENT_LIBS@ \
	-lpthread

controld_SOURCES = controld.cpp eventwatchdog.cpp

install-data-local:
	install -d $(CONFIGDIR)
	install -m 644 $(srcdir)/scart.conf $(CONFIGDIR)

