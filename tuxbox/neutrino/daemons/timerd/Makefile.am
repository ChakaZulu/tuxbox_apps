INCLUDES = \
	-I@DRIVER@/include \
	-I$(top_srcdir)/libs/libtimerdclient \
	@EVENTSERVER_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@

bin_PROGRAMS = timerd

timerd_SOURCES = timerd.cpp timermanager.cpp

timerd_LDADD = \
	@EVENTSERVER_LIBS@ \
	@ZAPITCLIENT_LIBS@
	-lpthread

