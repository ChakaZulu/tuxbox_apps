INCLUDES = \
	-I$(srcdir)/../lib \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/../enigma/include \
	-I$(top_srcdir)/../enigma \
	-I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = ngrabstart.la

ngrabstart_la_SOURCES = ngrabstart.cpp

ngrabstart_la_LDFLAGS = -rpath $(PLUGINDIR) -module -pthread

ngrabstart_la_CXXFLAGS = -fno-exceptions -fno-rtti $(SIGC_CFLAGS) $(XMLTREE_CFLAGS)

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/ngrabstart.so $(PLUGINDIR)
	install -m 0644 ngrabstart.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/ngrabstart.so
	-rm $(PLUGINDIR)/ngrabstart.cfg
	-rmdir $(PLUGINDIR)
