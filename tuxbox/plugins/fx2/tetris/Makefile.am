INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = tetris.la

tetris_la_SOURCES = board.c somain.c

tetris_la_LIBADD =       \
	../lib/libfx2.la \
	@CURL_LIBS@


tetris_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	$(LIBTOOL) install tetris.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tetris.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/games.cfg $(DESTDIR)$(CONFIGDIR)

uninstall-local:
	-rm $(DESTDIR)$(CONFIGDIR)/games.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/tetris.so
	-rm $(DESTDIR)$(PLUGINDIR)/tetris.cfg
	-rmdir $(DESTDIR)$(PLUGINDIR)
