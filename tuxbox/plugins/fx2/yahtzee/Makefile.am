INCLUDES = -I$(srcdir)/../lib

bin_PROGRAMS = yahtzee
noinst_LTLIBRARIES = libyahtzee.la

libyahtzee_la_SOURCES = yahtzee.c somain.c

libyahtzee_la_LDFLAGS = -rpath $(PLUGINDIR)

yahtzee_SOURCES = yahtzee.c main.c

yahtzee_LDADD = ../lib/libfx2.la

install-exec-local:
	install -d $(PLUGINDIR)
	install .libs/libyahtzee.so $(PLUGINDIR)/yahtzee.so
	install -m 0644  yahtzee.cfg $(PLUGINDIR)/yahtzee.cfg

uninstall-local:
	-rm $(PLUGINDIR)/yahtzee.so
	-rm $(PLUGINDIR)/yahtzee.cfg
	-rmdir $(PLUGINDIR)
