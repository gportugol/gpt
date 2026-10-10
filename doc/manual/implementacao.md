(cap-implementacao)=

# Implementação da linguagem G-Portugol

## Introdução

Ao definir uma linguagem de programação voltada para o ensino de lógica e
algoritmos, vários aspectos devem ser considerados. Ao contrário de linguagens
de produção, não há preocupações como o poder expressivo da linguagem,
dicionário em inglês, acesso a recursos de sistema, etc. A preocupação central
está em oferecer uma ferramenta que:

- reflita os processos computacionais, exigindo o mínimo de conhecimento e
  experiência do estudante;
- evidencie os processos relacionados com o desenvolvimento de softwares;
- estimule a abstração e raciocínio lógico do estudante.

## A linguagem

O ponto fundamental que guia as diretrizes da linguagem G-Portugol é seu
propósito educacional: ela deve expressar processos computacionais de forma que
um leigo os compreenda sem enfatizar a si mesma. Isso é, a linguagem em si deve
chamar o mínimo de atenção possível, fazendo com que a compreensão dos processos
computacionais seja tão natural quanto ler sua descrição informal, ou
não-estruturada.

Esse objetivo encontra restrições, quando se leva em consideração a natureza das
linguagens artificiais e o uso e forma popular atual da linguagem portugol,
principalmente em literaturas. É de interesse que G-Portugol ofereça
compatibilidade com essas formas, o que pode gerar conflitos quanto a decisões
de design e restringir suas características. Portanto, embora as diretrizes
marquem a base do design, muitas vezes, elas devem ser sacrificadas ou ajustadas
para incorporar formas populares.

É interessante ressaltar que criar uma linguagem totalmente nova, que utiliza
outros paradigmas como, por exemplo, orientação a objeto, possa ser interessante
e, talvez, mais eficiente como ferramenta de ensino, mas G-Portugol está, no
momento, comprometida com a compatibilidade.

A seguir, alguns tópicos serão comentados quanto às diretrizes por trás das
formas léxicas e gramaticais da linguagem G-Portugol.

### Diretrizes para o design da linguagem

#### A linguagem deve ser totalmente em português e deve respeitar acentuações

Linguagens de programação, em geral, não se utilizam de caracteres especiais
(ex. caracteres acentuados, cedilhas, e outros que não pertencem ao alfabeto
inglês) para definição de seu dicionário, visto que são baseadas na língua
inglesa. Portanto, letras acentuadas não são consideradas.

A decisão de incorporar palavras que respeitam a língua portuguesa é importante,
visto que modificar a linguagem de forma a se afastar de sua língua natural (o
português) pode evidenciar exceções as quais forçariam os usuários (estudantes e
professores) a se ater mais com o estudo da linguagem do que com o estudo da
disciplina em questão. Isso é, a ausência de acentos, por exemplo, obriga o
usuário a aprender seus termos excepcionais. Além do mais, o uso de termos como
"nao" chama a atenção constante do usuário para o fato de a palavra não estar
acentuada, o que costuma desviar atenção do estudo.

Consequentemente, o uso de acentos permite que a linguagem seja o mais próximo
do português quanto for possível, apoiando a regra de não chamar atenção para
si. Ademais, o código fica mais legível e permite uma leitura mais agradável.

Além dos acentos, é exigido que as palavras-chave usadas sejam completas ou por
extenso, sem permitir abreviações (ex. "proc", "func", "char", "int", ...), o
que dificulta a leitura de programas por um leigo.

Há também decisões quanto à forma verbal de comandos e funções. Em geral, na
literatura, os verbos nos algoritmos são expressos no imperativo, visto que a
linguagem é caracterizada como imperativa. Por outro lado, vale notar que, mesmo
em linguagens imperativas, é comum ver programas que utilizam termos em
português usando verbos no infinitivo.

Mesmo com essa perspectiva em vista, G-Portugol se utiliza de verbos no
imperativo para seus termos, de forma a se aproximar das formas utilizadas nas
literaturas sobre algoritmos.

> *"Programas devem ser escritos para pessoas lerem e, apenas incidentalmente,
> para máquinas executarem."*
>
> Abelson e Sussman

