SUBDIRS = data data-alt2

INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@ZAPIT_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@

bin_PROGRAMS = nhttpd

nhttpd_SOURCES = \
	controlapi.cpp bouqueteditapi.cpp debug.cpp helper.cpp \
	nhttpd.cpp request.cpp webapi.cpp webdbox.cpp webserver.cpp

nhttpd_LDADD = \
	$(top_builddir)/lib/controldclient/libcontroldclient.la \
	$(top_builddir)/lib/sectionsdclient/libsectionsdclient.la \
	$(top_builddir)/lib/timerdclient/libtimerdclient.la \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@ \
	@ZAPIT_LIBS@ \
	-lpthread

$(top_builddir)/lib/controldclient/libcontroldclient.la:
	$(MAKE) -C $(top_builddir)/lib/controldclient

$(top_builddir)/lib/timerdclient/libtimerdclient.la:
	$(MAKE) -C $(top_builddir)/lib/timerdclient

$(top_builddir)/lib/sectionsdclient/libsectionsdclient.la:
	$(MAKE) -C $(top_builddir)/lib/sectionsdclient
