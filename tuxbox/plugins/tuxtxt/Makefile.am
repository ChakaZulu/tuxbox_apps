SUBDIRS = font

INCLUDES = \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

noinst_LTLIBRARIES = tuxtxt.la

#tuxtxt_la_CFLAGS = -DDREAMBOX

tuxtxt_la_SOURCES = tuxtxt.c

tuxtxt_la_LIBADD = \
	 @FREETYPE_LIBS@ \
	-lpthread

tuxtxt_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	install -d $(CONFIGDIR)/tuxtxt
	$(LIBTOOL) install tuxtxt.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxtxt.cfg $(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxtxt.conf $(CONFIGDIR)/tuxtxt
