INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = vierg.la

vierg_la_SOURCES = board.c somain.c

vierg_la_LIBADD = ../lib/libfx2.la

vierg_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install vierg.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/vierg.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/vierg.so
	-rm $(PLUGINDIR)/vierg.cfg
	-rmdir $(PLUGINDIR)
