INCLUDES = -I../lib

lib_LTLIBRARIES = libmines.la

libmines_la_SOURCES = board.c main.c somain.c

install-libLTLIBRARIES:
	install -d $(PLUGINDIR)
	install -m 0644  .libs/libmines.so $(PLUGINDIR)/

uninstall-libLTLIBRARIES:
	-rm $(PLUGINDIR)/libmines.so
	-rmdir $(PLUGINDIR)