#### A linguagem deve ser simples, uniforme e coerente

Essa diretriz diz respeito a evitar exceções entre as formas gramaticais e
comportamentos semânticos na medida do possível. Isso significa não só que as
estruturas e comandos devem ser simples, claros e óbvios, como também, sua forma
e comportamento devem ter um princípio em comum, evidenciando um formato
uniforme. Naturalmente, a linguagem portugol já detém uma parcela relevante de
características que se acomodam nessas diretrizes, no entanto, alguns detalhes
ainda merecem atenção, como a representação de blocos estruturais.

A simplicidade implica em evitar construções e recursos que desviam a atenção do
propósito original para detalhes da linguagem. Isso é, apenas recursos
essenciais devem fazer parte de seu núcleo. Portanto, em um primeiro momento,
não é interessante equipar a linguagem com recursos como alocação dinâmica de
memória, controle de threads, etc.

#### Deve ser configurável naquilo que diz respeito a diferentes abordagens de ensino e estilo do professor

Essa diretriz diz respeito a permitir que a linguagem mude em certos aspectos
(ou disponibilizar meios simples para realizar essas mudanças) de acordo com o
gosto do professor, ou como modo de adaptar a forma de portugol de uma dada
literatura. Dialetos e formas da linguagem portugol variam de literatura para
literatura, e pode ser interessante permitir que a linguagem se adapte ou
oferecer diferentes formas da linguagem.

Entre os aspectos que podem ser adaptados com facilidade se encontram os termos
léxicos (palavras-chave, operadores, etc), e algumas formas de expressar
estruturas, como algumas declarações e enunciados.

Por exemplo, pode ser interessante que, para se declarar uma variável, não se
use um bloco iniciado por `variáveis` e terminado por `fim-variáveis`, mas
um bloco parecido com a declaração de variáveis em Pascal, ou até em C (que
sequer exige delimitação de bloco de variáveis).

Isso pode ser interessante quando se quer, por exemplo, ensinar uma linguagem
específica como Pascal, fazendo com que G-Portugol possa usar estruturas e
operadores semelhantes a esta linguagem, de forma a oferecer uma transição mais
direta e que aproveita melhor os conhecimentos do estudante.

Atualmente, o programa GPT não implementa mecanismos para esse tipo de mudança,
sendo necessário modificar seu código fonte e recompilá-lo.

## Formato Estrutural

Um programa escrito em G-Portugol tem o seguinte formato:

- declaração do algoritmo
- declaração de variáveis globais
- bloco principal
- declaração de funções

A seguir, alguns pontos serão discutidos a respeito do formato da linguagem.

### Declaração do algoritmo

A declaração do algoritmo não influencia o programa ou sua execução, visto que é
apenas uma seção informativa no código que, embora não seja um comentário, tem o
mesmo efeito prático. A adoção dessa declaração pode ser discutida, e foi
escolhida por ser bastante utilizada em literaturas.

### Declaração de variáveis globais

Como visto no {numref}`Capítulo %s <cap-programando>`, essa declaração é
opcional, e seu formato pode ser visto no exemplo a seguir:

```portugol
variáveis
  x : inteiro;
fim-variáveis
```

Esse formato difere de linguagens como Pascal e C. Em Pascal, o bloco não tem um
delimitador final (como `fim-variáveis`) e em C, não existe qualquer
delimitador.

Delimitar o bloco permite maior consistência com outras formas gramaticais como
os blocos de comando e estruturas de controle, e torna o código mais claro e
explícito, embora adicione construções redundantes.

Os tipos primitivos englobam os tipos mais básicos das linguagens populares. No
momento, agregados heterogêneos como, por exemplo, estruturas/registros
("struct" em C) não são suportados. Para uma lista dos tipos suportados, veja a
{numref}`tb-tipos-primitivos`.

### Bloco Principal

As linguagens de programação devem, de alguma forma, oferecer um "entry point"
(ponto de entrada), de onde se inicia a execução do programa. O ponto de entrada
pode ser uma função ou um bloco anônimo. Na literatura, em geral, o bloco
principal é delimitado pelos termos `início` e `fim` e G-Portugol segue essa
convenção. Essa decisão mantém um nível satisfatório de coerência com o bloco de
variáveis globais e estruturas de controle, embora não sejam intimamente
relacionados.

