INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = tetris
noinst_LTLIBRARIES = libtetris.la

libtetris_la_SOURCES = board.c somain.c

libtetris_la_LDFLAGS = -rpath $(PLUGINDIR)

tetris_SOURCES = board.c main.c

tetris_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libtetris.so $(PLUGINDIR)/tetris.so
	install -m 0644  tetris.cfg $(PLUGINDIR)/tetris.cfg

uninstall-local:
	-rm $(PLUGINDIR)/tetris.so
	-rm $(PLUGINDIR)/tetris.cfg
	-rmdir $(PLUGINDIR)
