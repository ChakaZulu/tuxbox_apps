INCLUDES = \
	-I$(top_srcdir)/enigma/src/include \
	-I$(top_srcdir)/enigma/src \
	-I$(top_srcdir)/enigma/src/core/system \
	-I$(top_srcdir)/enigma/src/core/base

lib_LTLIBRARIES = libenigma_socket.la

libenigma_socket_la_SOURCES = \
	serversocket.cpp serversocket.moc.cpp socket.cpp socket.moc.cpp

AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

%.moc.cpp: %.h
	$(MOC) $< -o $@
