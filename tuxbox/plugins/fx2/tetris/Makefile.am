INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = tetris
noinst_LTLIBRARIES = tetris.la

tetris_la_SOURCES = board.c somain.c

tetris_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

tetris_SOURCES = board.c main.c

tetris_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/tetris.so $(PLUGINDIR)
	install -m 0644  tetris.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/tetris.so
	-rm $(PLUGINDIR)/tetris.cfg
	-rmdir $(PLUGINDIR)
