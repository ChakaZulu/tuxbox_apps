EXTRA_DIST = rcdboxnew.xml rcdboxold.xml rcdboxbuttons.xml help.xml \
	rcdm7000.xml rcdm5xxx.xml rctrdvb272.xml \
	rcdreambox_inputdev.xml rcgeneric_inputdev.xml rcdbox_inputdev.xml \
	keymappings.xml rcdreambox_keyboard.xml

install-data-local:
	install -d $(DESTDIR)$(DATADIR)/enigma/resources
	install -m 0644 $(EXTRA_DIST) $(DESTDIR)$(DATADIR)/enigma/resources
