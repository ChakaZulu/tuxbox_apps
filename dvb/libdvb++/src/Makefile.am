SUBDIRS = channel hardware pool

lib_LTLIBRARIES = libdvb++.la

libdvb___la_SOURCES = libdvb++.cpp

libdvb___la_LIBADD = \
	channel/libdvbchannel.la \
	hardware/libdvbhardware.la \
	pool/libdvbpool.la

libdvb___la_LDFLAGS = \
	-release $(LT_RELEASE)
