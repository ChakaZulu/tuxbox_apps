SUBDIRS = skins

AM_CPPFLAGS = -Ienigma

bin_PROGRAMS = bootmenue

bootmenue_SOURCES = processutils.cpp my_fb.cpp my_rc.cpp my_lcd.cpp png.cpp my_timer.cpp bootmenue.cpp

bootmenue_LDADD = \
	@SIGC_LIBS@ \
	@PNG_LIBS@ \
	@FREETYPE_LIBS@
