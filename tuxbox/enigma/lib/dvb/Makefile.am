SUBDIRS = lowlevel

INCLUDES = \
	-I$(top_srcdir)/include

lib_LTLIBRARIES = libenigma_dvb.la

libenigma_dvb_la_SOURCES = \
	decoder.cpp dvb.cpp edvb.cpp epgcache.cpp \
	esection.cpp frontend.cpp si.cpp record.cpp \
	dvbwidgets.cpp dvbservice.cpp settings.cpp \
	dvbscan.cpp service.cpp servicedvb.cpp servicestructure.cpp \
	servicefile.cpp servicemp3.cpp serviceplaylist.cpp dvbci.cpp

libenigma_dvb_la_LDFLAGS = @ID3TAG_LIBS@ @MAD_LIBS@

AM_CPPFLAGS = -DPROFILE

