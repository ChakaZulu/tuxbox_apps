SUBDIRS = channel descriptor hardware pool table

lib_LTLIBRARIES = libdvb++.la

libdvb___la_SOURCES = libdvb++.cpp

libdvb___la_LIBADD = \
	channel/libdvbchannel.la \
	descriptor/libdvbdescriptor.la \
	hardware/libdvbhardware.la \
	pool/libdvbpool.la \
	table/libdvbtable.la

libdvb___la_LDFLAGS = \
	-release $(LT_RELEASE)
