INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include -I$(DRIVER)/include

noinst_LTLIBRARIES = outdoor.la

outdoor_la_SOURCES = outdoor.c

outdoor_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install outdoor.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/outdoor.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/outdoor.so
	-rm $(PLUGINDIR)/outdoor.cfg
	-rmdir $(PLUGINDIR)
