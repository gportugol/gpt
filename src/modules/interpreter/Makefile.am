AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/modules \
	-I$(top_srcdir)/src/modules/interpreter -I$(top_srcdir)/src/modules/parser \
	-I$(top_builddir)/src/modules/parser \
	$(ANTLR4_INC) $(PCRE_INC) $(all_includes)

noinst_LTLIBRARIES = libinterpreter.la

# No ANTLR4-generated files - using manual visitor implementation
libinterpreter_la_SOURCES = InterpreterEval.cpp Interpreter.cpp

if BUILD_DEBUGGER
libinterpreter_la_SOURCES += InterpreterDBG.cpp
endif

noinst_HEADERS = InterpreterDBG.hpp InterpreterEval.hpp Interpreter.hpp

EXTRA_DIST = interpreter.g
