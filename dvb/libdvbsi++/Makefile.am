SUBDIRS = include src qa

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdvbsi++.pc

DIST_HASHES = $(addsuffix .md5,$(DIST_ARCHIVES))
DIST_SIGNATURES = $(addsuffix .asc,$(DIST_ARCHIVES))

$(DIST_HASHES): $(DIST_ARCHIVES)
	@list='$(DIST_ARCHIVES)'; for i in $$list; do md5sum $$i > $$i.md5; done

$(DIST_SIGNATURES): $(DIST_ARCHIVES)
	@list='$(DIST_ARCHIVES)'; for i in $$list; do gpg --armor --detach-sign $$i; done

release: $(DIST_ARCHIVES) $(DIST_HASHES) $(DIST_SIGNATURES)
	@if [ ! -z "$(LIBDVBSI_RELEASE_PATH)" ]; then scp $^ $(LIBDVBSI_RELEASE_PATH); fi

