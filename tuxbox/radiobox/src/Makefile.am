## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = \
	 -I. \
	 -I./engine \
	 -I./gui \
	 -I./audio \
	 -I./menus \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@LCDDISPLAY_CFLAGS@ \
	@CONFIGFILE_CFLAGS@ \
	@NET_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/../../../driver/include \
	-I$(top_srcdir)/../neutrino/src/driver \
	-I$(top_srcdir)/../neutrino/src


AM_CXXFLAGS =\
	 -Wall \
	-O2 -DRADIOBOX \
	 -g

bin_PROGRAMS = radiobox

radiobox_SOURCES = \
	main.cc \
	$(top_srcdir)/../neutrino/src/driver/netfile.cpp \
	$(top_srcdir)/../neutrino/src/system/fsmounter.cpp \
	$(top_srcdir)/src/engine/playlist.cpp \
	engine/radiobox.cpp \
	engine/settings.cpp \
	engine/flashtool.cpp \
	engine/rcinput.cpp \
	engine/file.cpp \
	engine/rbxinput.cpp \
	menus/statehandler.cpp \
	menus/mounter.cpp \
	audio/mp3dec.cpp \
	audio/wavdec.cpp \
	audio/cdrdec.cpp \
	audio/basedec.cpp \
	audio/audiofile.cpp \
	audio/audiometadata.cpp \
	audio/audioplay.cpp \
	audio/tag.c \
	gui/lcdd.cpp

radiobox_LDFLAGS = 

radiobox_LDADD = \
	-lpthread \
	@CONFIGFILE_LIBS@ \
	@LCDDISPLAY_LIBS@ \
	@ID3TAG_LIBS@ \
	@MAD_LIBS@ \
	@FREETYPE_LIBS@ \
	@CRAMFS_LIBS@ \
	@PNG_LIBS@


