INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = tank.la

tank_la_SOURCES = board.c somain.c

tank_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install tank.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/tank.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/tank.so
	-rm $(PLUGINDIR)/tank.cfg
	-rmdir $(PLUGINDIR)
