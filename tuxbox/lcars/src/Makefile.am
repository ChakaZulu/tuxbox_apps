INCLUDES = \
	@CURL_CFLAGS@ \
	@FREETYPE_CFLAGS@ \
	@PLUGINS_CFLAGS@ \
	@TUXBOX_CFLAGS@

bin_PROGRAMS = lcars

lcars_SOURCES = \
	main.cpp timer.cpp pig.cpp plugins.cpp \
	hardware.cpp scan.cpp network.cpp teletext.cpp \
	osd.cpp sdt.cpp zap.cpp nit.cpp pat.cpp pmt.cpp \
	eit.cpp settings.cpp channels.cpp rc.cpp help.cpp fbClass.cpp \
	tot.cpp checker.cpp tuner.cpp update.cpp control.cpp variables.cpp \
	ir.cpp cam.cpp lcd.cpp

lcars_CFLAGS = -Werror
#	-DOLD_VBI
#	-DDREAMBOX

lcars_LDADD = \
	@CURL_LIBS@ \
	@FREETYPE_LIBS@ \
	@TUXBOX_LIBS@ \
	-lpthread -ldl

install-data-local:
	install -d $(CONFIGDIR)/lcars

