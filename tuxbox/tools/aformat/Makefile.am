if BOXTYPE_DBOX2
INCLUDES = \
        -I$(top_srcdir)/include \
        @ZAPIT_CFLAGS@ \
        @CONFIGFILE_CFLAGS@ \
        @CONNECTION_CFLAGS@

bin_PROGRAMS = aformat

aformat_SOURCES = \
	aformat.cpp

aformat_LDADD = \
	@ZAPIT_LIBS@ \
	@CONFIGFILE_LIBS@ \
	@CONNECTION_LIBS@

endif
