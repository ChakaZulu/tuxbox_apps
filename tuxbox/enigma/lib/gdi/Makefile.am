INCLUDES = \
	-I$(top_srcdir)/include \
	-I@DRIVER@/lcd \
	-I@DRIVER@/include/dbox

lib_LTLIBRARIES = libenigma_gdi.la

libenigma_gdi_la_SOURCES = \
	epng.cpp fb.cpp font.cpp gfbdc.cpp glcddc.cpp gpixmap.cpp grc.cpp \
	lcd.cpp

libenigma_gdi_la_LIBADD = \
	@FREETYPE_LIBS@ \
	@PNG_LIBS@

