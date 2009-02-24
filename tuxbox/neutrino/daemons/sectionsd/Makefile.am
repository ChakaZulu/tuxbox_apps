INCLUDES = \
	-I$(top_srcdir)/lib \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@ \
	@XML_CFLAGS@

if TRUE
AM_CPPFLAGS = -D DO_NOT_INCLUDE_STUFF_NOT_NEEDED_FOR_SECTIONSD -W
bin_PROGRAMS = sectionsd 
else
bin_PROGRAMS = bat epg epgMini nit sectionsd sdt
bat_SOURCES = bat.cpp dmxapi.cpp SIsections.cpp SIevents.cpp SIutils.cpp SIlanguage.cpp dvbstring.cpp
epg_SOURCES = epg.cpp dmxapi.cpp SIsections.cpp SIevents.cpp SIutils.cpp SIlanguage.cpp dvbstring.cpp
epgMini_SOURCES = epgMini.cpp dmxapi.cpp SIsections.cpp SIevents.cpp SIutils.cpp SIlanguage.cpp dvbstring.cpp
nit_SOURCES = nit.cpp dmxapi.cpp SIsections.cpp SIevents.cpp SIutils.cpp SIlanguage.cpp dvbstring.cpp
sdt_SOURCES = sdt.cpp dmxapi.cpp SIsections.cpp SIevents.cpp SIutils.cpp SIlanguage.cpp dvbstring.cpp

bat_LDADD = \
	@XML_LIBS@

epg_LDADD = \
	@XML_LIBS@

epgMini_LDADD = \
	@XML_LIBS@

nit_LDADD = \
	@XML_LIBS@

sdt_LDADD = \
	@XML_LIBS@

endif

sectionsd_SOURCES = sectionsd.cpp dmxapi.cpp debug.cpp dmx.cpp SIsections.cpp SIevents.cpp SIutils.cpp dvbstring.cpp SIlanguage.cpp

sectionsd_LDADD = \
	@CONNECTION_LIBS@ \
	@CONFIGFILE_LIBS@ \
	@EVENTSERVER_LIBS@ \
	@XML_LIBS@ \
	-lpthread
