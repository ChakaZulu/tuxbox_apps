INCLUDES = -I$(srcdir)/../lib \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/../enigma/include 

noinst_LTLIBRARIES = enigma_demo.la

enigma_demo_la_SOURCES = demo.cpp

enigma_demo_la_LDFLAGS = -rpath $(PLUGINDIR) -module

enigma_demo_la_CXXFLAGS = -fno-rtti -fno-exceptions $(SIGC_CFLAGS) $(XMLTREE_CFLAGS) 

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/enigma_demo.so $(PLUGINDIR)
	install -m 0644  enigma_demo.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/eingma_demo.so
	-rm $(PLUGINDIR)/enigma_demo.cfg
	-rmdir $(PLUGINDIR)
