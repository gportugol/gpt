# Changelog

Todas as mudanças relevantes do **GPT (GPortugol)** organizadas por release.
Este arquivo foi consolidado a partir do ChangeLog histórico (CVS/SVN).

## [Unreleased]

### Adicionado

- Backend AArch64 para GNU/Linux. Numa máquina arm64, `gpt -o` gera o
  executável com o `as` e o `ld` do binutils, e `gpt -s` grava o assembly
  GNU correspondente. Em outra máquina, o `gpt` chama
  `aarch64-linux-gnu-as` e `aarch64-linux-gnu-ld`. Esse backend não usa o
  NASM, que não monta código AArch64.
- Opção `-a <arq>`, que escolhe a arquitetura do código de `-o` e `-s`:
  `x86-64` (ou `x86_64` e `amd64`) ou `arm64` (ou `aarch64`). Sem ela, vale a
  arquitetura da máquina que roda o `gpt`. As páginas man e o manual
  descrevem a opção e as ferramentas que cada arquitetura usa.
- Cada release publica também `gpt-linux-aarch64.tar.gz`, compilado e
  testado no CI num runner arm64.
- O `configure` acrescenta as flags de proteção que o compilador aceitar:
  `-D_FORTIFY_SOURCE=2` (se o compilador ou o `CPPFLAGS` ainda não definem
  um nível), `-D_GLIBCXX_ASSERTIONS`, `-fstack-protector-strong`,
  `-fstack-clash-protection`, `-fcf-protection` (no arm64,
  `-mbranch-protection=standard`), `-Wl,-z,relro` e `-Wl,-z,now`. As flags
  do usuário vêm depois e prevalecem, e `--disable-hardening` desliga todas.

### Alterado

- Migração do ANTLR 2.7.7 para o ANTLR 4: a gramática passa a ser
  `src/modules/parser/Portugol.g4` e o lexer/parser são gerados pela
  ferramenta `antlr4` (Java) com o runtime C++ `libantlr4-runtime`. A
  análise semântica, o interpretador, o gerador x86 e o tradutor para C são
  agora classes C++ que percorrem a árvore sintática, portadas dos walkers
  ANTLR2 originais, e produzem a mesma saída da versão 1.4.0. O build
  exige um compilador C++17 e um runtime 4.9 ou mais novo da mesma versão
  4.x da ferramenta (com o runtime 4.9, a ferramenta pode ser de 4.7 a 4.9).
  O `configure` recusa outras combinações, como a ferramenta 4.9.2 com o
  runtime 4.10 do Ubuntu 24.04, cujo parser gerado não compila. Como o da
  1.4.0, o `gpt` dos tarballs Linux da release não depende do runtime do
  ANTLR: ele vai ligado estaticamente, com o aviso de licença em
  `share/doc/libantlr4-runtime`.
- Mensagens de erro sintático e léxico mantêm o formato
  `arquivo:linha - Esperando X, encontrado Y` e as dicas da opção `-d`.
  Algumas mensagens mudam com a recuperação de erros do novo parser.
- Strings aceitam qualquer caractere UTF-8; literais de caractere continuam
  limitados ao Latin-1.
- Uma quebra de linha ou um CR crus dentro de um literal de caractere (`'`,
  Enter e `'`) agora dão "Faltando fechar aspas.", como já acontecia nas
  strings. A 1.4.0 aceitava esse literal, embora o C gerado por `-t` não
  compilasse. Os escapes `'\n'` e `'\r'` dão o mesmo caractere nos três
  modos.
- Um `não` sem operando, como em `b := não;` ou `imprima(não, 1)`, agora é
  erro de sintaxe (`Esperando expressão, encontrado ; após "não".`). A 1.4.0
  aceitava o programa com o aviso `<AST>: unexpected AST node: <ASTNULL>`, e
  o resultado dependia do modo: `b := não;` dava `verdadeiro` no
  interpretador e `falso` no executável, `imprima(não, 1)` mostrava
  `verdadeiro1` no interpretador e só `1` no executável, e o C gerado por
  `-t` não compilava. `-`, `+` e `~` sem operando já eram rejeitados e
  passam a dar a mesma mensagem.
