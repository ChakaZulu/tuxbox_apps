INCLUDES = \
	-I$(top_srcdir)/lib \
        @CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@ZAPIT_CFLAGS@

lib_LTLIBRARIES  = libcontroldclient.la

libcontroldclient_la_SOURCES = controldclient.cpp

bin_PROGRAMS = controldc

controldc_SOURCES = controldc.cpp

controldc_LDADD = libcontroldclient.la @CONNECTION_LIBS@
