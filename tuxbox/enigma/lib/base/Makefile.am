INCLUDES = \
	-I$(top_srcdir)/enigma/src/include

noinst_LTLIBRARIES = libbase.la

libbase_la_SOURCES = \
	ebase.cpp

AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

libbase_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libbase.la $(LIBDIR)/enigma
