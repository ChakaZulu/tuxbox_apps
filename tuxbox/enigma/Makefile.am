if HAVE_LIBFREETYPE
if HAVE_LIBPNG

SUBDIRS = doc src

endif
endif

install-data-local:
	install -d $(FONTDIR)
	install -m 0644 fonts/*.ttf $(FONTDIR)
	install -m 0644 fonts/*.pfa $(FONTDIR)
	install -d $(DATADIR)/enigma/skins
	install -m 0644 skins/*.esml $(DATADIR)/enigma/skins
	install -d $(DATADIR)/enigma/pictures
	install -m 0644 pictures/*.png $(DATADIR)/enigma/pictures
	install -d $(DATADIR)/enigma/pictures/decoration
	install -m 0644 pictures/decoration/*.png $(DATADIR)/enigma/pictures/decoration
	install -d $(DATADIR)/enigma/templates
	install -m 0644 templates/*.tmp $(DATADIR)/enigma/templates
	install -d $(DATADIR)/enigma/htdocs
	install -m 0644 htdocs/*.gif $(DATADIR)/enigma/htdocs
	install -d $(CONFIGDIR)/enigma
	install -d $(DATADIR)/enigma/resources
	install -m 0644 resources/*.xml $(DATADIR)/enigma/resources
