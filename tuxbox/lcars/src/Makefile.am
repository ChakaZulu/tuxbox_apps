INCLUDES = \
	@CURL_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@PLUGINS_CFLAGS@ \
	-Werror
#	-DOLD_VBI
#	-DDREAMBOX

bin_PROGRAMS = lcars

lcars_SOURCES = \
	main.cpp timer.cpp pig.cpp plugins.cpp \
	hardware.cpp scan.cpp network.cpp teletext.cpp \
	osd.cpp sdt.cpp zap.cpp nit.cpp pat.cpp pmt.cpp \
	eit.cpp settings.cpp channels.cpp rc.cpp help.cpp fbClass.cpp \
	tot.cpp checker.cpp tuner.cpp update.cpp control.cpp variables.cpp \
	ir.cpp cam.cpp

lcars_LDADD = \
	@CURL_LIBS@ \
	@FREETYPE_LIBS@ \
	-lpthread -ldl -lz

install-data-local:
	install -d $(CONFIGDIR)/lcars

