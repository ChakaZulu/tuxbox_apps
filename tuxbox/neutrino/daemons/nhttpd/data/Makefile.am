SUBDIRS = images

EXTRA_DIST = \
	controlpanel.html dbox.html epg.html index.html \
	settings.html shutdown.html \
	global.css \ controlpanel_old.html \
	channels.js

install-data-local:
	install -d $(DATADIR)/neutrino/httpd
	install -m 0644 *.html $(DATADIR)/neutrino/httpd
	install -m 0644 *.css $(DATADIR)/neutrino/httpd
	install -m 0644 *.js $(DATADIR)/neutrino/httpd

uninstall-local:
	-rm -rf $(DATADIR)/neutrino/httpd
