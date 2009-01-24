if DISABLE_INTERNET_UPDATE
HTTPUpdate = 
else
HTTPUpdate = httptool.cpp
endif

INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@TUXTXT_CFLAGS@ \
	@XML_CFLAGS@

noinst_LIBRARIES = libneutrino_system.a

libneutrino_system_a_SOURCES = \
	localize.cpp setting_helpers.cpp debug.cpp \
	ping.c flashtool.cpp $(HTTPUpdate) \
	settings.cpp lastchannel.cpp \
	configure_network.cpp fsmounter.cpp helper.cpp
