AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/modules -I$(top_srcdir)/src/modules/parser \
	$(ANTLR3_INC) $(all_includes)

noinst_LTLIBRARIES = libparser.la

BUILT_SOURCES = PortugolLexer.c PortugolParser.c SemanticWalker.c

nodist_libparser_la_SOURCES = $(BUILT_SOURCES) \
	PortugolLexer.h PortugolParser.h SemanticWalker.h

headers = BasePortugolParser.hpp SemanticEval.hpp Antlr3Adapter.hpp \
          SemanticWalkerTokenTypes.hpp SemanticAnalyzer.hpp

libparser_la_SOURCES = BasePortugolParser.cpp SemanticEval.cpp SemanticAnalyzer.cpp

if INSTALL_DEVEL
nodist_pkginclude_HEADERS = PortugolLexer.h PortugolParser.h Portugol.tokens \
                            SemanticWalker.h

pkginclude_HEADERS = $(headers)
else
noinst_HEADERS = $(headers)
endif

portugol_g   = Portugol.g
semantic_g   = SemanticWalker.g

EXTRA_DIST = $(portugol_g) $(semantic_g) accent.sh

CLEANFILES = PortugolLexer.c PortugolLexer.h PortugolParser.c PortugolParser.h \
             Portugol.tokens SemanticWalker.c SemanticWalker.h

# Generate lexer and parser from combined grammar
PortugolLexer.c PortugolParser.c: $(srcdir)/$(portugol_g)
	$(ANTLR3_CMD) -o . $(srcdir)/$(portugol_g)
	cp Portugol.tokens ../c_translator
	cp Portugol.tokens ../interpreter
	cp Portugol.tokens ../x86

# Generate semantic walker
SemanticWalker.c: $(srcdir)/$(semantic_g) Portugol.tokens
	$(ANTLR3_CMD) -o . $(srcdir)/$(semantic_g)
