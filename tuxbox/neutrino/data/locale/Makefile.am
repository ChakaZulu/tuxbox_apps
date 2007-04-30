installdir = $(DATADIR)/neutrino/locale

install_DATA = \
	bayrisch.locale \
	bosanski.locale \
	ch-baslerdeutsch.locale \
	ch-berndeutsch.locale \
	deutsch.locale \
	ellinika.locale \
	english.locale \
	francais.locale \
	italiano.locale \
	nederlands.locale \
	polski.locale \
	portugues.locale \
	russkij.locale \
	suomi.locale \
	svenska.locale


if MAINTAINER_MODE

all: check

locals: locals.h locals_intern.h

ordercheck: deutsch.locale english.locale
	@for i in *.locale; do \
		echo "checking" $$i; \
		cut -d' ' -f1 $$i | LC_ALL=C sort | uniq > /tmp/log1; \
		cut -d' ' -f1 $$i | uniq > /tmp/log2; \
		diff -u /tmp/log1 /tmp/log2 || \
			(echo "ERROR: "$$i" not ordered or contains empty lines" && false); \
	done
	@rm /tmp/log1
	@rm /tmp/log2

locals.h: ordercheck deutsch.locale english.locale
	@cut -d' ' -f1 deutsch.locale | LC_ALL=C sort | uniq | tr [:lower:] [:upper:] | tr \. \_  | tr \- \_ | tr -d \? | ./create.locals.h
	@cut -d' ' -f1 english.locale | LC_ALL=C sort | uniq | tr [:lower:] [:upper:] | tr \. \_  | tr \- \_ | tr -d \? | ./create.locals.h

locals_intern.h: ordercheck deutsch.locale english.locale
	@cut -d' ' -f1 deutsch.locale | LC_ALL=C sort | uniq | ./create.locals_intern.h
	@cut -d' ' -f1 english.locale | LC_ALL=C sort | uniq | ./create.locals_intern.h

check: locals.h locals_intern.h
	@echo "checking locals.h"
	@cpp -P locals.h > /tmp/log1
	@cpp -P ../../src/system/locals.h /tmp/log2
	@diff -u /tmp/log1 /tmp/log2 || \
		(echo "ERROR: locals.h not ordered or contains empty lines" && false)
	@echo "checking locals_intern.h"
	@cpp -P locals_intern.h > /tmp/log1
	@cpp -P ../../src/system/locals_intern.h /tmp/log2
	@diff -u /tmp/log1 /tmp/log2 || \
		(echo "ERROR: locals_intern.h not ordered or contains empty lines" && false)
	@rm /tmp/log1
	@rm /tmp/log2

install-locals: locals.h locals_intern.h
	cp locals.h locals_intern.h ../../src/system
	@echo "Consider committing .../neutrino/src/system/[locals.h,locals_intern.h]"

locals-clean:
	rm -f locals.h locals_intern.h

endif
