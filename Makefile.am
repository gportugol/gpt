# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign subdir-objects

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src doc exemplos lib

docdir = $(datadir)/doc/$(PACKAGE)

doc_DATA = AGENTS.md CHANGELOG.md CONTRIBUTING.md HACKING.md INSTALL.md \
	   NEWS.md README.md README.en.md SECURITY.md THANKS

EXTRA_DIST = INSTALL.default $(doc_DATA)

uninstall-devel:
	rm -rf $(includedir)/${PACKAGE}
	rm -f $(libdir)/libgportugol*
