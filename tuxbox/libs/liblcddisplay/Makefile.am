INCLUDES = -I@DRIVER@/lcd

lib_LTLIBRARIES = liblcddisplay.la

tuxboxincludedir = ${includedir}/tuxbox
tuxboxinclude_HEADERS = fontrenderer.h lcddisplay.h

liblcddisplay_la_SOURCES = lcddisplay.cpp fontrenderer.cpp

AM_CXXFLAGS = @FREETYPE_CFLAGS@
