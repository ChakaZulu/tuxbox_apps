INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = tetris.la

tetris_la_SOURCES = board.c somain.c

tetris_la_LIBADD =       \
	../lib/libfx2.la \
	@CURL_LIBS@


tetris_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	$(LIBTOOL) install tetris.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/tetris.cfg $(PLUGINDIR)
	install -m 0644 $(srcdir)/games.cfg $(CONFIGDIR)

uninstall-local:
	-rm $(CONFIGDIR)/games.cfg
	-rm $(PLUGINDIR)/tetris.so
	-rm $(PLUGINDIR)/tetris.cfg
	-rmdir $(PLUGINDIR)
