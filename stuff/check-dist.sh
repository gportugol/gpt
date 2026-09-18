#!/bin/sh
#
# Confere se o tarball do `make dist` carrega tudo o que deveria.
#
# Issue #10. O automake so distribui o que esta em SUBDIRS, em EXTRA_DIST e
# na lista de arquivos padrao que ele conhece -- e nao avisa quando um
# arquivo novo fica de fora. O resultado medido antes desta verificacao: de
# 102 arquivos versionados, 88 entravam no tarball. Faltavam a suite de
# testes inteira, o instalador do Windows e a fonte LaTeX do manual.
#
# O modo de falha e silencioso por natureza: o tarball SAI, tem tamanho
# plausivel, e so quem tenta rodar os testes a partir dele descobre. Por
# isso a verificacao e automatica, e nao uma conferida na hora da release.
#
# Uso: stuff/check-dist.sh CAMINHO_DO_TARBALL

set -eu

tarball="${1:?uso: stuff/check-dist.sh CAMINHO_DO_TARBALL}"

# A referencia do que DEVERIA estar no tarball e o indice do git, entao o
# script so faz sentido rodado da raiz de um clone.
git rev-parse --show-toplevel >/dev/null 2>&1 || {
  echo "erro: rode da raiz do repositorio (a lista de referencia vem do git)." >&2
  exit 2
}

# Arquivos versionados que NAO devem entrar no tarball, e por que.
#
# O criterio e o da GNU: a distribuicao carrega o necessario para
# construir, testar, instalar e documentar o pacote. O que serve para
# DESENVOLVER ESTE REPOSITORIO -- CI, linters, projetos de IDE -- nao serve
# a quem baixa o fonte, e fica fora.
fora_de_proposito() {
  case "$1" in
    .github/*)              return 0 ;;  # CI deste repositorio
    .pre-commit-config.yaml) return 0 ;;  # linters deste repositorio
    .markdownlint.yaml)     return 0 ;;  # idem
    gpt.kdevelop)           return 0 ;;  # projeto do KDevelop
    doc/manual/gpt.kilepr)  return 0 ;;  # projeto do Kile
    Makefile.cvs)           return 0 ;;  # bootstrap; o tarball ja traz o configure pronto
    m4/.gitkeep)            return 0 ;;  # marcador de diretorio vazio do git
    *) return 1 ;;
  esac
}

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

# O GNU tar, ao LISTAR, escapa por padrao qualquer byte nao-ASCII:
# MUDANÇAS.txt sai como MUDAN\303\207AS.txt, com barras invertidas
# literais. O git com core.quotepath=false entrega os bytes crus. Sem
# alinhar os dois, todo arquivo com acento no NOME seria reportado como
# ausente -- foi exatamente o falso positivo que apareceu aqui primeiro.
if tar --quoting-style=literal -tzf "$tarball" >/dev/null 2>&1; then
  listar_tarball() { tar --quoting-style=literal -tzf "$1"; }
else
  listar_tarball() { tar -tzf "$1"; }   # bsdtar ja lista literal
fi

# O prefixo gpt-X.Y.Z/ sai fora para comparar com os caminhos do git.
listar_tarball "$tarball" | sed 's|^[^/]*/||' | grep -v '/$' | sort > "$tmp/no_tarball"
git -c core.quotepath=false ls-files | sort > "$tmp/no_git"

# 1) Versionado, deveria estar, e nao esta.
comm -23 "$tmp/no_git" "$tmp/no_tarball" | while IFS= read -r arquivo; do
  fora_de_proposito "$arquivo" && continue
  echo "FALTA no tarball: $arquivo"
done > "$tmp/faltando"

# 2) Esta na lista de excecoes, mas entrou -- ou a lista ficou obsoleta, ou
#    o EXTRA_DIST passou a incluir algo que nao deveria. Nos dois casos, a
#    lista precisa ser corrigida em vez de mentir.
while IFS= read -r arquivo; do
  fora_de_proposito "$arquivo" || continue
  echo "NAO deveria estar no tarball: $arquivo (esta na lista de excecoes de $0)"
done < "$tmp/no_tarball" > "$tmp/sobrando"

faltando=$(wc -l < "$tmp/faltando" | tr -d ' ')
sobrando=$(wc -l < "$tmp/sobrando" | tr -d ' ')

versionados=$(wc -l < "$tmp/no_git" | tr -d ' ')
distribuidos=$(wc -l < "$tmp/no_tarball" | tr -d ' ')
echo "versionados: $versionados | no tarball: $distribuidos"

if [ "$faltando" != 0 ] || [ "$sobrando" != 0 ]; then
  cat "$tmp/faltando" "$tmp/sobrando"
  echo
  echo "Acrescente o arquivo a um EXTRA_DIST (Makefile.am da raiz ou do"
  echo "diretorio correspondente), ou, se ele nao deve ser distribuido,"
  echo "a fora_de_proposito() em stuff/check-dist.sh com o motivo."
  exit 1
fi

echo "OK: o tarball carrega todos os arquivos versionados que deveria."
