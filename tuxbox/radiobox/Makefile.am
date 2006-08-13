## Process this file with automake to produce Makefile.in
## Created by Anjuta - will be overwritten
## If you don't want it to overwrite it,
## 	Please disable it in the Anjuta project configuration

SUBDIRS = src

radioboxdocdir = ${prefix}/doc/radiobox
radioboxdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS\
	TODO

EXTRA_DIST = $(radioboxdoc_DATA)

# Copy all the spec files. Of cource, only one is actually used.
dist-hook:
	for specfile in *.spec; do \
		if test -f $$specfile; then \
			cp -p $$specfile $(distdir); \
		fi \
	done

install-data-local:
	mkdir -p ${prefix}/share/fonts
	cp ${top_srcdir}/../enigma/data/fonts/bluebold.ttf ${prefix}/share/fonts
	cp ${top_srcdir}/../enigma/data/fonts/bluehigh.ttf ${prefix}/share/fonts
	cp ${top_srcdir}/../enigma/data/fonts/pakenham.ttf ${prefix}/share/fonts
	cp ${top_srcdir}/../enigma/data/fonts/unmrs.pfa    ${prefix}/share/fonts
	cp ${top_srcdir}/../neutrino/data/fonts/*.ttf      ${prefix}/share/fonts
	cp ${top_srcdir}/../neutrino/data/fonts/*.gz       ${prefix}/share/fonts
	cp ${top_srcdir}/../neutrino/data/fonts/*.pcf      ${prefix}/share/fonts
	for d in $$( find ${top_srcdir}/root -type d -printf "%P\n" | grep -v CVS ); \
	do \
		echo "making $(prefix)/$$d"; \
		mkdir -p $(prefix)/$$d; \
	done
	for f in $$( find ${top_srcdir}/root -type f -printf "%P\n" | grep -v CVS ); \
	do \
		cp ${top_srcdir}/root/$$f  $(prefix)/$$( dirname $$f ); \
	done
