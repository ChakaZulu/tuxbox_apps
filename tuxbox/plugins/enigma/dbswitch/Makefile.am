AM_CPPFLAGS = \
        -I$(srcdir)/../lib \
        -I$(top_srcdir)/include \
        -I$(top_srcdir)/../enigma/include \
        -I$(top_srcdir)/../enigma \
        -I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = dbswitch.la

dbswitch_la_SOURCES = lcddisplay.cpp rcinput.cpp dbswitch.cpp

dbswitch_la_LDFLAGS = -rpath $(PLUGINDIR) -module

dbswitch_la_CXXFLAGS = -fno-rtti -fno-exceptions $(SIGC_CFLAGS) $(XMLTREE_CFLAGS)

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/dbswitch.so $(PLUGINDIR)
	install -m 0644 dbswitch.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/dbswitch.so
	-rm $(PLUGINDIR)/dbswitch.cfg
	-rmdir $(PLUGINDIR)