Vale ressaltar que não se faz imposição sobre a indentação do código. Esse
recurso pode ser vantajoso no ensino, promovendo a clareza de código, portanto,
sua implementação pode ser discutida para versões futuras.

Embora a sintaxe seja bem diferente, o bloco principal pode ser visto como uma
função (como *main* na linguagem C). Há, portanto, a possibilidade de retornar
um valor inteiro através do comando `retorne`.

```{code-block} portugol
:caption: Bloco principal com comando retorne.
:name: programa-bloco-principal

algoritmo bloco_principal;

início
  imprima("Retornando valor do bloco principal");
  retorne 42;
  imprima("Essa linha não será impressa");
fim
```

### Estruturas de Controle

Estruturas de controle são formadas por um cabeçalho seguido por um bloco de
comandos e terminados por um delimitador final. Embora os delimitadores do bloco
não sejam `início` e `fim`, há um grau de semelhança mantido: o delimitador
inicial é omitido (uma vez que o cabeçalho é entendido como delimitador inicial)
e o delimitador final é o termo `fim-` seguido do nome da estrutura.

No cabeçalho das estruturas como "enquanto" e "se", as expressões não precisam
ser delimitadas por parênteses.

(sec-expr)=

### Expressões

Expressões são operações que sintetizam, ao final, um valor. Em geral constituem
valores ou operações aritméticas com um ou mais termos que podem ser variáveis,
constantes ou chamadas a funções e são usadas em atribuições, estruturas de
controle e subscritos de matrizes e vetores. Enunciados (como atribuições) não
podem ser avaliados como expressões.

Constantes são valores *inline*, e cada tipo de dado tem uma forma de ser
representada.

- **Inteiros**: podem ser representados em base decimal, hexadecimal, octal e
  binária. Representações decimais são formadas, opcionalmente, por um sinal
  (`+` ou `-`) seguido de um ou mais algarismos (ex. `120`, `+5` e `-2`).
  Representações hexadecimais são representadas com o prefixo `0x` ou `0X`,
  seguido de algarismos entre `0` e `9` e letras entre `a` e `f` ou `A` e `F`
  (ex. `0xF1A5`). Representações octais são representadas com o prefixo `0c` ou
  `0C`, seguido de algarismos entre `0` e `7` (ex. `0c61`). Finalmente,
  representações binárias são formadas pelo prefixo `0b` ou `0B`, seguido de
  algarismos `0` e `1` (ex. `0b101`).
- **Reais**: são representados por, opcionalmente, um sinal (`+` ou `-`),
  seguido de algarismos separados por um `.` como `-1.2345`.
