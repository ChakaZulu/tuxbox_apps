SUBDIRS = font

INCLUDES = \
	@FREETYPE_CFLAGS@ \
	-I@DRIVER@ \
	-I$(top_srcdir)/include

noinst_LTLIBRARIES = tuxtxt.la

tuxtxt_la_SOURCES = tuxtxt.c

tuxtxt_la_LIBADD = \
	 @FREETYPE_LIBS@ \
	-lpthread

tuxtxt_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	install -d $(CONFIGDIR)/tuxtxt
	install .libs/tuxtxt.so $(PLUGINDIR)
	install -m 0644 tuxtxt.cfg $(PLUGINDIR)
	install -m 0644 tuxtxt.conf $(CONFIGDIR)/tuxtxt
