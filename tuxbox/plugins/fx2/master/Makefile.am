INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = master.la

master_la_SOURCES = board.c somain.c

master_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/master.so $(PLUGINDIR)
	install -m 0644  master.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/master.so
	-rm $(PLUGINDIR)/master.cfg
	-rmdir $(PLUGINDIR)
