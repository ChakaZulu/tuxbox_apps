SUBDIRS = src

install-data-local:
	install -d $(FONTDIR)
	install -m 0644 fonts/*.ttf $(FONTDIR)
	install -m 0644 fonts/*.pfa $(FONTDIR)
	install -d $(DATADIR)/enigma/skins
	install -m 0644 skins/*.esml $(DATADIR)/enigma/skins
	install -d $(DATADIR)/enigma/pictures
	install -m 0644 pictures/*.png $(DATADIR)/enigma/pictures
	install -d $(CONFIGDIR)/enigma

