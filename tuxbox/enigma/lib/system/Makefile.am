INCLUDES = \
	-I$(top_srcdir)/enigma/src \
	-I$(top_srcdir)/libxmltree

lib_LTLIBRARIES = libenigma_system.la

libenigma_system_la_SOURCES = \
	init.cpp elock.cpp http_dyn.cpp http_file.cpp httpd.cpp \
	mktime.c nconfig.cpp nxml.cpp xmlrpc.cpp econfig.cpp

AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

