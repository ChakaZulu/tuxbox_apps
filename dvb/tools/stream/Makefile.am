sbin_PROGRAMS = streampes streamts streamsec udpstreamts streamfile fserver

INCLUDES = @MPEGTOOLS_CFLAGS@

streampes_SOURCES = streampes.c
streamts_SOURCES = streamts.c
streamsec_SOURCES = streamsec.c
udpstreamts_SOURCES = udpstreamts.c
streamfile_SOURCES = streamfile.c ringbuffer.c
fserver_SOURCES = fserver.c

streamts_LDADD = @MPEGTOOLS_LIBS@
streamfile_LDADD = @MPEGTOOLS_LIBS@ -lpthread
