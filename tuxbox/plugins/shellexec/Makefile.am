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

noinst_LTLIBRARIES = shellexec.la

shellexec_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

shellexec_la_SOURCES = \
	start.c

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	$(LIBTOOL) --mode=install install shellexec.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/shellexec.cfg $(DESTDIR)$(PLUGINDIR)
