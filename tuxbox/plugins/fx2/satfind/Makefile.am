INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include -I$(DRIVER)/include

noinst_LTLIBRARIES = satfind.la

satfind_la_SOURCES = satfind.c

satfind_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install satfind.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/satfind.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/satfind.so
	-rm $(PLUGINDIR)/satfind.cfg
	-rmdir $(PLUGINDIR)
