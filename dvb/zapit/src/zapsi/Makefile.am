INCLUDES = -I$(top_srcdir)/include \
           @XML_CFLAGS@

noinst_LTLIBRARIES = libzapsi.la

libzapsi_la_SOURCES = bat.cpp descriptors.cpp nit.cpp pat.cpp pmt.cpp sdt.cpp

AM_CPPFLAGS = -pedantic -Werror
AM_CXXFLAGS = -fno-rtti -fno-exceptions
