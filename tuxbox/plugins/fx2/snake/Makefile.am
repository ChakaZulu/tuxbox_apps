INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = snake
noinst_LTLIBRARIES = libsnake.la

libsnake_la_SOURCES = snake.c somain.c

libsnake_la_LDFLAGS = -rpath $(PLUGINDIR)

snake_SOURCES = snake.c main.c

snake_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libsnake.so $(PLUGINDIR)/snake.so
	install -m 0644  snake.cfg $(PLUGINDIR)/snake.cfg

uninstall-local:
	-rm $(PLUGINDIR)/snake.so
	-rm $(PLUGINDIR)/snake.cfg
	-rmdir $(PLUGINDIR)
