INCLUDES = \
	-I$(top_srcdir)/lib \
	@CONNECTION_CFLAGS@ \
	@EVENTSERVER_CFLAGS@

bin_PROGRAMS = bat epg epgMini nit sectionsd sdt

bat_SOURCES = bat.cpp SIsections.cpp SIevents.cpp SIutils.cpp
epg_SOURCES = epg.cpp SIsections.cpp SIevents.cpp SIutils.cpp
epgMini_SOURCES = epgMini.cpp SIsections.cpp SIevents.cpp SIutils.cpp
nit_SOURCES = nit.cpp SIsections.cpp SIevents.cpp SIutils.cpp
sectionsd_SOURCES = dmx.cpp sectionsd.cpp SIsections.cpp SIevents.cpp SIutils.cpp
sdt_SOURCES = sdt.cpp SIsections.cpp SIevents.cpp SIutils.cpp

sectionsd_LDADD = \
	@CONNECTION_LIBS@ \
	@EVENTSERVER_LIBS@ \
	-lpthread

