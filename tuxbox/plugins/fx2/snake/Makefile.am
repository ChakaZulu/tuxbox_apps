INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = snake.la

snake_la_SOURCES = snake.c somain.c

snake_la_LIBADD = ../lib/libfx2.la

snake_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install snake.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/snake.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/snake.so
	-rm $(PLUGINDIR)/snake.cfg
	-rmdir $(PLUGINDIR)
