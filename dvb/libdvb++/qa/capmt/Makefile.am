AM_CFLAGS = $(LIBDVBSIPP_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/include

bin_PROGRAMS = capmt

capmt_SOURCES = capmt.cpp

capmt_LDADD =				\
	$(top_srcdir)/src/libdvb++.la	\
	$(LIBDVBSIPP_LIBS)		\
	-lpthread

