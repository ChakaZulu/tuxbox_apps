INCLUDES = \
	-I$(DRIVER)/include \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/system

noinst_LTLIBRARIES = libdriver.la

libdriver_la_SOURCES = \
	eavswitch.cpp rc.cpp rc.moc.cpp rcdbox.cpp rcdbox.moc.cpp rcdreambox.cpp rcdreambox.moc.cpp streamwd.cpp streamwd.moc.cpp

libdriver_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

%.moc.cpp: %.h
	$(MOC) $< -o $@

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libdriver.la $(LIBDIR)/enigma

clean-local:
	-rm *.moc.cpp
