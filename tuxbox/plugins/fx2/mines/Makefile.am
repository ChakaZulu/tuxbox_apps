INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = mines.la

mines_la_SOURCES = board.c somain.c

mines_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) install mines.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/mines.cfg $(DESTDIR)$(PLUGINDIR)

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/mines.so
	-rm $(DESTDIR)$(PLUGINDIR)/mines.cfg
	-rmdir $(DESTDIR)$(PLUGINDIR)
