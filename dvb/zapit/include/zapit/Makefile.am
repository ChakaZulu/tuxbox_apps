SUBDIRS = client

pkgincludedir = ${includedir}/tuxbox/zapit
pkginclude_HEADERS = \
	audio.h bat.h bouquets.h cam.h channel.h \
	ci.h debug.h descriptors.h dmx.h dvbstring.h frontend.h \
	getservices.h nit.h pat.h pmt.h scan.h sdt.h settings.h \
	transponder.h types.h video.h zapit.h controldtypes.h

if BOXTYPE_TRIPLE
pkginclude_HEADERS += \
	td-demux-compat.h td-video-compat.h td-audio-compat.h \
	td-frontend-compat.h
endif
