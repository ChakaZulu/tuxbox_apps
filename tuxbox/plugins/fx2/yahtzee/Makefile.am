INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

bin_PROGRAMS = yahtzee
noinst_LTLIBRARIES = yahtzee.la

yahtzee_la_SOURCES = yahtzee.c somain.c

yahtzee_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

yahtzee_SOURCES = yahtzee.c main.c

yahtzee_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/yahtzee.so $(PLUGINDIR)
	install -m 0644  yahtzee.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/yahtzee.so
	-rm $(PLUGINDIR)/yahtzee.cfg
	-rmdir $(PLUGINDIR)
