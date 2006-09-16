

INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/daemons/nhttpd30 \
	@FREETYPE_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@


noinst_LIBRARIES = libyhttpd.a

libyhttpd_a_SOURCES = \
	ylogging.cpp helper.cpp \
	ywebserver.cpp yconnection.cpp yrequest.cpp yresponse.cpp yhook.cpp ysocket.cpp

