INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/system

noinst_LTLIBRARIES = libdriver.la

libdriver_la_SOURCES = \
	eavswitch.cpp rc.cpp rcdbox.cpp rcdreambox.cpp streamwd.cpp

libdriver_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libdriver.la $(LIBDIR)/enigma

