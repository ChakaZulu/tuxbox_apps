INCLUDES = -I$(srcdir)/../lib -I$(top_srcdir)/include

noinst_LTLIBRARIES = solitair.la

solitair_la_SOURCES = \
	Block.cpp \
	Buffer.cpp \
	Card.cpp \
	Foundation.cpp \
	Hand.cpp \
	Slot.cpp \
	Table.cpp \
	Tableau.cpp \
	Wastepile.cpp \
	somain.cpp \
	pnm_file.cpp \
	pnm_res.cpp \
	backbuffer.cpp

solitair_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version -lz

install-exec-local:
	install -d $(PLUGINDIR)
	$(LIBTOOL) install solitair.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/solitair.cfg $(PLUGINDIR)

uninstall-local:
	-rm $(PLUGINDIR)/solitair.so
	-rm $(PLUGINDIR)/solitair.cfg
	-rmdir $(PLUGINDIR)
