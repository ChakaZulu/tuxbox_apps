INCLUDES = -I$(srcdir)/../lib

noinst_LTLIBRARIES = libmines.la

libmines_la_SOURCES = board.c main.c somain.c

libmines_la_LDFLAGS = -rpath $(PLUGINDIR)

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libmines.so $(PLUGINDIR)/mines.so
	install -m 0644  mines.cfg $(PLUGINDIR)/mines.cfg

uninstall-local:
	-rm $(PLUGINDIR)/mines.so
	-rm $(PLUGINDIR)/mines.cfg
	-rmdir $(PLUGINDIR)