- `gpt -o` gera executáveis x86-64, ELF64 no GNU/Linux e PE32+ no Windows,
  no lugar de ELF32 e PE32. Os reais passam a ser `double` em SSE2, e não
  mais `float` no x87. Um executável gerado agora exige um sistema de 64
  bits.
- No Windows, qualquer NASM 2.x ou mais novo monta o código gerado, e o zip
  da release traz o `nasm.exe` do MSYS2 com as DLLs de que ele depende, as
  licenças de cada pacote em `share/licenses` e, em
  `share/doc/nasm/VERSION`, o pacote, a versão e o SHA-256 do `nasm.exe`.
  Antes o template PE32 dependia de um pacote de macros que só o NASM
  0.99.05 aceitava.
- Sem `-a`, `gpt -o` recusa gerar executável numa máquina que não é x86-64
  nem arm64 e sugere `-t`. A exceção é o GNU/Linux x86 de 32 bits com kernel
  de 64 bits, como o i386 do Debian 13, que não tem mais kernel de 32 bits:
  ali o `gpt` gera x86-64, inclusive sob a personalidade `linux32`. Com
  kernel de 32 bits, onde a 1.4.0 gerava ELF32, `gpt -o` passa a recusar.
- Com `--enable-install-devel`, a `libgportugol` instalada passa a ser
  `libgportugol.so.1`, porque a maior parte dos símbolos da `.so.0` sumiu com
  o ANTLR 2, e liga o runtime do ANTLR 4 e o PCRE2 que ela chama. A suíte de
  regressão também passa nessa configuração, em que `src/gpt` é o wrapper do
  libtool.
- O CI compila e testa o `gpt` também num runner arm64 e num container
  i386, e o job x86-64 roda a suíte para arm64 sob `qemu-aarch64`, com as
  variáveis `GPT_TEST_ARCH` e `GPT_TEST_RUNNER` do `test/run_test.sh`. Os jobs
  e artefatos levam o nome da arquitetura, e os nomes dos arquivos da release
  não mudam. Os jobs Linux instalam o `python3`, de que os testes do
  depurador da suíte precisam.
- Literais de caractere e strings só aceitam as sequências de escape `\n`,
  `\r`, `\t`, `\\`, `\"`, `\'` e `\?`, as que dão o mesmo caractere nos três
  modos. Qualquer outra, e uma barra invertida no fim da linha, agora é erro
  de compilação (`Sequência de escape inválida: "\a".`). Antes cada modo
  tratava esses escapes de um jeito: `"[\a\x41\0z]"` saía como `[ax410z]` no
  interpretador, como `[ax41` no executável e como `[`, BEL e `A` na tradução
  para C, e uma barra invertida no fim da linha virava quebra de linha no
  interpretador, sumia no C e fazia o NASM falhar.
- Uma matriz passada inteira como argumento precisa ter o mesmo tipo de
  elemento do parâmetro, e não só as mesmas dimensões, e o erro é o de
  dimensão errada (`Argumento 1 da função "f" deve ser do tipo "real[3]".`).
  Antes uma matriz de inteiros, reais, caracteres ou lógicos servia para um
  parâmetro matriz de qualquer um desses tipos: o interpretador convertia
  cada elemento, o executável nativo caía com falha de segmentação quando um
  dos lados era real, e o C gerado não compilava com o GCC 14.
- Uma chamada de função sem tipo de retorno é recusada como operando de
  `não`, nos dois lados de uma comparação ou de `e`/`ou`, como condição de
  `se`, `enquanto` e `repita` e depois de `retorne` numa função sem tipo de
  retorno (`Condição do "se" não retorna resultado.`). Antes, nos três
  primeiros casos, o C gerado não compilava, o interpretador lia o valor
  ausente como falso, então `não nada()` dava `verdadeiro`, e o executável
  nativo usava o que a chamada deixava no registrador de retorno.
