AM_CPPFLAGS =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

noinst_LTLIBRARIES = shellexec.la

shellexec_la_LIBADD = \
	@FREETYPE_LIBS@

shellexec_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

shellexec_la_SOURCES = \
	shellexec.c \
	lcd.c \
	io.c \
	text.c \
	gfx.c

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	$(LIBTOOL) --mode=install install shellexec.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/shellexec.cfg $(DESTDIR)$(PLUGINDIR)
