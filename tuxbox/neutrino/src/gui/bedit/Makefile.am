INCLUDES = \
	-I$(top_srcdir)/daemons \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPITCLIENT_CFLAGS@ \
	-I@DRIVER@/include

noinst_LIBRARIES = libneutrino_gui_bedit.a

libneutrino_gui_bedit_a_SOURCES = \
	bouqueteditor_bouquets.cpp \
	bouqueteditor_channels.cpp \
	bouqueteditor_chanselect.cpp

AM_CPPFLAGS = -D_GNU_SOURCE