- Uma matriz sem subscritos é recusada como condição de `se`, `enquanto` e
  `repita`, como operando de `não`, numa comparação ou num `e`/`ou` entre
  duas matrizes e como valor de `retorne` no bloco principal
  (`Faltando indicar subscritos da matriz/conjunto "m".`, o erro de tipo do
  operador ou o do valor de retorno do bloco principal). Antes `se m então`
  seguia o `senão` no interpretador e testava o endereço dos elementos no
  executável e no C, `m = n` comparava endereços, e `retorne m` não
  compilava em C com o GCC 14.
- Um programa com mais de 100 níveis de comandos e expressões aninhados é
  recusado com `Aninhamento muito profundo: mais de 100 níveis de comandos e
  expressões.` A 1.4.0 compilava alguns milhares de níveis e caía com falha
  de segmentação acima disso, e uma chamada recursiva dentro de milhares de
  `se` aninhados derrubava o interpretador em vez de parar com a mensagem de
  pilha esgotada. O manual explica como os níveis se somam.

### Corrigido

- A função `arredonda` da biblioteca padrão não tinha `retorne` e devolvia
  um valor indefinido, que mudava com o modo e com as chamadas anteriores.
  Agora `arredonda(2.6)` dá `3.00` nos três modos.
- No executável nativo, os reais tinham precisão simples, então
  `12345678.9` saía como `12345679.00` e `123456789012.0` como `28.74`. Ele
  também imprimia `-0.5` como `0.50` e `99.999` como `99.010`, escrevia `t`
  no lugar da tabulação de `\t` e mostrava um literal vazio devolvido por
  função como texto vazio em vez de `(nulo)`. Agora a saída é igual à da
  tradução para C.
- `imprima` mostrava o sinal de um NaN, que depende do processador: o NaN de
  `inf - inf` saía como `-nan` no x86 e como `nan` no ARM. Agora sai como
  `nan` nos três modos e em qualquer processador, e o manual explica como
  `inf` e `nan` são impressos.
- No executável nativo, `não` tinha o tipo do operando, então
  `imprima(não "abc")` mostrava `(nulo)`, `imprima(não "")` caía com falha
  de segmentação e `imprima(não x)`, com `x` inteiro, mostrava 1 ou 0 em
  vez de `verdadeiro` ou `falso`. No interpretador, `não "abc"` dava
  `verdadeiro`, e no C, `falso`. Agora os três modos dão `falso` para um
  literal não vazio e `verdadeiro` para o vazio.
- O interpretador só convertia um valor na atribuição a uma variável, e
  argumentos, valores de retorno, subscritos e elementos de matriz
  guardavam o texto de `leia()` ou de um real. Um parâmetro inteiro podia
  valer `0.5`: dividir por ele matava o interpretador com SIGFPE em vez de
  dar a mensagem de divisão por zero, e `x * 1.5` usava 0.5. Além disso,
  `v[leia()]` com a entrada `01` não gravava em `v[1]`, e um caractere lido
  com `leia()` saía como byte NUL quando passado como argumento, devolvido
  ou guardado numa matriz. Agora esses valores são convertidos como no
  executável nativo e no C: um real é truncado, e o texto de `leia()` é lido
  no tipo de destino.
- Uma função com tipo de retorno que terminava sem executar `retorne`
  devolvia um valor indefinido. O executável nativo devolvia o que estava no
  registrador, e uma função literal podia imprimir a memória num endereço
  vindo do chamador; o C gerado caía do fim da função; e o interpretador
  devolvia o valor do último `retorne` executado em qualquer ponto do
  programa. Agora os três modos devolvem o valor zero do tipo: 0, 0.00,
  `falso`, o caractere de código 0 ou o literal nulo, que `imprima` mostra
  como `(nulo)`.
- `imprima(..., leia(), ...)` e um `leia();` sozinho, onde nada indica o
  tipo a ler, leem a linha como literal nos três modos, como o interpretador
  já fazia. Antes a tradução para C lia um inteiro e o passava a um `%s`
  (falha de segmentação com a entrada `12345`), e o executável nativo não
  compilava (`Erro interno: tipo nao suportado`).
- No executável x86, um parâmetro matriz `x` e um parâmetro `p__x` da
  mesma função tinham o mesmo nome no assembly, e um lia o valor do outro
  como endereço. Uma variável global chamada `_str_0` impedia a montagem
  nos dois backends nativos. Os nomes gerados agora levam um `$`, que um
  identificador não pode conter.
