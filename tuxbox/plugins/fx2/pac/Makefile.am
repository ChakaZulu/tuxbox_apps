INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = pacman
noinst_LTLIBRARIES = pacman.la

pacman_la_SOURCES = maze.c somain.c

pacman_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

pacman_SOURCES = maze.c main.c

pacman_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/pacman.so $(PLUGINDIR)
	install -m 0644  pacman.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/pacman.so
	-rm $(PLUGINDIR)/pacman.cfg
	-rmdir $(PLUGINDIR)
