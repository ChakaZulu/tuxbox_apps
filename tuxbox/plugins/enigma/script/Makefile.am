AM_CPPFLAGS = \
        -I$(srcdir)/../lib \
        -I$(top_srcdir)/include \
        -I$(top_srcdir)/../enigma/include \
        -I$(top_srcdir)/../enigma \
        -I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = script.la

script_la_SOURCES = script.cpp

script_la_LDFLAGS = -rpath $(PLUGINDIR) -module

script_la_CXXFLAGS = -fno-rtti -fno-exceptions $(SIGC_CFLAGS) $(XMLTREE_CFLAGS)

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/script.so $(PLUGINDIR)
	install -m 0644 script.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/script.so
	-rm $(PLUGINDIR)/script.cfg
	-rmdir $(PLUGINDIR)

