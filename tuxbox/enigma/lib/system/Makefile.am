INCLUDES = \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/xml \
	-I$(top_srcdir)/enigma/src/apps/enigma \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/gui \
  -I$(includedir)/sigc++-1.0 \
	-I$(includedir)/../lib/sigc++-1.0/include

noinst_LTLIBRARIES = libsystem.la

libsystem_la_SOURCES = \
	init.cpp elock.cpp http_dyn.cpp http_file.cpp httpd.cpp \
	mktime.c nconfig.cpp nxml.cpp xmlrpc.cpp

libsystem_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libsystem.la $(LIBDIR)/enigma

clean-local: