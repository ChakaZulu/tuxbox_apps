INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = soko.la

soko_la_SOURCES = board.c somain.c

soko_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(DATADIR)/sokoban
	$(LIBTOOL) install soko.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/soko.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/*.xsb $(DESTDIR)$(DATADIR)/sokoban

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/soko.so
	-rm $(DESTDIR)$(PLUGINDIR)/soko.cfg
	-rm $(DESTDIR)$(DATADIR)/sokoban/*.xsb
	-rmdir $(DESTDIR)$(PLUGINDIR)
	-rmdir $(DESTDIR)$(DATADIR)/sokoban
