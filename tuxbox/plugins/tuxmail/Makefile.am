INCLUDES = \
	@FREETYPE_CFLAGS@ \
	@CURL_CFLAGS@ \
	-I$(top_srcdir)/include

bin_PROGRAMS = tuxmaild

tuxmaild_SOURCES = daemon/tuxmaild.c

tuxmaild_LDADD = \
	@CURL_LIBS@ \
	-lpthread

noinst_LTLIBRARIES = tuxmail.la

tuxmail_la_SOURCES = tuxmail.c

tuxmail_la_LIBADD = \
	@FREETYPE_LIBS@

tuxmail_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	install -d $(CONFIGDIR)/tuxmail
	$(LIBTOOL) install tuxmail.la $(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxmail.cfg $(PLUGINDIR)
	install -m 0644 $(srcdir)/tuxmail.conf $(CONFIGDIR)/tuxmail
