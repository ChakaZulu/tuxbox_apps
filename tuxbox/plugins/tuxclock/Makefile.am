SUBDIRS = daemon

AM_CPPFLAGS = \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

noinst_LTLIBRARIES = tuxclock.la

tuxclock_la_SOURCES = tuxclock.c

tuxclock_la_LIBADD = \
	@FREETYPE_LIBS@

tuxclock_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	install -d $(DESTDIR)$(CONFIGDIR)/tuxclock
	$(LIBTOOL) --mode=install install tuxclock.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxclock.cfg $(DESTDIR)$(PLUGINDIR)
