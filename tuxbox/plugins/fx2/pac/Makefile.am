INCLUDES = -I../lib

noinst_LTLIBRARIES = libpac.la

libpac_la_SOURCES = main.c maze.c somain.c

libpac_la_LDFLAGS = -rpath $(PLUGINDIR)

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libpac.so $(PLUGINDIR)/pacman.so
	install -m 0644  pacman.cfg $(PLUGINDIR)/pacman.cfg

uninstall-local:
	-rm $(PLUGINDIR)/pacman.so
	-rm $(PLUGINDIR)/pacman.cfg
	-rmdir $(PLUGINDIR)
