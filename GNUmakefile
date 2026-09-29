# G-Portugol -- atalhos de desenvolvimento.
#
# Este NAO e o Makefile do build. O build e autotools: o `configure` gera
# um `Makefile` de verdade, e e ele quem compila, instala e distribui.
#
# O GNU make le `GNUmakefile` antes de `Makefile` quando os dois existem,
# e e por isso que este arquivo se chama assim: ele consegue oferecer
# `make build`, `make test`, `make dist` a partir de um clone recem-feito,
# sem que ninguem precise decorar a sequencia
# `autoreconf -i && ./configure && make`.
#
# Todo alvo que nao esta definido aqui e REPASSADO ao Makefile gerado,
# entao `make install`, `make distclean`, `make distcheck` e qualquer outro
# alvo do automake continuam funcionando exatamente como antes.

# `help` tem de ser o alvo padrao explicitamente. As regras vazias logo
# abaixo vem antes de qualquer outra, e sem esta linha o primeiro alvo do
# arquivo -- e portanto o padrao -- passaria a ser `Makefile`: um `make`
# sem argumentos respondia "'Makefile' is up to date" em vez da ajuda.
.DEFAULT_GOAL := help

# Sem isto, a regra generica la embaixo tentaria "reconstruir" os proprios
# makefiles e o configure, e o make entraria em recursao.
Makefile: ;
GNUmakefile: ;
configure: ;
configure.ac: ;
Makefile.am: ;

CONFIGURE_FLAGS ?=
IMAGEM_CI ?= ubuntu:24.04

.PHONY: help deps bootstrap build test lint manual dist check-dist ci clean distclean

help:
	@echo "G-Portugol -- atalhos de desenvolvimento"
	@echo ""
	@echo "Primeiros passos:"
	@echo "  make deps        - instala as dependencias de build (Debian/Ubuntu)"
	@echo "  make build       - compila (faz bootstrap e configure se preciso)"
	@echo "  make test        - roda test/run_test.sh"
	@echo ""
	@echo "Distribuicao:"
	@echo "  make dist        - gera o tarball do fonte e confere se esta completo"
	@echo "  make check-dist  - so a conferencia, sobre um tarball ja gerado"
	@echo "  make manual      - gera doc/manual.pdf a partir do LaTeX"
	@echo ""
	@echo "Qualidade:"
	@echo "  make lint        - roda os hooks do pre-commit em todos os arquivos"
	@echo "  make ci          - repete build+test dentro de um container limpo"
	@echo "                     (IMAGEM_CI=$(IMAGEM_CI))"
	@echo ""
	@echo "Limpeza:"
	@echo "  make clean       - remove os objetos do build"
	@echo "  make distclean   - remove tambem o que o configure gerou"
	@echo ""
	@echo "Opcoes:"
	@echo "  CONFIGURE_FLAGS='--prefix=/opt/gpt'  passa argumentos ao configure"
	@echo ""
	@echo "Qualquer outro alvo vai para o Makefile do autotools:"
	@echo "  make install, make distcheck, make uninstall, ..."

# As dependencias do build, iguais as do .github/workflows/build.yml.
#
# O pacote `antlr` entra explicitamente: quem traz o binario `runantlr`,
# que o configure procura, e ele -- o `libantlr-dev` apenas o RECOMENDA, e
# uma instalacao com --no-install-recommends passa pelo apt e so falha la
# no configure, com "o programa antlr nao foi encontrado".
deps:
	@command -v apt-get >/dev/null 2>&1 || { \
	  echo "make deps so cobre Debian/Ubuntu."; \
	  echo "Instale o equivalente a: antlr libantlr-dev libpcre2-dev nasm build-essential autoconf automake libtool pkg-config"; \
	  exit 1; \
	}
	sudo apt-get update
	sudo apt-get install -y build-essential autoconf automake libtool pkg-config \
	  antlr libantlr-dev libpcre2-dev nasm

bootstrap: configure.ac
	autoreconf -i

# O `configure` so roda de novo quando faz sentido: se ele nao existe, ou
# se o configure.ac mudou depois dele.
config.status: configure.ac
	@test -f configure || $(MAKE) bootstrap
	./configure $(CONFIGURE_FLAGS)

build: config.status
	@$(MAKE) -f Makefile all

test: build
	bash test/run_test.sh

dist: build
	@$(MAKE) -f Makefile dist
	@$(MAKE) check-dist

# A conferencia de completude do tarball chega com o PR do #10. Enquanto
# ela nao existir, `make dist` gera o tarball e avisa que ninguem conferiu
# -- em vez de falhar por causa de um script ausente, ou, pior, de dar a
# impressao de que conferiu.
check-dist:
	@tarball=$$(ls -t gpt-*.tar.gz 2>/dev/null | head -1); \
	test -n "$$tarball" || { echo "nenhum gpt-*.tar.gz aqui -- rode 'make dist'"; exit 1; }; \
	if test -x stuff/check-dist.sh; then \
	  stuff/check-dist.sh "$$tarball"; \
	else \
	  echo "gerado: $$tarball (sem conferencia de completude -- ver issue #10)"; \
	fi

manual:
	@command -v pdflatex >/dev/null 2>&1 || { \
	  echo "pdflatex nao encontrado. No Debian/Ubuntu:"; \
	  echo "  sudo apt-get install texlive-latex-base texlive-lang-portuguese"; \
	  exit 1; \
	}
	@# Tres passadas: a primeira gera o .toc, a segunda o resolve, a
	@# terceira acerta as referencias cruzadas. E o que o CI faz.
	pdflatex -interaction=nonstopmode -output-directory=doc doc/manual/manual.tex
	pdflatex -interaction=nonstopmode -output-directory=doc doc/manual/manual.tex
	pdflatex -interaction=nonstopmode -output-directory=doc doc/manual/manual.tex
	@echo "gerado: doc/manual.pdf"

lint:
	@command -v pre-commit >/dev/null 2>&1 || { \
	  echo "pre-commit nao encontrado: pipx install pre-commit"; \
	  exit 1; \
	}
	pre-commit run --all-files

# Repete o build do CI num container limpo, sobre uma COPIA do fonte.
#
# Existe porque a maior parte do tempo perdido neste projeto e de
# divergencia de ambiente -- a versao do ANTLR, o pacote que falta, a
# distribuicao. Aqui da para reproduzir o ambiente do CI sem esperar por
# ele, e sem sujar a arvore local.
ci:
	@command -v docker >/dev/null 2>&1 || { echo "docker nao encontrado."; exit 1; }
	docker run --rm -v "$$PWD:/fonte:ro" $(IMAGEM_CI) sh -c '\
	  set -e; \
	  export DEBIAN_FRONTEND=noninteractive; \
	  apt-get update -qq; \
	  apt-get install -y -qq build-essential autoconf automake libtool \
	    pkg-config antlr libantlr-dev libpcre2-dev nasm; \
	  cp -a /fonte /trabalho; cd /trabalho; \
	  autoreconf -i; ./configure; make -j"$$(nproc)"; \
	  bash test/run_test.sh'

clean: config.status
	@$(MAKE) -f Makefile clean

distclean: config.status
	@$(MAKE) -f Makefile distclean

# Qualquer alvo nao definido acima e do autotools. Repassar em vez de
# reimplementar mantem `make install`, `make distcheck` e companhia
# identicos ao que sempre foram.
%: config.status
	@$(MAKE) -f Makefile $@
