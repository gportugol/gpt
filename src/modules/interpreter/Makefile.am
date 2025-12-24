AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/modules \
	-I$(top_srcdir)/src/modules/interpreter -I$(top_srcdir)/src/modules/parser \
	-I$(top_builddir)/src/modules/parser \
	$(ANTLR3_INC) $(PCRE_INC) $(all_includes)

noinst_LTLIBRARIES = libinterpreter.la

BUILT_SOURCES = InterpreterWalker.c

nodist_libinterpreter_la_SOURCES = $(BUILT_SOURCES) InterpreterWalker.h

libinterpreter_la_SOURCES = InterpreterEval.cpp Interpreter.cpp

if BUILD_DEBUGGER
libinterpreter_la_SOURCES += InterpreterDBG.cpp
endif

noinst_HEADERS = InterpreterDBG.hpp InterpreterEval.hpp Interpreter.hpp

interpreter_g = InterpreterWalker.g
EXTRA_DIST = $(interpreter_g) interpreter.g

CLEANFILES = InterpreterWalker.c InterpreterWalker.h Portugol.tokens

InterpreterWalker.c: $(srcdir)/$(interpreter_g) Portugol.tokens
	$(ANTLR3_CMD) -o . $(srcdir)/$(interpreter_g)

Portugol.tokens: $(top_builddir)/src/modules/parser/Portugol.tokens
	cp $(top_builddir)/src/modules/parser/Portugol.tokens .
