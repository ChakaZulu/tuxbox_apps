lib_LTLIBRARIES = libflashimage.la

libflashimage_la_SOURCES = \
	flashimage.cpp \
	flashimagearchive.cpp

libflashimage_la_LIBADD = $(top_builddir)/libcrypto++/libcrypto++.la

noinst_PROGRAMS = test

test_SOURCES = test.cpp

test_LDADD = libflashimage.la

AM_CXXFLAGS = -fno-rtti

$(top_builddir)/libcrypto++/libcrypto++.la:
	$(MAKE) -C $(top_builddir)/libcrypto++

