INCLUDES = -I$(DRIVER)/include -I$(top_srcdir)/include

noinst_LTLIBRARIES = tuxtxt.la

tuxtxt_la_SOURCES = tuxtxt.c

tuxtxt_la_LIBADD = \
	$(LIBFREETYPE_LIBS)
	-lpthread

tuxtxt_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

AM_CFLAGS = $(LIBFREETYPE_CFLAGS)

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	install .libs/tuxtxt.so $(PLUGINDIR)
	install -m 0644 tuxtxt.cfg $(PLUGINDIR)
	install -m 0644 font/tuxtxt.fon $(FONTDIR)

