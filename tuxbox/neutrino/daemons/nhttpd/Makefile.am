INCLUDES = \
	@ZAPIT_CFLAGS@ \
	-I$(top_srcdir)/lib/include \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	-I@DRIVER@

bin_PROGRAMS = nhttpd

nhttpd_SOURCES = \
	webserver.cpp nhttpd.cpp webdbox.cpp helper.cpp \
	request.cpp webapi.cpp controlapi.cpp bouqueteditapi.cpp debug.cpp

nhttpd_LDADD = \
	$(top_builddir)/lib/controldclient/libcontroldclient.la \
	$(top_builddir)/lib/timerdclient/libtimerdclient.la \
	$(top_builddir)/lib/sectionsdclient/libsectionsdclient.la \
	@CONFIGFILE_LIBS@ \
	@MD5SUM_LIBS@ \
	@UCODES_LIBS@ \
	@ZAPIT_LIBS@ \
	-lpthread

