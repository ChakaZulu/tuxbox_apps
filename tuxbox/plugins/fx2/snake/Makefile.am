INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = snake
noinst_LTLIBRARIES = snake.la

snake_la_SOURCES = snake.c somain.c

snake_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

snake_SOURCES = snake.c main.c

snake_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/snake.so $(PLUGINDIR)
	install -m 0644  snake.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/snake.so
	-rm $(PLUGINDIR)/snake.cfg
	-rmdir $(PLUGINDIR)
