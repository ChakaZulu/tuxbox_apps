INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = mines
noinst_LTLIBRARIES = mines.la

mines_la_SOURCES = board.c somain.c

mines_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

mines_SOURCES = board.c main.c

mines_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/mines.so $(PLUGINDIR)
	install -m 0644  mines.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/mines.so
	-rm $(PLUGINDIR)/mines.cfg
	-rmdir $(PLUGINDIR)
