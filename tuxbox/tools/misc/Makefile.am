if BOXTYPE_DREAMBOX
bin_PROGRAMS = camd fbclear lcddump pluginx rcsim audioplay
else
bin_PROGRAMS = aviafbtool camd fbclear lcddump pluginx saa switch rcsim aviaext audioplay
endif

aviafbtool_SOURCES = aviafbtool.c
aviaext_SOURCES = aviaext.c
camd_SOURCES = camd.c
fbclear_SOURCES = fbclear.c
lcddump_SOURCES = lcddump.c
pluginx_SOURCES = pluginx.c
saa_SOURCES = saa.c
switch_SOURCES = switch.c
rcsim_SOURCES = rcsim.c
audioplay_SOURCES = audioplay.c

pluginx_LDADD = -ldl -lpthread

AM_CFLAGS = @PLUGINS_CFLAGS@
