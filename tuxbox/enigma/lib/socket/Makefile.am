INCLUDES = \
	-I$(top_srcdir)/enigma/src

lib_LTLIBRARIES = libenigma_socket.la

libenigma_socket_la_SOURCES = \
	serversocket.cpp socket.cpp
	
AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions