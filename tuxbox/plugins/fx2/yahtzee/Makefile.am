INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = yahtzee.la

yahtzee_la_SOURCES = yahtzee.c somain.c

yahtzee_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install yahtzee.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/yahtzee.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/yahtzee.so
	-rm $(PLUGINDIR)/yahtzee.cfg
	-rmdir $(PLUGINDIR)
