INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = mines.la

mines_la_SOURCES = board.c somain.c

mines_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install mines.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/mines.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/mines.so
	-rm $(PLUGINDIR)/mines.cfg
	-rmdir $(PLUGINDIR)
