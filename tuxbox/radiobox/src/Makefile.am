## Process this file with automake to produce Makefile.in

## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

INCLUDES = \
	 -I../include \
	@FREETYPE_CFLAGS@ \
	@LCDDISPLAY_CFLAGS@ \
	-I$(top_srcdir)/../../../driver/include \
	-I$(top_srcdir)/../neutrino/src/driver

AM_CXXFLAGS =\
	 -Wall \
	-O2 -DRADIOBOX
#\
#	 -g

bin_PROGRAMS = radiobox

radiobox_SOURCES = \
	main.cc\
	audiofile.cpp\
	audiometadata.cpp\
	audioplay.cpp\
	basedec.cpp\
	cdrdec.cpp\
	file.cpp\
	mp3dec.cpp\
	$(top_srcdir)/../neutrino/src/driver/netfile.cpp \
	playlist.cpp\
	radiobox.cpp\
	wavdec.cpp\
	tag.c\
	statehandler.cpp \
	lcdd.cpp \
	rcinput.cpp \
	flashtool.cpp

radiobox_LDFLAGS = 

radiobox_LDADD = \
	 -lpthread -lmad -lid3tag -llcddisplay -lfreetype -lpng12 -ltuxbox-cramfs

