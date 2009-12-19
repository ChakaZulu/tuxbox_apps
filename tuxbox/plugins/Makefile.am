AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	include tuxmail tuxtxt tuxcom tuxcal tuxclock fx2 vncviewer enigma shellexec tuxwetter

if BOXTYPE_DREAMBOX
SUBDIRS += pip mosaic
else
SUBDIRS += pluginx
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	tuxbox-plugins.pc

