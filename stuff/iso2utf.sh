#!/bin/sh
#
# Converte para UTF-8 os arquivos versionados que ainda estao em ISO-8859-1.
#
# Issue #17. A versao anterior deste script convertia INCONDICIONALMENTE, e
# o repositorio hoje ja e quase todo UTF-8: rodada agora, ela produzia
# duplo-encode em 16 arquivos de src/ (`á` viraria `Ã¡`) e apontava para
# quatro caminhos da epoca do SVN que nao existem mais -- README, INSTALL,
# NEWS e exemplos/highlight.gpt, renomeados ou removidos desde entao.
#
# Esta versao TESTA antes de converter, entao e idempotente: rodar duas
# vezes seguidas nao muda nada na segunda.
#
# Uso:
#   stuff/iso2utf.sh              # relata o que converteria, sem escrever
#   stuff/iso2utf.sh --escrever   # converte de fato

set -eu

escrever=no
if [ "${1:-}" = "--escrever" ]; then
  escrever=sim
fi

# Os arquivos que ficam em Latin-1 de proposito estao em
# stuff/check-encoding.py, com o motivo de cada um. A lista e repetida aqui
# de forma minima so para nao converter o que o hook depois recusaria.
eh_excecao() {
  case "$1" in
    packages/win_setup/setup.iss) return 0 ;;
    test/asm/win32/header.asm)    return 0 ;;
    test/asm/win32/nagoa+.inc)    return 0 ;;
    *) return 1 ;;
  esac
}

# Binario e "tem byte NUL", que e o teste do proprio git.
eh_binario() {
  [ "$(LC_ALL=C tr -dc '\000' < "$1" | wc -c | tr -d ' ')" != 0 ]
}

# `git ls-files` em vez de `find`: pega o que esta versionado e nada do que
# o build deixou para tras. O awk da versao anterior tambem dependia do
# gawk, que nem toda maquina tem.
git ls-files | while IFS= read -r arquivo; do
  [ -f "$arquivo" ] || continue
  eh_binario "$arquivo" && continue

  # iconv de utf-8 para utf-8 falha exatamente quando a entrada nao e
  # UTF-8 valido -- que e a pergunta que interessa.
  if iconv -f utf-8 -t utf-8 "$arquivo" >/dev/null 2>&1; then
    continue
  fi

  if eh_excecao "$arquivo"; then
    echo "pulando      $arquivo  (Latin-1 de proposito -- ver stuff/check-encoding.py)"
    continue
  fi

  if [ "$escrever" = sim ]; then
    echo "convertendo  $arquivo"
    iconv -f iso-8859-1 -t utf-8 "$arquivo" > "$arquivo.utf8tmp"
    mv "$arquivo.utf8tmp" "$arquivo"
  else
    echo "converteria  $arquivo"
  fi
done

if [ "$escrever" != sim ]; then
  echo
  echo "Nada foi escrito. Rode com --escrever para converter."
fi
