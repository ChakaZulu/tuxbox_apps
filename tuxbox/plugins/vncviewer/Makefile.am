SUBDIRS = rfb

AM_CPPFLAGS = -I./rfb @CONFIGFILE_CFLAGS@ -I$(top_srcdir)/include

noinst_LTLIBRARIES = vnc.la

vnc_la_SOURCES = args.c fbdraw.c fscale.c fbgl.c icons.c keyboard.c \
                 list.c overlay.c rfbproto.c sockets.c fbvnc.cpp 

vnc_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version @CONFIGFILE_LIBS@

vnc_la_LIBADD = rfb/librfb.a

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)
	install -d $(DESTDIR)$(CONFIGDIR)
	$(LIBTOOL) --mode=install install vnc.la $(DESTDIR)$(PLUGINDIR)
	install -m 0644 $(srcdir)/vnc.cfg $(DESTDIR)$(PLUGINDIR)
	if test ! -e  $(DESTDIR)$(CONFIGDIR)/vnc.conf; then cp $(srcdir)/vnc.conf $(DESTDIR)$(CONFIGDIR);fi
