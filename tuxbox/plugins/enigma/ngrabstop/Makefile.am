AM_CPPFLAGS = \
	-I$(srcdir)/../lib \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/../enigma/include \
	-I$(top_srcdir)/../enigma \
	-I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = ngrabstop.la

ngrabstop_la_SOURCES = ngrabstop.cpp

ngrabstop_la_LDFLAGS = -rpath $(PLUGINDIR) -module -pthread

ngrabstop_la_CXXFLAGS = $(SIGC_CFLAGS) $(XMLTREE_CFLAGS) -fno-exceptions -fno-rtti

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/ngrabstop.so $(PLUGINDIR)
	install -m 0644 ngrabstop.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/ngrabstop.so
	-rm $(PLUGINDIR)/ngrabstop.cfg
	-rmdir $(PLUGINDIR)