- **Caracteres**: são representados por um único símbolo entre aspas simples.
  Alguns caracteres especiais são representados com escape (`\`) seguido de
  símbolo identificador. Esses caracteres são o LF (`\n`), CR (`\r`) e barra
  invertida (`\\`). A ausência de símbolos entre as aspas simples indica um
  caractere "nulo". Internamente, caracteres são representados como números
  inteiros, o que permite sua compatibilidade numérica.
- **Literais**: são representados por um conjunto de caracteres entre aspas
  duplas. Eles podem conter caracteres especiais como `\n` e devem ser definidos
  em apenas uma linha de código. Valores literais são os únicos que não têm uma
  representação numérica, impedindo sua participação em expressões com
  operadores aritméticos (soma, divisão, etc). Comparações de igualdade para
  valores literais são feitas caractere por caractere em *case sensitive*.
  Portanto, a expressão `"portugol" = "Portugol"` é avaliada como falsa. Já
  comparações de grandeza são feitas calculando o número de caracteres que
  compõem os valores literais. Então, a expressão `"maria" > "josé"` é avaliada
  como verdadeira.
- **Lógicos**: são representados pelas palavras `verdadeiro` e `falso`.
  Numericamente, qualquer valor diferente de "0" representa o valor verdadeiro e
  "0" representa o valor falso.

A precedência de operadores é mostrada na {numref}`tb-precedencia-op` (da menor
precedência para a maior) e pode ser explicitamente modificada com o uso de
parênteses.

```{list-table} Precedência de operadores
:name: tb-precedencia-op
:header-rows: 1

* - Operador
  - Nome
* - `ou`
  - OR lógico
* - `e`
  - AND lógico
* - `|`
  - OR binário
* - `^`
  - XOR binário
* - `&`
  - AND binário
* - `=`, `<>`
  - operadores igual e diferente
* - `>`, `>=`, `<`, `<=`
  - operadores relacionais maior, maior ou igual, menor, menor ou igual
* - `+`, `-`
  - operadores aritméticos soma e subtração
* - `*`, `/`, `%`
  - operadores aritméticos multiplicação, divisão, módulo
* - `+`, `-`, `~`, `não`, `()`
  - operadores unários positivo, negativo, NOT binário, NOT lógico, parênteses
```

Em G-Portugol, não há coerção ou *casting* de tipos explicitamente. Todos os
tipos numéricos (inteiro, real, lógico e caractere) são compatíveis entre si. É
importante ressaltar que expressões envolvendo igualdade ou diferença de valores
reais não são apropriadas dada a forma como esses valores são representados
internamente. Tais comparações podem ter resultados imprevisíveis, e até
diferentes, ao serem executados nos modos compilado, interpretado ou traduzido.

Existem dois casos em que ocorre uma coerção implícita. O primeiro caso ocorre
durante a avaliação de uma expressão que tem operandos de tipos diferentes (mas
compatíveis entre si), onde não há perda de dados e um dos termos é promovido
para o tipo do seu termo complementar. Por exemplo, na expressão `2 + 1.5`, o
termo `2` é promovido para o tipo real, tendo o valor `2.0` antes que a soma
seja processada. Da mesma forma, sendo `x` uma variável de tipo real, a
expressão `x := 5 / 2` atribuirá a `x` o valor `2.00`. Esse comportamento não
muito óbvio é similar ao da linguagem C, onde `5 / 2` é avaliado como uma
divisão de inteiros, onde apenas o valor inteiro final é relevante. Para obter o
resultado real, a expressão deve ser `5.0 / 2`, para informar ao compilador que
a divisão usará valores reais, produzindo a promoção de tipos mencionada
anteriormente.

O segundo caso em que ocorre uma coerção implícita é discutido na seção
{ref}`subsec-atribuicoes`.

(subsec-atribuicoes)=

### Atribuições

Atribuições permitem a manipulação de valores na memória. Em G-Portugol, como já
foi visto, é usado o operador `:=`, onde o termo à esquerda ou "*lvalue*" deve
ser uma variável primitiva ou índice de uma matriz/vetor, e o termo à direita,
uma expressão que, quando avaliada, tem seu tipo compatível com o *lvalue*.

Pode haver coerção de tipos durante a atribuição, quando o resultado da
expressão é de um tipo diferente (mas compatível) do tipo de *lvalue*. É
possível que dados sejam comprometidos, por exemplo, tendo uma expressão
avaliada como real sendo atribuída a uma variável de tipo inteiro (o valor será
truncado).

Matrizes não são aceitas como *lvalue*, como pode ser visto no
{numref}`bad-matrix`.

```{code-block} portugol
:caption: Uso incorreto de matrizes.
:name: bad-matrix

algoritmo atribuicao_de_matrizes;

variáveis
  m1 : matriz[2] de inteiros;
  m2 : matriz[2] de inteiros;
fim-variáveis

início
  m1 := m2;    //erro
  imprima(m1); //erro
fim
```

## Funções

Funções são os subprogramas de G-Portugol. São definidas após o bloco principal
e podem receber argumentos e retornar valores. Em tempo de execução, as funções
criam um novo escopo sobreposto ao escopo principal. Isso permite recursos como
recursão e possibilita que variáveis locais (no escopo da função) tenham o mesmo
nome que variáveis globais, onde essas últimas ficam "escondidas", isso é,
incapazes de serem acessadas enquanto o escopo durar. Ressalta-se que não há
suporte para funções aninhadas, isso é, funções declaradas dentro de funções.

O retorno de dados é feito por meio da instrução `retorne` e o valor de retorno
(se houver) deve ser compatível com o tipo da função. Esse tipo não pode ser um
tipo agregado como matrizes e vetores.

Tanto a passagem de argumentos quanto o retorno são feitos *por valor*, isso é,
a cópia do valor é feita, ao invés da cópia de endereço ou passagem *por
referência*.

As variáveis locais de uma função são formadas por seus parâmetros e pelas
variáveis declaradas em seu escopo. A declaração de variáveis locais é feita
entre o cabeçalho da função e a palavra-chave `início`, portanto, não é uniforme
em relação à declaração global, onde se usam as palavras-chave `variáveis` e
`fim-variáveis`.

A declaração dos parâmetros da função também não segue estritamente o formato de
declaração de variáveis. O {numref}`exemplo-funcao` ilustra a declaração de uma
função.

```{code-block} portugol
:caption: Exemplo de usos de funções.
:name: exemplo-funcao

algoritmo exemplo_funcao;

início
  imprima(soma(2,2));
fim

função soma(x: inteiro, y: inteiro) : inteiro
  res : inteiro;
início
  res := x + y;
  retorne res;
fim
```

## Funções internas

G-Portugol oferece duas funções internas para manipulação básica de entrada e
saída. Ambas as funções têm comportamentos excepcionais quando comparadas com as
funções de usuário. Essas funções são discutidas nas seções a seguir.

### A função "imprima"

A função `imprima` tem como objetivo imprimir texto no dispositivo de saída. Ela
recebe um número variável de argumentos (pelo menos um), onde cada argumento
deve ser um valor primitivo, e os imprime em sequência. Ao final, é impresso um
caractere de nova linha (LF) e a função retorna.

Essa função se comporta de forma excepcional visto que não é possível declarar
funções em G-Portugol que recebem um número variável de argumentos.

### A função "leia"

A função `leia` espera por uma entrada do teclado seguida do caractere LF (em
geral, associado à tecla "Enter") e retorna o valor lido. Esse valor é
convertido implicitamente para o tipo do *lvalue*. A {numref}`tb-leia-casts`
apresenta conversões implícitas processadas pela função `leia` para o enunciado
de exemplo `x := leia()`.

Nota: números reais são arredondados, se necessário, e exibidos no formato .xx
(com duas casas decimais). Portanto, o número `250.0` ou `250` seria impresso
como `250.00` e `1.449` seria impresso como `1.45`. Se o parâmetro for uma
variável, seu valor não será modificado.

```{table} Conversões implícitas da função "leia"
:name: tb-leia-casts

| Tipo do lvalue `x` | Texto lido    | Valor final de `x` |
| ------------------ | ------------- | ------------------ |
| inteiro            | `123`         | `123`              |
| inteiro            | `123 456`     | `123`              |
| inteiro            | `abc`         | `0`                |
| inteiro            | `123s`        | `123`              |
| lógico             | `falso`       | `falso`            |
| lógico             | `0`           | `falso`            |
| lógico             | `falso 12wtc` | `verdadeiro`       |
| lógico             | `0 umdois3`   | `verdadeiro`       |
```

Essa função se comporta de forma excepcional visto que seu tipo de retorno não é
absoluto (*overloaded*, depende do tipo de *lvalue*) e, no momento, é restrita a
expressões isentas de operadores. Isso é, não é permitido aplicá-la como um
termo em uma expressão com múltiplos operandos (ex. `x := y + leia()`), embora
seja possível utilizá-la em subscritos de vetores e matrizes, assim como em
expressões de estruturas de controle.

Em princípio, pode-se levar em conta duas formas de implementar uma função de
leitura em alto nível. A primeira, e mais simples, é o uso de funções
sobrecarregadas, que não retornam valor e recebem um parâmetro de um dado tipo
(inteiro, real, etc), que é alimentado com o valor lido. Essa forma exige que a
passagem seja feita por referência. A outra forma é a implementada em
G-Portugol, onde a função não recebe parâmetros e retorna o valor lido.

As duas formas exigem comportamentos excepcionais, visto que G-Portugol não tem
suporte para sobrecarga de funções (por parâmetro ou retorno) ou passagem de
parâmetros por referência.
