INCLUDES = \
	-I$(top_srcdir)/src \
	@FREETYPE_CFLAGS@ \
	@ZAPIT_CFLAGS@ \
	-I$(top_srcdir)/lib
 
noinst_LIBRARIES = libneutrino_pictureviewer.a

libneutrino_pictureviewer_a_SOURCES = \
			pictureviewer.cpp fb_display.cpp \
			png.cpp bmp.cpp resize.cpp jpeg.cpp gif.cpp crw.cpp
