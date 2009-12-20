INCLUDES = \
	@FREETYPE_CFLAGS@ \
	@TUXBOX_CFLAGS@ \
	-I$(top_srcdir)/include

sysinfo_LDADD = \
	@FREETYPE_LIBS@ \
	@TUXBOX_LIBS@ \
	@MD5SUM_LIBS@ \
	@UCODES_LIBS@

bin_PROGRAMS = sysinfo

sysinfo_SOURCES = \
	sysinfo.c \
	text.c \
	gfx.c

noinst_LTLIBRARIES = sysinfo.la

sysinfo_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

sysinfo_la_SOURCES = \
	start.c

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	$(LIBTOOL) --mode=install install sysinfo.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/sysinfo.cfg $(DESTDIR)$(PLUGINDIR)
