if BOXTYPE_DREAMBOX
sbin_PROGRAMS = streampes streamts streamsec udpstreamts
else
sbin_PROGRAMS = streampes streamts streamsec udpstreamts streamfile fserver
INCLUDES = @MPEGTOOLS_CFLAGS@
streamts_LDADD = @MPEGTOOLS_LIBS@
streamfile_LDADD = @MPEGTOOLS_LIBS@ -lpthread
streamfile_SOURCES = streamfile.c ringbuffer.c
fserver_SOURCES = fserver.c
endif
streampes_SOURCES = streampes.c
streamts_SOURCES = streamts.c
streamsec_SOURCES = streamsec.c
udpstreamts_SOURCES = udpstreamts.c
