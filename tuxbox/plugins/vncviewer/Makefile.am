INCLUDES = -I./rfb @CONFIGFILE_CFLAGS@

noinst_LTLIBRARIES = vnc.la

vnc_la_CFLAGS = -DPLUGIN
vnc_la_CXXFLAGS = -DPLUGIN

vnc_la_SOURCES = args.c fbdraw.c fbgl.c fbvnc.cpp fscale.c icons.c keyboard.c \
                 list.c overlay.c rfbproto.c sockets.c rfb/d3des.c  \
                 rfb/vncauth.c

vnc_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version @CONFIGFILE_LIBS@

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	$(LIBTOOL) install vnc.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/vnc.cfg $(PLUGINDIR)
	install -m 0644 $(srcdir)/vnc.conf $(CONFIGDIR)



