INCLUDES = -I$(DRIVER)/include

noinst_LTLIBRARIES = libfx2.la

libfx2_la_SOURCES = draw.c pig.c rcinput.c math.c

libfx2_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libfx2.so ${PLUGINDIR}/libfx2.so
	-rm ${prefix}/lib/libfx2.so

uninstall-local:
	-rm $(PLUGINDIR)/libfx2.so
