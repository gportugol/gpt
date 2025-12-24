AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/modules \
	-I$(top_srcdir)/src/modules/c_translator \
	-I$(top_builddir)/src/modules/parser \
	$(ANTLR3_INC) $(all_includes)

noinst_LTLIBRARIES = libctranslator.la

BUILT_SOURCES = Portugol2CWalker.c

nodist_libctranslator_la_SOURCES = $(BUILT_SOURCES) Portugol2CWalker.h

pt2c_g = Portugol2CWalker.g
EXTRA_DIST = $(pt2c_g) pt2c.g

CLEANFILES = Portugol2CWalker.c Portugol2CWalker.h Portugol.tokens

Portugol2CWalker.c: $(srcdir)/$(pt2c_g) Portugol.tokens
	$(ANTLR3_CMD) -o . $(srcdir)/$(pt2c_g)

Portugol.tokens: $(top_builddir)/src/modules/parser/Portugol.tokens
	cp $(top_builddir)/src/modules/parser/Portugol.tokens .
