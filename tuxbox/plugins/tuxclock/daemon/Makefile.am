bin_PROGRAMS = tuxclockd

tuxclockd_SOURCES = tuxclockd.c

tuxclockd_LDADD = \
	-lpthread \
	@FREETYPE_LIBS@

AM_CPPFLAGS = \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include
