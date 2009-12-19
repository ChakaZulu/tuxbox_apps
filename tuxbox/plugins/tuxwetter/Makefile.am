SUBDIRS = bitmap

AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	@CURL_CFLAGS@ \
	@PNG_CFLAGS@ \
	-I$(top_srcdir)/include

tuxwetter_LDADD = \
	@FREETYPE_LIBS@ \
	@CURL_LIBS@ \
	@PNG_LIBS@ \
	-ljpeg \
	-lungif

bin_PROGRAMS = tuxwetter

tuxwetter_SOURCES = \
	tuxwetter.c \
	parser.c \
	bmps.c \
	lcd.c \
	io.c \
	text.c \
	gfx.c \
	jpeg.c \
	gif.c \
	gifdecomp.c \
	pngw.c \
	php.c \
	fb_display.c \
	resize.c \
	http.c

noinst_LTLIBRARIES = tuxwetter.la

tuxwetter_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

tuxwetter_la_SOURCES = \
	start.c

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	install -d $(DESTDIR)$(CONFIGDIR)/tuxwetter
	$(LIBTOOL) --mode=install install tuxwetter.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxwetter.cfg $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxwetter.conf $(DESTDIR)$(CONFIGDIR)/tuxwetter
	install -m 0644 $(srcdir)/convert.list $(DESTDIR)$(CONFIGDIR)/tuxwetter
