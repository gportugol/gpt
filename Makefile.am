# not a GNU package. You can remove this line, if
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign subdir-objects

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src doc exemplos lib

docdir = $(datadir)/doc/$(PACKAGE)

doc_DATA = README.md README.en.md CHANGELOG.md HACKING.md NEWS.md THANKS

# Issue #10 -- o que entra no tarball do `make dist`.
#
# O automake so distribui o que esta em SUBDIRS, em EXTRA_DIST e na lista
# de arquivos padrao que ele conhece. Sem as listas abaixo o tarball saia
# com 88 dos 102 arquivos versionados: sem a suite de testes, sem o
# instalador do Windows e sem a fonte do manual.
#
# O criterio para estar aqui e o da propria GNU: a distribuicao carrega o
# que e preciso para CONSTRUIR, TESTAR, INSTALAR e DOCUMENTAR o pacote.
# O que ficou de fora de proposito -- configuracao de CI, arquivo de
# projeto de IDE -- esta listado em stuff/check-dist.sh, com o motivo, e o
# proprio script cobra as duas direcoes.

# A suite de testes. Sem ela, quem baixa o tarball nao consegue verificar
# o que compilou -- e verificar e justamente o que um tarball de release
# deveria permitir a quem nao vai clonar o repositorio.
test_files = \
	test/run_test.sh \
	test/tester.gpt \
	test/error_tester.php \
	test/build_debugger \
	test/cli_debugger.c \
	test/feed_test \
	test/asm/build \
	test/asm/bin.asm \
	test/asm/test.asm \
	test/asm/win32/header.asm \
	test/asm/win32/nagoa+.inc \
	test/asm/win32/teste.asm

# O instalador do Windows (issue #2). E fonte do pacote como qualquer
# outro: o .iss e os arquivos que ele empacota.
win_setup_files = \
	packages/win_setup/setup.iss \
	packages/win_setup/modpath.iss \
	packages/win_setup/bin/gptshell.bat \
	packages/win_setup/bin/nasm.exe \
	packages/win_setup/copy/BSD.COPYING.txt \
	packages/win_setup/copy/GNU.COPYING.txt \
	packages/win_setup/doc/MUDANÇAS.txt \
	packages/win_setup/notepad++/LEIAME.txt \
	packages/win_setup/notepad++/config.xml \
	packages/win_setup/notepad++/shortcuts.xml \
	packages/win_setup/notepad++/userDefineLang.xml

# Utilitarios de manutencao referidos pelo HACKING.md.
stuff_files = \
	stuff/check-dist.sh \
	stuff/highlight.gpt \
	stuff/iso2utf.sh \
	stuff/svn2cl.php

EXTRA_DIST = INSTALL.default $(doc_DATA) \
	$(test_files) $(win_setup_files) $(stuff_files)

uninstall-devel:
	rm -rf $(includedir)/${PACKAGE}
	rm -f $(libdir)/libgportugol*
