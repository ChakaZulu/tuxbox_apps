INCLUDES = \
	-I$(top_srcdir)/enigma/src/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src

lib_LTLIBRARIES = libenigma_socket.la

libenigma_socket_la_SOURCES = \
	qserversocket.cpp qserversocket.moc.cpp qsocket.cpp qsocket.moc.cpp

AM_CXXFLAGS = $(LIBSIGC_CFLAGS) -fno-rtti -fno-exceptions

%.moc.cpp: %.h
	$(MOC) $< -o $@
