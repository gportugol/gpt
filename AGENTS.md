# Guia para agentes

## Visão geral

G-Portugol é uma implementação em C++ da linguagem didática Portugol. O binário
`gpt` pode interpretar programas, compilá-los para executáveis x86, gerar
assembly e traduzir para C. O projeto usa Autotools, ANTLR 4.x e PCRE2, e deve
continuar compatível com GNU/Linux e Windows/MSYS2.

## Estrutura do repositório

- `src/`: ponto de entrada e coordenação do compilador.
- `src/modules/parser/`: gramática ANTLR4 (`Portugol.g4`), relatório de erros
  sintáticos em português (`PortugolErrorStrategy`) e análise semântica
  (`SemanticAnalyzer`/`SemanticEval`), que também anota o tipo de cada
  expressão em `SymbolTable::setEvalType()` para os geradores de código.
- `src/modules/interpreter/`: interpretador (`Interpreter`, sobre
  `InterpreterEval`) e depurador.
- `src/modules/x86/`: gerador de assembly x86 (`X86Generator`, sobre `X86`) e
  trechos de runtime.
- `src/modules/c_translator/`: tradutor de Portugol para C
  (`Portugol2CTranslator`).
- `lib/base.gpt`: biblioteca padrão da linguagem.
- `test/`: `run_test.sh`, o programa `tester.gpt`, os casos de regressão em
  `casos/` (programas com saída esperada) e `erros/` (programas inválidos com
  a mensagem esperada).
- `exemplos/`: programas de exemplo.
- `doc/`: páginas de manual e fonte LaTeX do manual.
- `packages/win_setup/`: empacotamento e recursos do instalador Windows.

## Ambiente, build e testes

No GNU/Linux, instale as dependências descritas em `HACKING.md`: compilador
C++, make, autoconf, automake, libtool, pkg-config, ANTLR 4.x (a ferramenta
`antlr4`, que roda em Java, e o runtime C++ `libantlr4-runtime`), PCRE2 e
NASM.

```bash
autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
bash test/run_test.sh
```

O teste requer `src/gpt` já compilado. Ele executa `test/tester.gpt` e cada
programa de `test/casos/` nos três modos (interpretação, binário nativo e
tradução para C compilada com `gcc`), comparando stdout e código de saída com
os arquivos `.saida`/`.codigo`, e verifica que cada programa de `test/erros/`
é rejeitado com a mensagem em `.msg`. Em hosts que não são x86, a execução do
binário nativo é pulada intencionalmente.

Para uma verificação rápida e manual:

```bash
src/gpt -i exemplos/olamundo.gpt
src/gpt -o olamundo exemplos/olamundo.gpt && ./olamundo
```

Não execute `make install` sem uma razão explícita. Para validar a instalação
sem alterar o sistema, use `make install DESTDIR="$PWD/release"`.

## Regras para mudanças

- Mantenha o escopo mínimo e preserve compatibilidade com o estilo C++ já
  presente no arquivo modificado; o código legado usa `std::string` exposto por
  headers e ponteiros crus em diversos pontos.
- Rode `pre-commit run --all-files` quando disponível. O hook aplica
  `clang-format` a C/C++ e também verifica espaços finais, conflitos e arquivos
  grandes.
- Ao alterar comportamento da linguagem, adicione ou ajuste um caso em
  `test/casos/` ou `test/erros/` (ou em `test/tester.gpt`) e execute o script
  de testes. Os três modos de execução devem produzir a mesma saída; use
  `NOME.saida.nativo` ou `NOME.saida.c` apenas para divergências conhecidas.
- Ao alterar opções da CLI, atualize a ajuda em `src/GPT.cpp` e a documentação
  aplicável.
- Preserve os avisos de licença/copyright existentes nos arquivos C++ que forem
  modificados. Não troque a licença GPL-2.0 do projeto.

## ANTLR e arquivos gerados

O lexer, o parser e as classes de visitor são gerados durante o build a partir
de `src/modules/parser/Portugol.g4` (`BUILT_SOURCES`/`CLEANFILES` em
`src/modules/parser/Makefile.am`). Edite a gramática, não os arquivos gerados
(`PortugolLexer.cpp`, `PortugolParser.cpp`, `PortugolVisitor.cpp`,
`PortugolBaseVisitor.cpp` e headers correspondentes).

A análise semântica e os três geradores de código são classes C++ escritas à
mão que percorrem a árvore sintática (`PortugolParser::*Context`). Ao mudar a
gramática, faça um build limpo o suficiente para forçar a geração e confira os
quatro consumidores. Os geradores dependem dos tipos anotados pelo
`SemanticAnalyzer` (`SymbolTable::getEvalType`).

## Portabilidade

- Evite dependências e comandos exclusivos de Linux no código do produto.
- Teste ou considere os caminhos condicionais de Windows ao mexer em arquivos,
  processos temporários, assembly e flags de compilação.
- O CI é a referência para os builds Linux e Windows; mantenha `HACKING.md` e
  os workflows coerentes quando o processo de build mudar.

## Antes de concluir

Relate os arquivos alterados, os comandos de validação executados e qualquer
limitação de ambiente (por exemplo, ANTLR/NASM indisponível). Não inclua
artefatos de build, binários de teste ou arquivos temporários no patch.
