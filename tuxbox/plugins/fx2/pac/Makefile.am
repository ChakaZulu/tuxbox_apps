INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = pac
noinst_LTLIBRARIES = libpac.la

libpac_la_SOURCES = maze.c somain.c

libpac_la_LDFLAGS = -rpath $(PLUGINDIR)

pac_SOURCES = maze.c main.c

pac_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libpac.so $(PLUGINDIR)/pacman.so
	install -m 0644  pacman.cfg $(PLUGINDIR)/pacman.cfg

uninstall-local:
	-rm $(PLUGINDIR)/pacman.so
	-rm $(PLUGINDIR)/pacman.cfg
	-rmdir $(PLUGINDIR)
