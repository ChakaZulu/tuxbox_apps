INCLUDES = \
	-I@DRIVER@/include \
	-I@DRIVER@ \
	-I$(top_srcdir)/libs/libsectionsdclient \
	@EVENTSERVER_CFLAGS@

bin_PROGRAMS = bat epg epgMini nit sectionsd sdt

bat_SOURCES = bat.cpp SIsections.cpp SIevents.cpp SIutils.cpp
epg_SOURCES = epg.cpp SIsections.cpp SIevents.cpp SIutils.cpp
epgMini_SOURCES = epgMini.cpp SIsections.cpp SIevents.cpp SIutils.cpp
nit_SOURCES = nit.cpp SIsections.cpp SIevents.cpp SIutils.cpp
sectionsd_SOURCES = sectionsd.cpp SIsections.cpp SIevents.cpp SIutils.cpp mktime.c
sdt_SOURCES = sdt.cpp SIsections.cpp SIevents.cpp SIutils.cpp

sectionsd_LDADD = \
	@EVENTSERVER_LIBS@ \
	-lpthread
	#$(top_builddir)/libs/libtimerdclient/libtimerdclient.la

