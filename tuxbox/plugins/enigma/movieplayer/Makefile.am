AM_CPPFLAGS = \
	-I$(srcdir)/../lib \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/../enigma/include \
	-I$(top_srcdir)/../enigma \
	-I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = movieplayer.la

movieplayer_la_SOURCES = movieplayer.cpp

movieplayer_la_LDFLAGS = -rpath $(PLUGINDIR) -module

movieplayer_la_CXXFLAGS = -fno-rtti -fno-exceptions $(SIGC_CFLAGS) $(XMLTREE_CFLAGS)

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	install .libs/movieplayer.so $(PLUGINDIR)
	install -m 0644 movieplayer.cfg $(PLUGINDIR)
	install -m 0644 movieplayer.xml $(CONFIGDIR)

uninstall-local:
	-rm $(PLUGINDIR)/movieplayer.so
	-rm $(PLUGINDIR)/movieplayer.cfg
	-rm $(CONFIGDIR)/movieplayer.xml
	-rmdir $(PLUGINDIR)
	-rmdir $(CONFIGDIR)

