INCLUDES = -I../lib

bin_PROGRAMS = tetris
lib_LTLIBRARIES = libtetris.la

libtetris_la_SOURCES = board.c main.c somain.c

tetris_SOURCES = board.c main.c somain.c

tetris_LDADD = -lfx2

install-libLTLIBRARIES:
	install -d $(PLUGINDIR)
	install -m 0644  .libs/libtetris.so $(PLUGINDIR)/

uninstall-libLTLIBRARIES:
	-rm $(PLUGINDIR)/libtetris.so
	-rmdir $(PLUGINDIR)
