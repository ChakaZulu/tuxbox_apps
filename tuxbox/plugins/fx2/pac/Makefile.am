INCLUDES = -I../lib

lib_LTLIBRARIES = libpac.la

libpac_la_SOURCES = main.c maze.c somain.c

install-libLTLIBRARIES:
	install -d $(PLUGINDIR)
	install -m 0644  .libs/libpac.so $(PLUGINDIR)/pacman.so

uninstall-libLTLIBRARIES:
	-rm $(PLUGINDIR)/pacman.so
	-rmdir $(PLUGINDIR)
