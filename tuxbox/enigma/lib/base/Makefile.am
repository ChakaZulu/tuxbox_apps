INCLUDES = \
	-I$(top_srcdir)/enigma/src/include \
	-I$(top_srcdir)/enigma/src/core/base

lib_LTLIBRARIES = libenigma_base.la

libenigma_base_la_SOURCES = \
	ebase.cpp erect.cpp esize.cpp epoint.cpp eptrlist.cpp message.cpp thread.cpp

AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

