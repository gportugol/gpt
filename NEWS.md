# NEWS

Notícias e destaques das versões do **GPT (GPortugol)**.

Este arquivo apresenta um resumo das principais mudanças de cada versão, com
foco em funcionalidades e correções relevantes para os usuários.

## 1.1 — 2010-03-24

- Implementação da nova estrutura de repetição `repita`.
- Suporte à instrução `retorne` no bloco principal.
- Correção do bug #83 (ver Mantis).
- Correções na impressão de valores lógicos, nulos e literais.
- Correções na representação e manipulação de números reais.

## 1.0.2a — 2008-12-11

- Correções de compatibilidade na compilação com GCC 4.4.

## 1.0.2 — 2008-06-24

- O GPT passou a depender da biblioteca `libgportugol`.
- Correções de compatibilidade na compilação com GCC 4.3.

## 1.0.1 — 2008-03-18

- Suporte exclusivo a arquivos fonte codificados em UTF-8.
- Novo instalador para sistemas Microsoft Windows.
- Integração do compilador com o Notepad++ no Windows.
- Adição do script `gptshell.bat` para facilitar o uso no Windows.
- Correções em flags de tradução do compilador.
- Diversas correções de bugs.

## 1.0 — 2006-04-08

- Primeira versão estável do GPT.
- Inclusão da biblioteca padrão (`base.gpt`) com funções básicas.
- Melhorias no módulo de depuração.
- Revisão e correção das mensagens de erro de compilação.
- Correções importantes na compilação em GNU/Linux (formato ELF).
- Correções extensivas em compilação, tradução e interpretação.
- Suporte à compilação de algoritmos com múltiplos arquivos.
- Variável de ambiente `GPT_INCLUDE` para inclusão automática de algoritmos.
- Atualização da documentação, manual e páginas man.
- O nome do algoritmo passa a ser usado como nome do executável.

## 0.9.2 — 2006-04-05

- Correção de bug na avaliação de expressões aritméticas.

## 0.9.1 — 2006-03-31

- Correção de bug de compilação relacionado à função `leia`.

## 0.9b — 2006-03-08

- Implementação da geração de código executável (x86, PE/ELF).
- Backend NASM.
- Port do GPT para Microsoft Windows (testado com MinGW32).
- Diversas correções de bugs.

## 0.8b — 2006-01-27

- Primeira versão pública do GPT.
- Funcionalidades iniciais:
  - Interpretação e depuração de algoritmos.
  - Tradução de algoritmos para C.
  - Compilação de algoritmos usando o GCC como backend.
