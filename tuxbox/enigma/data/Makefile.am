SUBDIRS = \
	fonts \
	htdocs \
	pictures \
	resources \
	skins \
	templates \
	lirc

if TARGET_CDK
if !TARGETRULESET_UCLIBC
SUBDIRS += \
	sysconfig
endif 
endif 

install-data-local:
	install -d $(DESTDIR)$(CONFIGDIR)/enigma
	install -m 0644 encoding.conf $(DESTDIR)$(CONFIGDIR)/enigma
