INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = vierg.la

vierg_la_SOURCES = board.c somain.c

vierg_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/vierg.so $(PLUGINDIR)
	install -m 0644  vierg.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/vierg.so
	-rm $(PLUGINDIR)/vierg.cfg
	-rmdir $(PLUGINDIR)
