INCLUDES = -I../lib

lib_LTLIBRARIES = libmines.la

libmines_la_SOURCES = board.c main.c somain.c

install-libLTLIBRARIES:
	install -d $(PLUGINDIR)
	install -m 0644  .libs/libmines.so $(PLUGINDIR)/mines.so
	install -m 0644  mines.cfg $(PLUGINDIR)/mines.cfg

uninstall-libLTLIBRARIES:
	-rm $(PLUGINDIR)/mines.so
	-rm $(PLUGINDIR)/mines.cfg
	-rmdir $(PLUGINDIR)
