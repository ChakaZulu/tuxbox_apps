INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = sol.la

sol_la_SOURCES = solboard.c somain.c

sol_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install sol.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/sol.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/sol.so
	-rm $(PLUGINDIR)/sol.cfg
	-rmdir $(PLUGINDIR)
