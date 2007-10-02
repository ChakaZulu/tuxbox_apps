INCLUDES = \
	-I$(top_srcdir)/src \
        -I$(top_srcdir)/lib \
        @FREETYPE_CFLAGS@ \
        @VORBISIDEC_CFLAGS@ \
        @ZAPIT_CFLAGS@ 

noinst_LIBRARIES = libneutrino_driver_audiodec.a

if ENABLE_FLAC
FLACdec= flacdec.cpp
endif

libneutrino_driver_audiodec_a_SOURCES = \
	basedec.cpp \
	cdrdec.cpp \
	mp3dec.cpp \
	oggdec.cpp \
	wavdec.cpp \
	$(FLACdec) \
	tag.c \
	crc.c
