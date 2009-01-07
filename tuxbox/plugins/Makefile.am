AUTOMAKE_OPTIONS = gnu

if BOXTYPE_DREAMBOX
SUBDIRS = \
	include tuxmail tuxtxt tuxcom tuxcal fx2 vncviewer enigma pip mosaic
else
SUBDIRS = \
	include tuxmail tuxtxt tuxcom tuxcal fx2 vncviewer enigma pluginx
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	tuxbox-plugins.pc

