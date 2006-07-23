## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = \
	 -I../include \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@LCDDISPLAY_CFLAGS@ \
	@CONFIGFILE_CFLAGS@ \
	@NET_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	@TUXTXT_CFLAGS@ \
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
	main.cc\
	audiofile.cpp \
	audiometadata.cpp\
	audioplay.cpp\
	basedec.cpp\
	cdrdec.cpp\
	file.cpp\
	mp3dec.cpp\
	$(top_srcdir)/../neutrino/src/driver/netfile.cpp \
	$(top_srcdir)/../neutrino/src/system/fsmounter.cpp \
	playlist.cpp\
	radiobox.cpp\
	wavdec.cpp\
	tag.c\
	statehandler.cpp \
	lcdd.cpp \
	rcinput.cpp \
	flashtool.cpp \
	settings.cpp \
	mounter.cpp

radiobox_LDFLAGS = 

radiobox_LDADD = \
	-lpthread\
	@CONFIGFILE_LIBS@ \
	@LCDDISPLAY_LIBS@ \
	@ID3TAG_LIBS@ \
	@MAD_LIBS@ \
	@FREETYPE_LIBS@ \
	@CRAMFS_LIBS@ \
	@PNG_LIBS@


