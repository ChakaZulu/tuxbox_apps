EXTRA_DIST = tuxtxt.fon

install-data-local:
	install -d $(FONTDIR)
	install -m 0644 $(foreach file,$(EXTRA_DIST),$(srcdir)/$(file)) $(FONTDIR)
