AM_CPPFLAGS = -I$(top_srcdir)/. -I$(top_srcdir)/src/modules \
	-I$(top_srcdir)/src/modules/c_translator \
	-I$(top_srcdir)/src/modules/parser \
	-I$(top_builddir)/src/modules/parser \
	$(ANTLR4_INC) $(all_includes)

noinst_LTLIBRARIES = libctranslator.la

# C translator visitor - implemented manually
libctranslator_la_SOURCES = Portugol2CTranslator.cpp

noinst_HEADERS = Portugol2CTranslator.hpp

EXTRA_DIST = pt2c.g Portugol2CWalker.g
