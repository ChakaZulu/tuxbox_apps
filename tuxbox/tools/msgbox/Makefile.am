AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

bin_PROGRAMS = msgbox

msgbox_LDADD = \
	@FREETYPE_LIBS@

msgbox_SOURCES = \
	msgbox.c \
	io.c \
	text.c \
	gfx.c \
	txtform.c
