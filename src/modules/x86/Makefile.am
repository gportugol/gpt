AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/modules \
	-I$(top_srcdir)/src/modules/x86 \
	-I$(top_srcdir)/src/modules/parser \
	-I$(top_builddir)/src/modules/parser \
	$(ANTLR4_INC) $(all_includes)
METASOURCES = AUTO

noinst_LTLIBRARIES = libx86.la

# X86 code generator - implemented manually
libx86_la_SOURCES = X86.cpp X86Generator.cpp

noinst_HEADERS = X86.hpp X86Generator.hpp asm_elf.h asm_lib.h asm_prologue.h asm_win32.h

EXTRA_DIST = x86.g X86Walker.g
