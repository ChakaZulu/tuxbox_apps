if HAVE_LIBSIGC
if HAVE_LIBFREETYPE
if HAVE_LIBPNG

SUBDIRS = doc fonts htdocs pictures resources skins src templates

endif
endif
else
all-local:
	echo configure was unable to find libsigc++ on your disk.
	echo this probably means that you do not have pkg-config
	echo installed. grab it from your linux ftp or from here:
	echo
	echo ftp://ftp.berlios.de/pub/tuxbox/misc/pkgconfig-0.12.0.tar.gz
	echo
	echo note that this package does not need to be cross-compiled.
	/bin/false
endif

install-data-local:
	install -d $(CONFIGDIR)/enigma
