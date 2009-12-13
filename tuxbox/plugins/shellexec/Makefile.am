AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

bin_PROGRAMS = shellexec

shellexec_LDADD = \
	@FREETYPE_LIBS@

shellexec_SOURCES = \
	shellexec.c \
	lcd.c \
	io.c \
	text.c \
	gfx.c
