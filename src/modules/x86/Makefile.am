AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/modules \
	-I$(top_srcdir)/src/modules/x86 \
	-I$(top_builddir)/src/modules/parser \
	$(ANTLR3_INC) $(all_includes)
METASOURCES = AUTO

noinst_LTLIBRARIES = libx86.la

BUILT_SOURCES = X86Walker.c

libx86_la_SOURCES = X86.cpp
nodist_libx86_la_SOURCES = $(BUILT_SOURCES) X86Walker.h
noinst_HEADERS = X86.hpp asm_elf.h asm_lib.h asm_prologue.h asm_win32.h

x86_g = X86Walker.g
EXTRA_DIST = $(x86_g) x86.g

CLEANFILES = X86Walker.c X86Walker.h Portugol.tokens

X86Walker.c: $(srcdir)/$(x86_g) Portugol.tokens
	$(ANTLR3_CMD) -o . $(srcdir)/$(x86_g)

Portugol.tokens: $(top_builddir)/src/modules/parser/Portugol.tokens
	cp $(top_builddir)/src/modules/parser/Portugol.tokens .
