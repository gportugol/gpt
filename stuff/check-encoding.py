#!/usr/bin/env python3
"""Recusa arquivo de texto que nao seja UTF-8 valido.

O repositorio e UTF-8, com excecoes contadas -- a lista LATIN1_PERMITIDOS
abaixo. O objetivo deste hook nao e converter nada: e impedir que a lista
cresca sem que alguem decida que deve crescer.

Cada excecao carrega o motivo ao lado. Quando um caso for resolvido, some
a linha daqui junto com a conversao, e o hook passa a cobrar o arquivo --
o que torna a conversao verificavel em vez de declarada.

Uso: check-encoding.py ARQUIVO...  (o pre-commit passa os arquivos alterados)
"""

import sys

# Arquivos que NAO sao UTF-8 hoje, e por que.
LATIN1_PERMITIDOS = {
    # Definitivo: o Inno Setup le o .iss como ISO-8859-1 fora do modo
    # Unicode, entao converter quebraria os acentos do instalador.
    'packages/win_setup/setup.iss':
        'o Inno Setup exige ISO-8859-1 fora do modo Unicode',

    # Definitivo: codigo de terceiro vendorizado. Os acentos estao so em
    # comentarios e creditos, mas converter cria diff permanente contra o
    # upstream e complica qualquer atualizacao.
    'test/asm/win32/header.asm':
        'codigo de terceiro vendorizado',
    'test/asm/win32/nagoa+.inc':
        'codigo de terceiro vendorizado (biblioteca NAGOA+)',

    # PENDENTE -- issue #17, caso 2. Converter muda o que o programa
    # IMPRIME, e nao so como o arquivo e gravado. E decisao, nao formatacao.
    'test/asm/test.asm':
        'pendente: a string de runtime muda de comportamento (#17)',
    'test/asm/win32/teste.asm':
        'pendente: a string de runtime muda de comportamento (#17)',

    # PENDENTE -- issue #17, caso 4. Fixture orfa: nada a referencia, e os
    # bytes divergem dos literais de token da gramatica. Converter ou
    # apagar depende de ela ainda servir para alguma coisa.
    'test/feed_test':
        'pendente: fixture orfa, converter ou apagar (#17)',
}


def eh_binario(dados: bytes) -> bool:
    """Um NUL em qualquer lugar e o teste que o proprio git usa."""
    return b'\0' in dados


def main(caminhos: list[str]) -> int:
    problemas = []

    for caminho in caminhos:
        try:
            with open(caminho, 'rb') as arquivo:
                dados = arquivo.read()
        except OSError:
            # Arquivo removido no mesmo commit: nao e problema deste hook.
            continue

        if eh_binario(dados):
            continue

        try:
            dados.decode('utf-8')
        except UnicodeDecodeError as erro:
            if caminho in LATIN1_PERMITIDOS:
                continue
            problemas.append((caminho, erro))
            continue

        # O contrario tambem importa: um arquivo da lista que JA foi
        # convertido deve sair dela, ou a lista vira ficcao.
        if caminho in LATIN1_PERMITIDOS:
            problemas.append((
                caminho,
                'esta na lista de excecoes mas ja e UTF-8 valido: '
                'remova a entrada de stuff/check-encoding.py',
            ))

    for caminho, motivo in problemas:
        print(f'{caminho}: {motivo}', file=sys.stderr)

    if problemas:
        print(
            '\nO repositorio e UTF-8. Converta com:\n'
            '    iconv -f iso-8859-1 -t utf-8 ARQUIVO > tmp && mv tmp ARQUIVO\n'
            'ou, se o arquivo tiver de continuar em Latin-1, acrescente-o a\n'
            'LATIN1_PERMITIDOS em stuff/check-encoding.py com o motivo.',
            file=sys.stderr,
        )
        return 1

    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))
