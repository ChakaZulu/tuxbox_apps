AM_CPPFLAGS = -I$(srcdir)/../lib \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/../enigma/include \
	-I$(top_srcdir)/../enigma \
	-I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = enigma_getset.la

enigma_getset_la_SOURCES = getset.cpp

enigma_getset_la_LDFLAGS = -rpath $(PLUGINDIR) -module

enigma_getset_la_CXXFLAGS = -fno-rtti -fno-exceptions $(SIGC_CFLAGS) $(XMLTREE_CFLAGS) 

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/enigma_getset.so $(PLUGINDIR)
	install -m 0644  enigma_getset.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/enigma_getset.so
	-rm $(PLUGINDIR)/enigma_getset.cfg
	-rmdir $(PLUGINDIR)
