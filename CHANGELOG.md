# Changelog

Todas as mudanças relevantes do **GPT (GPortugol)** organizadas por release.
Este arquivo foi consolidado a partir do ChangeLog histórico (CVS/SVN).

## [Unreleased]

### Alterado

- Sem `-o`, o programa compilado recebe o nome do primeiro arquivo fonte, sem
  o diretório e a extensão, e não mais o identificador de
  `algoritmo <nome>;`. Esse identificador deixava o fonte escolher qual
  arquivo do diretório atual seria substituído: `algoritmo Makefile;`
  transformava o `Makefile` num executável. Um fonte sem extensão gera
  `<nome>.out` (`.exe` no Windows), e no Windows um fonte `*.exe` gera
  `<nome>.out.exe`. Quem dependia do nome antigo precisa passar `-o`.
- O depurador (`-H`) resolve o host com `getaddrinfo`, então aceita IPv6 e
  hosts com vários endereços, e avisa em stderr quando o host não resolve ou
  não aceita a conexão. Antes o programa rodava sem depurador, em silêncio.
  O `SECURITY.md` descreve o protocolo, que é TCP sem autenticação, e
  recomenda usar `-H` só em localhost ou numa rede confiável.
- A versão `latest` do manual no Read the Docs é gerada a partir da `master`
  e mostra mudanças que ainda não saíram numa release. Antes ela respondia
  404. A raiz do site continua apontando para a `stable`.

### Corrigido

- `gpt -v` e `gpt -h` saíam com código 1, o que quebrava scripts que
  conferem a versão. Agora saem com 0, e `gpt` sem argumentos continua saindo
  com 1.
- O `bin/gpt` do pacote Linux da release vinha sem permissão de execução, e
  era preciso rodar `chmod +x` depois de extrair.
- Divisão e módulo por zero, inteiros ou reais, encerram o programa nos três
  modos com a mesma mensagem, a linha do operador e código de saída 1. Antes
  o interpretador morria com SIGFPE no `%`, e o executável nativo e a
  tradução para C morriam com SIGFPE na divisão inteira e dividiam reais por
  zero sem aviso. A tradução para C também morria em `INT_MIN / -1`.
- No executável nativo, a divisão e o módulo de números negativos davam
  resultados errados (`-7 / 2` dava 2147483644 e `-7 % 3` dava 0). No
  interpretador, `-x` com `x = 1234567` imprimia `-1` e `-2147483648` saía
  como `-2`.
- Uma recursão sem fim derrubava o interpretador com falha de segmentação.
  Agora ele mede a pilha usada e para com
  `Pilha de execução esgotada (recursão muito profunda)` e a linha da
  chamada. O executável nativo e a tradução para C ainda terminam com falha
  de segmentação.
- O `leia()` do executável nativo lia de uma vez até 1024 bytes. De um pipe
  ou arquivo, o primeiro `leia()` recebia várias linhas e os seguintes não
  recebiam nada. No fim da entrada o valor ficava sem terminador, uma linha
  sem quebra perdia o último caractere, e um programa que lia cerca de mil
  linhas esgotava a memória. No Windows, o executável nativo não lia entrada
  redirecionada. Agora cada `leia()` lê exatamente uma linha nos dois
  sistemas, e uma linha maior que 1023 bytes é cortada.
- Uma linha vazia ou o fim da entrada fazem `leia()` de literal devolver o
  literal nulo nos três modos, como o interpretador já fazia. Antes a
  tradução para C devolvia `""` para a linha vazia e terminava com erro no
  fim da entrada.
- Na tradução para C, `leia()` de literal removia o último caractere de uma
  linha sem quebra (`abc` virava `ab`) e, numa linha que começava com o
  byte NUL, escrevia um byte antes do buffer.
- `gpt -i -H ... | head -1` terminava com código 0 em vez de morrer com
  SIGPIPE, como acontece sem `-H`.

### Segurança

- O executável nativo e a tradução para C não conferiam os índices de
  matriz. Um índice lido com `leia()` permitia escolher onde uma escrita de 4
  bytes caía na memória do programa. Agora os três modos conferem cada
  índice e encerram com a mensagem do interpretador e código de saída 1.
