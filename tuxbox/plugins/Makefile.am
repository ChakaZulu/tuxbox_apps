AUTOMAKE_OPTIONS = gnu

if BOXTYPE_DREAMBOX
SUBDIRS = \
	include tuxmail tuxtxt tuxcom fx2 enigma
else
SUBDIRS = \
	include tuxmail tuxtxt tuxcom fx2 vncviewer enigma
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = \
	tuxbox-plugins.pc

