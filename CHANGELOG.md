# Changelog

Todas as mudanças relevantes do **GPT (GPortugol)** organizadas por release.
Este arquivo foi consolidado a partir do ChangeLog histórico (CVS/SVN).

## [1.1] - 2010-03-24

### Adicionado

- Estrutura de repetição `repita` (interpretador, tradutor C e backend x86).
- Suporte à instrução `retorne` no bloco principal.
- Melhorias no backend x86.

### Corrigido

- Bug #83 (impressão de literais).
- Impressão incorreta de valores lógicos.
- Impressão de valores nulos e literais.
- Conversão incorreta de números reais (`atof`).
- Diversos bugs em casting, matrizes e expressões literais.
- Correções na geração de código x86 e interpretação.

## [1.0.2a] - 2008-12-11

### Corrigido

- Problemas de compilação com GCC 4.4.
- Ajustes de build em ambientes Debian.

## [1.0.2] - 2008-06-24

### Alterado

- GPT passou a ser linkado dinamicamente com a biblioteca `libgportugol`.

### Corrigido

- Problemas de compilação com GCC 4.3 em múltiplas plataformas.

## [1.0.1] - 2008-03-18

### Adicionado

- Suporte exclusivo a arquivos fonte em UTF-8.
- Instalador para Microsoft Windows.
- Integração com Notepad++.
- Script `gptshell.bat`.
- Documentação adicional para ambiente Windows.

### Alterado

- Limpeza e reorganização de `configure.ac` e `Makefile.am`.
- Revisão das páginas man e do manual.

### Corrigido

- Correções de encoding UTF-8 no Windows.
- Correções em flags e opções de linha de comando.
- Diversos bugs no compilador e ferramentas auxiliares.

## [1.0] - 2006-04-08

### Adicionado

- Biblioteca padrão `base.gpt`.
- Suporte à compilação de algoritmos com múltiplos arquivos.
- Variável de ambiente `GPT_INCLUDE`.
- Nome do algoritmo usado como nome do executável.
- Atualização completa do manual e man pages.

### Alterado

- Melhorias no módulo de depuração.
- Revisão geral das mensagens de erro de compilação.

### Corrigido

- Correções extensivas em:
  - Compilação (incluindo ELF no GNU/Linux).
  - Tradução para C.
  - Interpretação.
  - Análise sintática e semântica.
- Correções de escopo de variáveis (shadowing).
- Correções em operadores, casting e expressões relacionais.
- Correções de loops infinitos e retorno de funções.

## [0.9.2] - 2006-04-05

### Corrigido

- Bug na avaliação de expressões aritméticas.

## [0.9.1] - 2006-03-31

### Corrigido

- Bug de compilação relacionado à função `leia`.

## [0.9b] - 2006-03-08

### Adicionado

- Geração de código executável (x86, PE/ELF).
- Backend NASM.
- Port do GPT para Microsoft Windows (MinGW32).

### Corrigido

- Diversos bugs iniciais (ver ChangeLog histórico).

## [0.8b] - 2006-01-27

### Adicionado

- Primeira versão pública do GPT.
- Funcionalidades iniciais:
  - Interpretação e depuração de algoritmos.
  - Tradução de algoritmos para C.
  - Compilação usando GCC como backend.
