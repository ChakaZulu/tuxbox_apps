INCLUDES = -I../lib

bin_PROGRAMS = tetris
noinst_LTLIBRARIES = libtetris.la

libtetris_la_SOURCES = board.c main.c somain.c

libtetris_la_LDFLAGS = -rpath $(PLUGINDIR)

tetris_SOURCES = board.c main.c somain.c

tetris_LDADD = -lfx2

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libtetris.so $(PLUGINDIR)/tetris.so
	install -m 0644  tetris.cfg $(PLUGINDIR)/tetris.cfg

uninstall-local:
	-rm $(PLUGINDIR)/tetris.so
	-rm $(PLUGINDIR)/tetris.cfg
	-rmdir $(PLUGINDIR)
