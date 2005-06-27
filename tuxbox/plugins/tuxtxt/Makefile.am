SUBDIRS = font 

INCLUDES = \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include \
	-I$(includedir)/tuxbox/tuxtxt 

noinst_LTLIBRARIES = tuxtxt.la

tuxtxt_la_SOURCES = tuxtxt.c

tuxtxt_la_LIBADD = \
	@FREETYPE_LIBS@
#@TUXTXT_LIBS@ 


tuxtxt_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	install -d $(DESTDIR)$(CONFIGDIR)/tuxtxt
	$(LIBTOOL) install tuxtxt.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxtxt.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxtxt.conf $(DESTDIR)$(CONFIGDIR)/tuxtxt
