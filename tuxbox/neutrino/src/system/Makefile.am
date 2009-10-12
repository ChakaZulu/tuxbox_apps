INCLUDES = \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@XML_CFLAGS@

if ENABLE_INTERNAL_TUXTXT
INCLUDES += \
	@TUXTXT_CFLAGS@
endif

noinst_LIBRARIES = libneutrino_system.a

libneutrino_system_a_SOURCES = \
	localize.cpp setting_helpers.cpp debug.cpp \
	ping.c flashtool.cpp \
	settings.cpp lastchannel.cpp \
	configure_network.cpp helper.cpp

if !DISABLE_INTERNET_UPDATE
libneutrino_system_a_SOURCES += httptool.cpp
endif

if ENABLE_GUI_MOUNT
libneutrino_system_a_SOURCES += fsmounter.cpp
endif
