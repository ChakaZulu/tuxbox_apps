INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = soko.la

soko_la_SOURCES = board.c somain.c

soko_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(DATADIR)/sokoban
	$(LIBTOOL) install soko.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/soko.cfg $(PLUGINDIR)
	install -m 0644 $(srcdir)/*.xsb $(DATADIR)/sokoban

uninstall-local:
	-rm $(PLUGINDIR)/soko.so
	-rm $(PLUGINDIR)/soko.cfg
	-rm $(DATADIR)/sokoban/*.xsb
	-rmdir $(PLUGINDIR)
	-rmdir $(DATADIR)/sokoban