- O ELF gerado não tinha `PT_GNU_STACK`, e os dados começavam na mesma
  página do código. Por isso código, dados, a área de memória do runtime e a
  pilha eram mapeados com permissão de leitura, escrita e execução. Agora o
  código é mapeado só para leitura e execução, e os dados, a área de memória
  e a pilha só para leitura e escrita.
- O depurador lia as mensagens do host num buffer de 1024 bytes na pilha sem
  nenhum limite, e um cabeçalho ou um tamanho declarado maiores estouravam a
  pilha. O cabeçalho agora precisa caber no buffer, e o corpo é limitado a
  64 KiB. Uma mensagem malformada encerra a conexão com o depurador, e o
  programa continua rodando.
- O XML enviado ao depurador levava nomes e valores de variáveis sem escape.
  Um valor com aspas, vindo de um literal ou de `leia()`, injetava marcação
  no documento que o cliente lê. Agora o texto é escapado, e o que o XML não
  permite, como bytes que não são UTF-8 válido, vira U+FFFD.

## [1.3.0] - 2026-10-16

### Adicionado

- Suíte de regressão em `test/casos/` e `test/erros/`: `test/run_test.sh`
  compara a saída dos três modos de execução com as saídas esperadas e as
  mensagens de erro de compilação.
- Manual publicado no Read the Docs (<https://gportugol.readthedocs.io/>),
  gerado a cada release. O PDF continua anexado à release.
- `CONTRIBUTING.md`, `SECURITY.md` e `AGENTS.md`, distribuídos e instalados
  com o restante da documentação.
- Dependabot para manter as GitHub Actions atualizadas.
- Cada release passa a publicar também o tarball do código-fonte gerado pelo
  `make dist`, junto com os binários e o manual.

### Alterado

- O binário para Windows passa a ser compilado no ambiente UCRT64 do MSYS2,
  que descontinuou o MINGW64, e depende da UCRT, incluída no Windows 10 e no
  Windows Server 2016 em diante.
- Arquivos fonte que não estão em UTF-8 são rejeitados com
  `gpt: <arquivo>: o arquivo não está em UTF-8`. Antes, um programa salvo em
  ISO-8859-1 gerava uma série de caracteres inválidos e aspas não fechadas,
  sem indicar a codificação.
- `gpt -i` termina com código de saída 1 quando o programa tem erro de
  sintaxe ou semântico. Antes saía com 0.
- `gpt -o` recusa gerar executável em máquinas que não são x86 e sugere `-t`,
  e qualquer falha do `nasm`, inclusive quando ele não é encontrado, passa a
  ser erro. Antes o `gpt` saía com 0 nesses casos.
- `gpt -o` procura o `nasm` primeiro no diretório do próprio `gpt` (no
  GNU/Linux e no Windows) e depois no `PATH`, então o pacote para Windows
  funciona sem colocar o `nasm` no `PATH`.
- O `configure` passa a respeitar o `CXXFLAGS` informado pelo usuário, que
  antes era descartado.
- O manual foi convertido de LaTeX para Markdown com Sphinx e revisado para
  refletir o comportamento atual: a saída real de `-h`, `repita` na tabela de
  palavras reservadas, dígitos octais de 0 a 7, identificadores que podem
  começar com `_` e `;` como separador do `GPT_INCLUDE` no Windows.
- O `HACKING.md` explica por que o Windows precisa do NASM 0.99.05 (as
  versões 2.16.01 e 3.01 rejeitam o assembly gerado) e como rodar a suíte de
  regressão.
- Os downloads usados no build da release (ANTLR 2.7.7, JDK e NASM) são
  conferidos por SHA-256, e as GitHub Actions são fixadas por commit.
- O push só dispara o CI na `master`, então um commit de pull request deixa
  de rodar duas vezes, pelo push e pelo evento `pull_request`.
- O job Linux do CI compila com `-Wall -Wextra -Werror`, então um aviso novo
  do compilador quebra o build. O `HACKING.md` mostra como fazer a mesma
  verificação antes de enviar.

### Corrigido

- Arquivos fonte salvos com BOM UTF-8, comum em editores do Windows, eram
  rejeitados na primeira linha com "Caractere inválido".
- No interpretador, `retorne` dentro de `enquanto`, `repita`, `para` ou de um
  `se` não encerrava a função: as instruções seguintes continuavam a rodar, e
  um `enquanto verdadeiro` com `retorne` nunca terminava. Agora `gpt -i` se
  comporta como o executável nativo e a tradução para C.
- Chamar uma variável como função (`a()`) era aceito e derrubava o
  interpretador com falha de segmentação. Agora é rejeitado com
  `"a" não é uma função.`, com arquivo e linha.
- Uma função que retorna um literal vazio fazia o programa traduzido para C
  terminar com falha de segmentação.
- No backend x86, `retorne` sem valor lia uma variável não inicializada para
  decidir se emitia instruções de conversão x87.
- No Windows, caracteres de U+0080 a U+00BF nas mensagens, como o espaço
  inseparável, `º` e `°`, apareciam no console como dois caracteres errados.
- Um literal octal com letra, como `0c7e`, era descrito como hexadecimal na
  mensagem de erro, e três mensagens terminavam com ponto duplo.
- `gpt -o` montava comandos de shell com o nome do executável e o caminho do
  arquivo temporário. Uma pasta temporária com espaços no caminho fazia o
  `nasm` falhar. No GNU/Linux, um executável com espaços no nome ficava sem
  permissão de execução, e um nome com `$( )`, crases ou `;` executava os
  comandos contidos nele. Agora o `nasm` roda sem shell e a permissão de
  execução é dada por `chmod(2)`.
- O arquivo temporário do `gpt -o` ignorava o `TMPDIR` no GNU/Linux e, no
  Windows, tinha nome fixo e ficava fora da pasta `TEMP`, porque o nome era
  colado ao caminho sem separador (`...\Tempgpt_tmp`). Agora ele respeita o
  `TMPDIR` e, no Windows, cada compilação cria um arquivo com nome novo na
  pasta temporária.
- Uma declaração de matriz com as dimensões e o tipo dos elementos errados,
  como `a: matriz[1x2] de inteiro;`, derrubava o `gpt` com falha de
  segmentação em `-i`, `-t` e `-o`. Agora os dois erros são reportados com
  arquivo e linha.
- O interpretador lia o tipo não inicializado do valor de retorno de uma
  função que não retorna nada, a tradução para C lia o escopo não
  inicializado num `retorne` do bloco principal, e o parser recebia uma
  profundidade de lookahead indeterminada. Nenhum desses casos mudava a saída
  dos testes, mas o valgrind e o GCC os apontavam.
- `INSTALL.md` ficava fora do `make dist` e do `make install`, e o script do
  instalador do Windows citava `README.pt_BR` e `NEWS`, que não existem mais.

## [1.2.0] - 2026-01-06

### Adicionado

- Geração automática do manual em LaTeX durante o processo de build.
- Publicação do manual em PDF como artefato da release.
- Novos exemplos de código, instalados junto com o projeto.
- Novo site oficial do projeto: gportugol.github.io.
- Workflows de GitHub Actions para:
- Execução de testes unitários.
- Build e testes de binários.
- Build em ambiente Windows (MinGW64).
- Upload automático de artefatos de CI para as releases.
- Hooks de pre-commit para padronização e qualidade de código.
- Arquivo de configuração .markdownlint.yaml.

### Alterado

- Port do projeto para PCRE2, substituindo a biblioteca anterior.
- Conversão da documentação para Markdown:
  - README
  - INSTALL
  - HACKING
  - ChangeLog
  - NEWS
- Reorganização e refatoração geral dos arquivos README.
- Atualização da seção de referências do README.
- Ajustes no sistema de build (autoupdate e limpeza de arquivos).
- Atualização do EXTRA_DIST para usar variantes em Markdown.
- Remoção do diretório debian do repositório principal.
- Refatoração do manual (layout, codificação UTF-8 e correções na geração do
  PDF).

### Corrigido

- Diversos warnings de compilação.
- Problemas de compilação em ambientes mais antigos (ex.: GCC 4.7).
- Ajustes e melhorias na suíte de testes.
- Padronização e formatação completa do código-fonte.
- Inclusão de headers ausentes que causavam falhas de build (FTBFS).

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
