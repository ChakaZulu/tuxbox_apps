INCLUDES = \
	-I$(top_srcdir)/lib \
	@EVENTSERVER_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	-I@DRIVER@

bin_PROGRAMS = nhttpd

nhttpd_SOURCES = \
	webserver.cpp nhttpd.cpp webdbox.cpp helper.cpp \
	request.cpp webapi.cpp controlapi.cpp bouqueteditapi.cpp debug.cpp

nhttpd_LDADD = \
	@CONFIGFILE_LIBS@ \
	@MD5SUM_LIBS@ \
	@UCODES_LIBS@ \
	-lpthread

