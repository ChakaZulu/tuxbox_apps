INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include -I$(DRIVER)/include

noinst_LTLIBRARIES = lcdcirc.la

lcdcirc_la_SOURCES = lcdcirc.c

lcdcirc_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install lcdcirc.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/lcdcirc.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/lcdcirc.so
	-rm $(PLUGINDIR)/lcdcirc.cfg
	-rmdir $(PLUGINDIR)
