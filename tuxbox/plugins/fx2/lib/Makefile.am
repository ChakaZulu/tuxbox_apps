INCLUDES = -I$(DRIVER)/include

noinst_LTLIBRARIES = libfx2.la

libfx2_la_SOURCES = draw.c pig.c rcinput.c math.c

libfx2_la_LDFLAGS = -rpath ${prefix}/lib

install-exec-local:
	install -d ${prefix}/lib
	install .libs/libfx2.so ${prefix}/lib/libfx2.so

uninstall-local:
	-rm ${prefix}/lib/libfx2.so
