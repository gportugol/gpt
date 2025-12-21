# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign subdir-objects

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src doc exemplos lib

EXTRA_DIST = INSTALL.default CHANGELOG.md HACKING.md THANKS README.en.md README.md

uninstall-devel:
	rm -rf $(includedir)/${PACKAGE}
	rm -f $(libdir)/libgportugol*
