SUBDIRS = intl po Arenas Robots Documentation Packages src 

rtbstddocsdir = $(DOCS_DIR)
rtbstddocs_DATA = ABOUT-NLS BUGS FAQ TODO README INSTALL ChangeLog CVS-Help

rtbextradir = $(RTB_DIR)
rtbextra_DATA = RealTimeBattle.xpm tournament.txt

EXTRA_DIST = $(rtbextra_DATA) $(rtbstddocs_DATA)

AUTOMAKE_OPTIONS = foreign


uninstall-local:
	rmdir $(RTB_DIR)/Arenas || true; \
	rmdir -p $(RTB_DIR)/Robots/* || true



#These targets are indended mainly for the maintainer

robots:
	cd Robots && $(MAKE)

html_docs:
	cd Documentation && $(MAKE) html_docs

docs:
	cd Documentation && $(MAKE) docs

bz2_dist: dist
	gunzip -c $(PACKAGE)-$(VERSION).tar.gz | bzip2 > $(PACKAGE)-$(VERSION).tar.bz2

html_clean:
	cd HTML && $(MAKE) -fMakefile.rtb clean

website: html_docs
	cd HTML && $(MAKE) -fMakefile.rtb

tar-website: 
	cd HTML && $(MAKE) -fMakefile.rtb tar-website

website_download: docs
	cd HTML && $(MAKE) -fMakefile.rtb website_download VERSION=$(VERSION)

website_all: html_clean website website_download

realclean: clean
	cd Documentation && $(MAKE) realclean

debian freebsd rpm:
	cd Packages/$@ && $(MAKE) $@

packages:
	cd Packages && $(MAKE) packages

