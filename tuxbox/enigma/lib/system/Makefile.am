INCLUDES = \
	-I$(top_srcdir)/edvb/miniqt \
	-I$(top_srcdir)/enigma/src/core/dvb \
	-I$(top_srcdir)/enigma/src/core/xml \
	-I$(top_srcdir)/enigma/src/apps/enigma \
	-I$(top_srcdir)/enigma/src/core/driver \
	-I$(top_srcdir)/enigma/src/core/gdi \
	-I$(top_srcdir)/enigma/src/core/gui

noinst_LTLIBRARIES = libsystem.la

libsystem_la_SOURCES = \
	init.cpp elock.cpp http_dyn.cpp http_file.cpp httpd.cpp httpd.moc.cpp \
	mktime.c nconfig.cpp nxml.cpp qserversocket.cpp xmlrpc.cpp 

libsystem_la_LDFLAGS = -rpath $(_LIBDIR)/enigma

%.moc.cpp: %.h
	$(MOC) $< -o $@

install-exec-local:
	install -d $(LIBDIR)/enigma
	$(LIBTOOL) --mode=install $(INSTALL) libsystem.la $(LIBDIR)/enigma

clean-local:
	-rm *.moc.cpp
