INCLUDES = -I../lib

lib_LTLIBRARIES = libpac.la

libpac_la_SOURCES = main.c maze.c somain.c

install-libLTLIBRARIES:
	install -d $(PLUGINDIR)
	install -m 0644  .libs/libpac.so $(PLUGINDIR)/pacman.so
	install -m 0644  pacman.cfg $(PLUGINDIR)/pacman.cfg

uninstall-libLTLIBRARIES:
	-rm $(PLUGINDIR)/pacman.so
	-rm $(PLUGINDIR)/pacman.cfg
	-rmdir $(PLUGINDIR)
