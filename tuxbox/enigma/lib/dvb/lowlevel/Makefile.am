noinst_LTLIBRARIES = liblowlevel.la

liblowlevel_la_SOURCES = decode.cpp

liblowlevel_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

AM_FLAGS_CXX = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) liblowlevel.la $(LIBDIR)/enigma

