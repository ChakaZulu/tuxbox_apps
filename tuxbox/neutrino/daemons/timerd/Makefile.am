INCLUDES = \
	-I$(top_srcdir)/lib \
	@EVENTSERVER_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@

bin_PROGRAMS = timerd

timerd_SOURCES = timerd.cpp timermanager.cpp

timerd_LDADD = \
	@EVENTSERVER_LIBS@ \
	@ZAPITCLIENT_LIBS@ \
	-lpthread

