# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign subdir-objects

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src doc exemplos lib

docdir = $(datadir)/doc/$(PACKAGE)

doc_DATA = README.md README.en.md CHANGELOG.md HACKING.md NEWS.md THANKS

EXTRA_DIST = INSTALL.default $(doc_DATA)

uninstall-devel:
	rm -rf $(includedir)/${PACKAGE}
	rm -f $(libdir)/libgportugol*
