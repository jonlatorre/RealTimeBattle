SUBDIRS = debian freebsd rpm

PACKAGES = debian freebsd rpm

packages: $(PACKAGES)

.PHONY: $(PACKAGES)

$(PACKAGES):
	cd $@ && $(MAKE) $@

