INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = lemmings.la

lemmings_la_SOURCES = somain.c sprite.c lemm.c pic.c

lemmings_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version $(LIBCURL_LIBS)

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/lemmings.so $(PLUGINDIR)
	install -m 0644  lemmings.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/lemmings.so
	-rm $(PLUGINDIR)/lemmings.cfg
	-rmdir $(PLUGINDIR)
