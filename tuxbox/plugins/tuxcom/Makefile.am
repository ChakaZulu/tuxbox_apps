
INCLUDES =  \
	@FREETYPE_CFLAGS@ \
	-I$(top_srcdir)/include

noinst_LTLIBRARIES = tuxcom.la

#tuxcom_la_CFLAGS = -DDREAMBOX 

tuxcom_la_SOURCES = tuxcom.c

tuxcom_la_LIBADD = \
	 @FREETYPE_LIBS@
	 

tuxcom_la_LDFLAGS = -rpath $(PLUGINDIR) -module -avoid-version 

install-exec-local:
	install -d $(PLUGINDIR)
	install -d $(CONFIGDIR)
	$(LIBTOOL) install tuxcom.la $(PLUGINDIR) 
	install -m 0644 $(srcdir)/tuxcom.cfg $(PLUGINDIR)
