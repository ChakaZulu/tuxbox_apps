SUBDIRS = font

noinst_LTLIBRARIES = tuxtxt.la

if BOXTYPE_DREAMBOX
tuxtxt_la_CFLAGS = -DDREAMBOX

AM_CPPFLAGS = \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

tuxtxt_la_LIBADD = \
	@FREETYPE_LIBS@ \
	-lpthread

else

AM_CPPFLAGS = \
	@TUXBOX_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

tuxtxt_la_LIBADD = \
	@TUXBOX_LIBS@  \
	@FREETYPE_LIBS@ \
	-lpthread

endif

tuxtxt_la_SOURCES = tuxtxt.c

tuxtxt_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	install -d $(DESTDIR)$(CONFIGDIR)/tuxtxt
	$(LIBTOOL) --mode=install install tuxtxt.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxtxt.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxtxt.conf $(DESTDIR)$(CONFIGDIR)/tuxtxt
