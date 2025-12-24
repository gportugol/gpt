AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/modules -I$(top_srcdir)/src/modules/parser \
	$(ANTLR4_INC) $(all_includes)

noinst_LTLIBRARIES = libparser.la

# ANTLR4 generates C++ files
BUILT_SOURCES = PortugolLexer.cpp PortugolParser.cpp \
                PortugolBaseVisitor.cpp PortugolVisitor.cpp

nodist_libparser_la_SOURCES = $(BUILT_SOURCES) \
	PortugolLexer.h PortugolParser.h \
	PortugolBaseVisitor.h PortugolVisitor.h

headers = BasePortugolParser.hpp SemanticEval.hpp SemanticAnalyzer.hpp

libparser_la_SOURCES = BasePortugolParser.cpp SemanticEval.cpp SemanticAnalyzer.cpp

if INSTALL_DEVEL
nodist_pkginclude_HEADERS = PortugolLexer.h PortugolParser.h Portugol.tokens \
                            PortugolBaseVisitor.h PortugolVisitor.h

pkginclude_HEADERS = $(headers)
else
noinst_HEADERS = $(headers)
endif

portugol_g = Portugol.g4

EXTRA_DIST = $(portugol_g)

CLEANFILES = PortugolLexer.cpp PortugolLexer.h PortugolLexer.interp PortugolLexer.tokens \
             PortugolParser.cpp PortugolParser.h PortugolParser.interp Portugol.tokens \
             PortugolBaseVisitor.cpp PortugolBaseVisitor.h \
             PortugolVisitor.cpp PortugolVisitor.h

# Generate lexer, parser and visitor from grammar
PortugolLexer.cpp PortugolParser.cpp PortugolBaseVisitor.cpp PortugolVisitor.cpp: $(srcdir)/$(portugol_g)
	$(ANTLR4_CMD) -Dlanguage=Cpp -visitor -no-listener -o . $(srcdir)/$(portugol_g)
