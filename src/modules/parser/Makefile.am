AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/parser \
	-I$(top_srcdir)/src/modules -I$(top_srcdir)/src/modules/parser \
	$(ANTLR4_INC) $(all_includes)

noinst_LTLIBRARIES = libparser.la

antlr4_generated = PortugolLexer.cpp PortugolLexer.h \
                   PortugolParser.cpp PortugolParser.h \
                   PortugolBaseVisitor.cpp PortugolBaseVisitor.h \
                   PortugolVisitor.cpp PortugolVisitor.h

BUILT_SOURCES = antlr4.stamp $(antlr4_generated)

nodist_libparser_la_SOURCES = $(antlr4_generated)

headers = BasePortugolParser.hpp SemanticEval.hpp SemanticAnalyzer.hpp \
          PortugolErrorStrategy.hpp

libparser_la_SOURCES = BasePortugolParser.cpp SemanticEval.cpp SemanticAnalyzer.cpp \
                       PortugolErrorStrategy.cpp

if INSTALL_DEVEL
nodist_pkginclude_HEADERS = PortugolLexer.h PortugolParser.h Portugol.tokens \
                            PortugolBaseVisitor.h PortugolVisitor.h

pkginclude_HEADERS = $(headers)
else
noinst_HEADERS = $(headers)
endif

portugol_g = Portugol.g4

EXTRA_DIST = $(portugol_g)

CLEANFILES = antlr4.stamp $(antlr4_generated) \
             PortugolLexer.interp PortugolLexer.tokens \
             PortugolParser.interp Portugol.tokens

# One antlr4 run produces all generated files. The stamp file and the
# recursive make below are the "multiple outputs" idiom from the automake
# manual: a plain multi-target rule would run the tool once per target
# under make -j, with the parallel runs clobbering each other's output.
antlr4.stamp: $(srcdir)/$(portugol_g)
	@rm -f antlr4.tmp
	@touch antlr4.tmp
	$(ANTLR4_CMD) -Dlanguage=Cpp -visitor -no-listener -o . $(srcdir)/$(portugol_g)
	@mv -f antlr4.tmp $@

$(antlr4_generated): antlr4.stamp
	@if test -f $@; then :; else \
	  rm -f antlr4.stamp; \
	  $(MAKE) $(AM_MAKEFLAGS) antlr4.stamp; \
	fi
