INCLUDES = \
	-I$(top_srcdir)/lib \
	@EVENTSERVER_CFLAGS@ \
	-I@DRIVER@

bin_PROGRAMS = bat epg epgMini nit sectionsd sdt

bat_SOURCES = bat.cpp SIsections.cpp SIevents.cpp SIutils.cpp
epg_SOURCES = epg.cpp SIsections.cpp SIevents.cpp SIutils.cpp
epgMini_SOURCES = epgMini.cpp SIsections.cpp SIevents.cpp SIutils.cpp
nit_SOURCES = nit.cpp SIsections.cpp SIevents.cpp SIutils.cpp
sectionsd_SOURCES = sectionsd.cpp SIsections.cpp SIevents.cpp SIutils.cpp
sdt_SOURCES = sdt.cpp SIsections.cpp SIevents.cpp SIutils.cpp

sectionsd_LDADD = \
	@EVENTSERVER_LIBS@ \
	-lpthread

