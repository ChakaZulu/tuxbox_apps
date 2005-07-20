AM_CPPFLAGS = \
        -I$(srcdir)/../lib \
        -I$(top_srcdir)/include \
        -I$(top_srcdir)/../enigma/include \
        -I$(top_srcdir)/../enigma \
        -I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = dreamdata.la

dreamdata_la_SOURCES = dreamdata.cpp

dreamdata_la_LDFLAGS = -rpath $(PLUGINDIR) -module

dreamdata_la_CXXFLAGS = -fno-rtti -fno-exceptions $(SIGC_CFLAGS) $(XMLTREE_CFLAGS)

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	install .libs/dreamdata.so $(PLUGINDIR)
	install -m 0644 dreamdata.cfg $(PLUGINDIR)
	install -m 0644 dreamdata.xml $(CONFIGDIR)

uninstall-local:
	-rm $(PLUGINDIR)/dreamdata.so
	-rm $(PLUGINDIR)/dreamdata.cfg
	-rm $(CONFIGDIR)/dreamdata.xml
	-rmdir $(PLUGINDIR)
	-rmdir $(CONFIGDIR)