- O executável x86 gravava no arquivo os zeros das variáveis globais: uma
  `matriz[20000000] de inteiros` global gerava um arquivo de 80 MB na 1.4.0
  (160 MB no x86-64). Agora as globais ficam na seção `.bss`, e o mesmo
  programa gera um ELF de 4 KB e um PE32+ de 12 KB.
- O executável nativo guardava os literais numa área fixa de 1 MiB, e nada
  do que ela guarda é liberado, então um programa que lia 200000 linhas de
  40 caracteres terminava em `Não foi possível alocar memória.`, enquanto o
  interpretador e o C iam até o fim. Agora a área cresce em blocos pedidos
  ao sistema (`mmap` no GNU/Linux, `VirtualAlloc` no Windows), e a mensagem
  só aparece quando o sistema recusa memória. Como nada é liberado, um laço
  sem fim que gera literais cresce até esse ponto.
- `gpt -s x.gpt x.gpt`, e o mesmo com `-t` ou `-o`, substituía o fonte pelo
  código gerado e saía com 0. O mesmo acontecia por um link, por outra
  grafia do caminho, por um arquivo de `GPT_INCLUDE` ou pelo nome padrão de
  `-o` (`gpt x.gpt x`). Agora o `gpt` compara a saída com cada entrada pela
  identidade do arquivo, recusa com `o arquivo de saída é um dos arquivos de
  entrada` e código 1, e não toca em nenhum arquivo.
- Um SIGINT, SIGTERM ou SIGHUP durante a montagem deixava o assembly
  temporário (e, no arm64, o objeto) no `TMPDIR`, e um SIGTERM mandado só
  ao `gpt` deixava o montador rodando. Agora, fora do Windows, o `gpt -o`
  encerra a ferramenta com o mesmo sinal, apaga os temporários e termina
  pelo sinal recebido. Um sinal ignorado na partida, como o SIGHUP sob
  `nohup`, continua ignorado.
- Os pacotes binários da release não traziam o texto da GPL-2.0, que só
  vinha no tarball do código-fonte. Agora o `COPYING` vai em
  `share/doc/gpt` nos tarballs Linux e em `share/licenses/gpt` no zip do
  Windows.

### Segurança

- No Windows, sem um `nasm.exe` ao lado do `gpt.exe`, o `gpt` passava só o
  nome `nasm` ao `CreateProcess`, que procura no diretório atual antes do
  `PATH`, então um `nasm.exe` deixado no diretório do projeto rodava a cada
  `gpt -o`. Com `-a arm64`, o `as` e o `ld` sempre iam assim. Agora o `gpt`
  resolve o caminho de cada ferramenta: primeiro o diretório do próprio
  `gpt`, depois cada entrada do `PATH`, como o manual descreve. No
  GNU/Linux, o `as` e o `ld` do arm64 também são procurados primeiro ao lado
  do `gpt`. Uma ferramenta que não existe dá `não foi possível encontrar o
  nasm.` (ou o nome da ferramenta do binutils).
- O `ld` lê um argumento começando com `@` como arquivo de opções, mesmo no
  valor de `-o`: `gpt -a arm64 @aula.gpt` fazia o `ld` ler opções do
  arquivo `aula`, e um `-plugin` ali executava o código de uma biblioteca na
  compilação. O `nasm` também lê como opções uma entrada começando com `@`
  ou `-`, o que um `TMPDIR` relativo podia provocar. Agora o nome de saída e
  os arquivos temporários começando com `@`, `-`, `=` ou `$SYSROOT` chegam
  às ferramentas como `./nome`.
- As dimensões de matriz e o produto delas não tinham limite. Em
  `matriz[65536][65536]` o produto estourava e a matriz ficava sem espaço,
  então um índice lido da entrada escrevia sobre a global seguinte, e uma
  matriz local cujo tamanho em bytes dava a volta (no x86-64,
  `matriz[536870912]`) sobrepunha seus elementos ao endereço de retorno.
  Agora os três modos recusam dimensões negativas e matrizes com mais de
  67108864 elementos (2^26), os backends nativos recusam mais de 1 GiB de
  globais ou de variáveis de uma função, e um quadro de pilha de uma página
  ou mais é tocado página a página, para que bata na página de guarda em vez
  de saltá-la. O interpretador passa a recusar as matrizes maiores que
  aceitava.
