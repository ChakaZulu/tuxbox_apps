AM_CPPFLAGS = \
	-I$(srcdir)/../lib \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/../enigma/include \
	-I$(top_srcdir)/../enigma \
	-I$(top_srcdir)/../enigma/src

noinst_LTLIBRARIES = dslconnect.la

dslconnect_la_SOURCES = dslconnect.cpp

dslconnect_la_LDFLAGS = -rpath $(PLUGINDIR) -module

dslconnect_la_CXXFLAGS = $(SIGC_CFLAGS) $(XMLTREE_CFLAGS) -fno-exceptions -fno-rtti

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/dslconnect.so $(PLUGINDIR)
	install -m 0644 dslconnect.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/dslconnect.so
	-rm $(PLUGINDIR)/dslconnect.cfg
	-rmdir $(PLUGINDIR)
