INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = master.la

master_la_SOURCES = board.c somain.c

master_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install master.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/master.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/master.so
	-rm $(PLUGINDIR)/master.cfg
	-rmdir $(PLUGINDIR)