- No executável x86, cada argumento de `imprima` que chamava uma função sem
  tipo de retorno deixava a pilha desalinhada em uma posição, e um
  empilhamento seguinte podia trocar o endereço de retorno da função por um
  valor lido da entrada. Agora esse argumento é recusado nos três modos
  (`Argumento 2 da função "imprima" não retorna resultado.`). Antes o
  interpretador imprimia uma linha vazia.
- Uma barra invertida seguida de quebra de linha, ou um byte 0x1A cru,
  dentro de uma string fechava a linha `db` do assembly x86, e o resto
  do literal virava código NASM: um `%include` ali fazia o `nasm` abrir e
  ler um arquivo local antes de o `gpt -o` falhar. Agora as aspas e os
  bytes de controle vão para o NASM como números, e o executável imprime
  esses literais como o interpretador.
- Na tradução para C, um inteiro com zeros à esquerda virava octal: `010`
  valia 8, e `matriz[0100]` era declarada com 64 elementos enquanto a
  verificação de índices usava 100, então um índice de 64 a 99 escrevia
  fora da matriz. Agora os zeros à esquerda são descartados, como no
  interpretador e no executável nativo, e literais como `08` e `09`, que o
  `gcc` recusava, compilam.
- A tradução para C copiava o texto dos literais, e o `gcc` com `-std=c11`,
  `-std=c99` ou `-trigraphs` troca os trigraphs antes de ler os escapes:
  `"??/"` virava uma barra invertida que escapava a aspa final, e
  `imprima("??/", "+0, (exit(42),0)); //")` executava `exit(42)`. Agora todo
  `?` fora de um escape sai como `\?`.
- Depois de montar, o `gpt -o` dava permissão de execução à saída com
  `chmod` pelo nome: como root, `gpt -o /dev/null` mudava o `/dev/null` de
  666 para 777, e como outro usuário o comando falhava. Agora só um arquivo
  regular recebe a permissão, e `gpt -o /dev/null` funciona. Um link
  simbólico na saída continua sendo seguido.
- O depurador guardava cada breakpoint pedido pelo host, repetidos
  inclusive, e um pedido de remoção para um arquivo desconhecido criava uma
  entrada, então o host podia fazer o `gpt` crescer sem limite e deixar o
  programa mais lento a cada comando. Agora um host que define mais de 1000
  breakpoints ou manda um nome de arquivo com mais de 4096 bytes é
  desconectado como um que manda uma mensagem malformada.
- O texto do fonte ia cru para as mensagens de erro, e um literal com
  `ESC [2J` depois de um `;` faltando limpava o terminal de quem compilava.
  Agora os caracteres de controle C0 e C1 e o DEL aparecem como `\xNN` nos
  diagnósticos, nos nomes de arquivo das mensagens e no eco dos comandos
  desconhecidos do depurador. Letras acentuadas em UTF-8 não mudam, mas um
  nome de arquivo em Latin-1 no GNU/Linux passa a mostrar os acentos como
  `\xNN`.
- Os tarballs Linux da release guardavam o uid e o gid 1001 (`runner`) em
  cada entrada, e o zip também, para o `unzip -X`, então extrair como root
  entregava os arquivos à conta local de uid 1001. Agora o tar grava dono e
  grupo 0 em cada entrada, e o zip não grava mais uid nem gid.
- O `test/run_test.sh` criava o diretório de trabalho com
  `mkdir -p ${TMPDIR:-/tmp}/gpt_test_$$`, que aceita um diretório ou link
  já existente, e outro usuário podia fazer a suíte gravar e executar seus
  binários num diretório dele. Agora o diretório vem do `mktemp -d`.

## [1.4.0] - 2026-10-26

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
  404. Ela é a versão padrão, aberta pela raiz do site, e o manual da última
  release continua em `stable`.

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
