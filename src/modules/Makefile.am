AM_CPPFLAGS = -I$(top_srcdir) -I$(top_srcdir)/src/parser -Iparser $(ANTLR4_INC) $(all_includes)

SUBDIRS = parser c_translator interpreter x86 arm64

headers = GPTDisplay.hpp NativeBackend.hpp Symbol.hpp SymbolTable.hpp
sources = GPTDisplay.cpp NativeBackend.cpp Symbol.cpp SymbolTable.cpp

if INSTALL_DEVEL
lib_LTLIBRARIES = libgportugol.la
pkginclude_HEADERS = $(headers)
else
noinst_LTLIBRARIES = libgportugol.la
noinst_HEADERS = $(headers)
endif


libgportugol_la_SOURCES = $(sources)

libgportugol_la_LIBADD = $(top_builddir)/src/modules/parser/libparser.la \
                         $(top_builddir)/src/modules/interpreter/libinterpreter.la \
                         $(top_builddir)/src/modules/c_translator/libctranslator.la \
												 $(top_builddir)/src/modules/x86/libx86.la \
                         $(top_builddir)/src/modules/arm64/libarm64.la

if INSTALL_DEVEL
# The installed library is used outside gpt (GPTEditor), so it names the
# runtimes it calls and carries a SONAME. Bump -version-info whenever the
# installed headers change incompatibly: .so.0 was the ANTLR 2 API.
libgportugol_la_LDFLAGS = -version-info 1:0:0
libgportugol_la_LIBADD += $(ANTLR4_LIB)
if BUILD_DEBUGGER
libgportugol_la_LIBADD += $(PCRE_LIB)
endif
endif
