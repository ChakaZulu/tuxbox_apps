INCLUDES = -I$(DRIVER)/include

noinst_LTLIBRARIES = libfx2.la

libfx2_la_SOURCES = draw.c pig.c rcinput.c math.c

libfx2_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install libfx2.la $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/libfx2.so
