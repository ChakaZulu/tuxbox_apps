INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = lemmings.la

lemmings_la_SOURCES = somain.c sprite.c lemm.c pic.c sound.c double.c

lemmings_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version @CURL_LIBS@ -lz

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install lemmings.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/lemmings.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/lemmings.so
	-rm $(PLUGINDIR)/lemmings.cfg
	-rmdir $(PLUGINDIR)
