INCLUDES = -I../lib

lib_LTLIBRARIES = libpac.la

libpac_la_SOURCES = main.c maze.c somain.c

install-libLTLIBRARIES:
	install -d $(PLUGINDIR)
	install -m 0644  .libs/libpac.so $(PLUGINDIR)/

uninstall-libLTLIBRARIES:
	-rm $(PLUGINDIR)/libpac.so
	-rmdir $(PLUGINDIR)
