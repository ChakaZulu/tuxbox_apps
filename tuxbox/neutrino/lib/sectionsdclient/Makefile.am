INCLUDES = \
	-I$(top_srcdir)/lib \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@ZAPIT_CFLAGS@


lib_LTLIBRARIES  = libsectionsdclient.la

libsectionsdclient_la_SOURCES = sectionsdclient.cpp


bin_PROGRAMS = sectionsdcontrol

sectionsdcontrol_SOURCES = sectionsdcontrol.cpp
sectionsdcontrol_LDADD = \
	libsectionsdclient.la \
	@CONNECTION_LIBS@
