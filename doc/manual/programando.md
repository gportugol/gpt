(cap-programando)=

# Programando em G-Portugol

A proposta desse capítulo é mostrar os elementos da linguagem G-Portugol usando
programas como exemplos. Não é um capítulo sobre como programar, nem tem como
intuito ensinar algoritmos. Portanto, é esperado que o leitor seja familiarizado
com programação.

## Olá Mundo

Mantendo a tradição, vamos criar nosso primeiro programa. O propósito dele será
exibir na tela o texto "Olá mundo". Crie um arquivo chamado `olamundo.gpt` com o
seguinte conteúdo:

```{code-block} portugol
:caption: “Olá Mundo” em G-Portugol.
:name: prog-olamundo

/*
  Nosso primeiro programa
*/

algoritmo olamundo;

início
  imprima("Olá mundo!");
fim
```

Após salvar o arquivo, digite o seguinte na linha de comando:

- No GNU/Linux:

  ```shell
  gpt -o olamundo olamundo.gpt
  ```

- No Windows:

  ```shell
  gpt -o olamundo.exe olamundo.gpt
  ```

Esse comando compila o algoritmo e salva o arquivo binário resultante como
`olamundo` (ou `olamundo.exe`) no diretório atual. Se a opção `-o <arquivo>` não
for informada, o GPT criará o executável usando o nome do algoritmo. Após
executar o programa criado, é exibido o texto "Olá mundo!" na tela.

Como já deve ter assumido, comentários ao estilo C (`/* */`) e C++ (`//`) são
permitidos e o que estiver entre aspas duplas (`"`) é tratado como uma string ou
constante literal. Escapes são permitidos como EOL (`\n`), tabulação (`\t`)
dentre outros. Não é permitida a concatenação em múltiplas linhas como na
linguagem C:

```portugol
imprima("Isso é uma "
        "Concatenação"); //erro!
```

Outro detalhe é a pontuação. Da mesma forma que em C, o `;` é utilizado como
finalizador de enunciados. Se for necessário utilizar uma aspa dupla em um
literal, deve-se utilizar o escape:

```portugol
imprima("Isso é literal com aspas duplas (\") ");
```

## Variáveis

A declaração de variáveis deve ser feita dentro de um bloco específico, que deve
aparecer logo após a declaração do algoritmo. O bloco é iniciado pela
palavra-chave `variáveis` (sim, com acento) e termina com a palavra-chave
`fim-variáveis`. Pelo menos uma variável deve ser declarada dentro do bloco
(embora o bloco em si seja opcional) e apenas um bloco em escopo global deve ser
declarado. Eis um exemplo para estudo:

```{code-block} portugol
:caption: Declaração de variáveis globais.
:name: programa-inutil

algoritmo teste_variaveis;

variáveis
  x    : inteiro;
  nome : literal;
fim-variáveis

início
fim
```

### Variáveis primitivas

Variáveis primitivas são declaradas seguindo o seguinte modelo:

```text
<identificador> [, identificador]* : <tipo>;
```

Isso é, um ou mais identificadores separados por vírgula, seguido de um `:`,
seguido de um tipo, seguido, finalmente, de `;`. Como pode-se notar, é parecido
com Pascal. Assim que um programa é executado, todas as variáveis, primitivas ou
vetores/matrizes, declaradas são iniciadas com um valor nulo ou "0"
automaticamente. Vale ressaltar que constantes (const, final, etc) não são
suportadas. Os tipos primitivos suportados encontram-se na
{numref}`tb-tipos-primitivos`.

A fim de explorar melhor os aspectos da declaração, seguem-se alguns comentários
a respeito do último exemplo ({numref}`programa-inutil`):

- Observe que o nome do algoritmo (`teste_variaveis`) não tem acento. Se
  declarar o algoritmo como `teste_variáveis` e tentar compilar o código, o
  seguinte erro será exibido: **Linha: 1 - "teste_variáveis" não pode ter
  caracteres especiais.** Portanto, identificadores (nomes de variáveis, funções
  e do algoritmo) não podem ter acentos ou caracteres especiais como `$`, `#`,
  etc. A definição de um identificador em G-Portugol é equivalente à das
  linguagens populares: uma letra (a-z ou A-Z) ou underline (`_`) seguido de
  qualquer número de letras, números ou underlines. Cedilhas, portanto, também
  não formam identificadores válidos.
- O segundo ponto é a palavra-chave `variáveis`: ela tem acento, e isso é
  permitido e obrigatório.
- O terceiro é a definição do bloco e sua (falta de) semelhança com o Pascal.
  Todos os blocos em G-Portugol tentam seguir o formato "nome/fim-nome", em
  favor da uniformidade e em detrimento de exceções linguísticas que confundem
  os estudantes.
- E, finalmente, o quarto ponto é a ausência de código entre `início` e `fim`.
  O programa não é obrigado a ter enunciados ou comandos.

Para maiores detalhes, veja o {numref}`Capítulo %s <cap-implementacao>`, sobre
a implementação da linguagem.

### Vetores e matrizes (conjuntos)

Vetores e matrizes "n" dimensionais de tipos primitivos são suportados. Um
exemplo de declaração de uma matriz:

```portugol
variáveis
   bitset : matriz[10] de lógicos;
   quadr : matriz[4][4] de inteiros;
fim-variáveis
```

O tipo do vetor ou matriz é dado pelo nome do tipo no plural (`inteiros` para
tipo inteiro, `literais` para tipo literal, etc). Os subscritos na declaração
(delimitados por `[]`) indicam o tamanho da matriz, e sua dimensão é informada
pelo número de subscritos. Portanto, `bitset` é um vetor de 10 valores lógicos,
enquanto `quadr` é uma matriz bidimensional, onde as duas dimensões têm tamanho
4.

É importante observar que matrizes são "0 based", isso é, iniciam no índice 0 e
seus índices são sempre inteiros positivos. Logo, a matriz `bitset` pode ser
usada do índice 0 até o índice 9 (inclusive). Seu índice 10 não é válido e seu
uso poderá acarretar em erros de execução (*runtime errors*). Matrizes, assim
como variáveis de tipos primitivos, são inicializadas com o valor "0" ou "nulo"
em todas as suas posições. Quando usadas como argumentos de funções, matrizes
são passadas *por valor*.

> *"As pessoas são divididas em dois grupos: aquelas que começam a contar a
> partir do '0', e aquelas que não."*
>
> Anônimo

## Estruturas condicionais

Por enquanto, apenas a estrutura se/então/senão é suportada. Essa e as demais
estruturas utilizam expressões, que são avaliadas para que uma decisão seja
tomada (repetir execução, selecionar bloco de instruções, etc). Qualquer
expressão pode ser avaliada como expressão lógica. Expressões numéricas de valor
"0" são avaliadas como falso. Demais valores numéricos são avaliados como
verdadeiro. Valores literais nulos, da mesma forma, são avaliados como falso, e
demais textos, como verdadeiro. Para maiores detalhes sobre expressões, veja a
seção {ref}`sec-expr`.

```portugol
enquanto x faça                   //depende do valor de x
enquanto "nome" = "nome" faça     //avalia como verdadeiro
enquanto verdadeiro faça          //avalia como verdadeiro
enquanto 3+5 faça                 //avalia 8 como verdadeiro
enquanto "nome" faça              //avalia como verdadeiro
enquanto 0 faça                   //avalia 0 como falso
enquanto "" faça                  //avalia como falso
enquanto '' faça                  //avalia como falso
```

O {numref}`programa-exemplo-condicional` ilustra um algoritmo simples que
verifica se o usuário é maior de idade.

```{code-block} portugol
:caption: Exemplo de programa que utiliza estrutura condicional.
:name: programa-exemplo-condicional

algoritmo idade;

variáveis
  idade : inteiro;
  nome : literal;
fim-variáveis

início
  imprima("Digite seu nome:");
  nome := leia();

  imprima(nome, ", digite sua idade:");
  idade := leia();

  se idade >= 18 então
    se idade < 60 então
      imprima("adulto!");
    senão
      imprima("ancião", '!');
    fim-se
  senão
    imprima("menor", "!");
  fim-se
fim
```

Alguns pontos a serem considerados:

- O nome do algoritmo é `idade`, assim como o nome de uma variável. Não há
  conflitos.
- O operador de atribuição é o *pascal-like* `:=`.
- A função interna `leia` pode ser usada por variáveis primitivas de qualquer
  tipo.
- A função `imprima` recebe um número variável de argumentos de qualquer tipo
  primitivo, sendo que pelo menos um argumento deve ser passado, que podem ser
  constantes literais (números, textos entre aspas, caracteres entre aspas
  simples,...), variáveis primitivas ou índices de vetores/matrizes. Os valores
  são concatenados e adicionados de um caractere EOL (end of line, ou fim de
  linha).
- Já vimos que strings/literais constantes são denotados por texto entre aspas
  duplas (`"`). Tal qual em C, um caractere entre aspas simples (`'`) é um
  caractere constante que, também, permite escapes para representar caracteres
  como EOL (`'\n'`).
- Assim como a palavra-chave `variáveis`, `então` e `senão` devem ser
  acentuadas.
- Parênteses ao redor da expressão da estrutura "se/então" são opcionais.
- Como em "variáveis/fim-variáveis", blocos `se` têm seus respectivos e
  obrigatórios `fim-se`. Não há a opção de ignorar a definição do bloco quando
  apenas um enunciado é usado, como na linguagem C onde o uso de `{}` é
  opcional. Também, não há imposições sobre a indentação. O
  {numref}`programa-se-incorreto` ilustra esse assunto.

```{code-block} portugol
:caption: Uso incorreto de estrutura condicional.
:name: programa-se-incorreto

//código inválido: faltando fim-se
algoritmo se_invalido;

início
  se x = 2 então
    imprima("ok");
  imprima("estou dentro ou fora do bloco \"se\"?");
fim
```

## Estruturas de repetição

### A estrutura "enquanto"

A estrutura "enquanto" é a mais básica e comum das estruturas de repetição. Seu
funcionamento é o mesmo que em outras linguagens populares, onde um conjunto de
instruções é executado repetidamente enquanto o valor lógico de uma dada
expressão for avaliado como "verdadeiro".

```{code-block} portugol
:caption: Exemplo de programa que utiliza a estrutura "enquanto".
:name: programa-enquanto

algoritmo fatorial;

variáveis
  res : inteiro;
  fat : inteiro;
  x : inteiro;
fim-variáveis

início
  imprima("Digite um número:");
  x := leia();

  fat := x;
  res := 1;

  enquanto x <> 0 faça
    res := res * x;
    x   := x - 1;
  fim-enquanto

  imprima("fatorial de ",fat," é igual a ",res);
fim
```

Assim como na estrutura "se/então", parênteses em volta da expressão são
opcionais e as expressões seguem as mesmas regras.

### A estrutura "repita"

A estrutura "repita" tem o funcionamento semelhante à "enquanto". A diferença
está na ordem da avaliação da condição de repetição: "enquanto" avalia a
condição antes de iniciar o laço, e "repita" avalia ao final do bloco.

```{code-block} portugol
:caption: Exemplo de programa que utiliza a estrutura "repita".
:name: programa-repita

algoritmo fatorial2;

variáveis
  res : inteiro;
  fat : inteiro;
  x   : inteiro;
fim-variáveis

início
  imprima("Digite um número:");
  fat := leia();
  res := 1;
  x := fat;

  repita
    res := res * x;
    x   := x - 1;
  até x <= 0;

  imprima("fatorial de ",fat," é igual a ",res);
fim
```

### A estrutura "para"

A estrutura "para" é uma especialização da estrutura "enquanto", e costuma ser
ensinada em cursos de programação. Sua sintaxe é semelhante ao que se vê em
literatura sobre algoritmos e estruturas de dados, entretanto, é uma sintaxe
diferente de linguagens populares como C e Java.

A estrutura "para" tem a seguinte forma:

```text
para <variável> de <expressão> até <expressão> [passo <inteiro>] faça
  [lista de comandos]
fim-para
```

Onde:

- "variável" deve ser uma variável numérica;
- "expressão" deve ser uma expressão que tem seu valor avaliado como numérico;
- "passo", se existir, deve ser seguido por um inteiro constante.

As expressões de/até controlam os valores que a variável numérica terá no início
e no fim do laço, respectivamente. Tanto o controle da frequência quanto a
decisão de incrementar ou decrementar a variável de controle são feitos pelo
termo opcional "passo", e seu valor padrão é 1. Por exemplo, para iterar o valor
de uma variável numérica `x` de 0 até 10, escreve-se:

```portugol
para x de 0 até 10 faça
  //comandos...
fim-para
```

Da mesma forma, para uma iteração decrescente, de 2 em 2, escreve-se:

```portugol
para x de 10 até 0 passo -2 faça
  //comandos...
fim-para
```

```{code-block} portugol
:caption: Exemplo de programa que utiliza a estrutura "para".
:name: programa-para

algoritmo fatorial;

variáveis
  res : inteiro;
  fat : inteiro;
  x   : inteiro;
fim-variáveis

início
  imprima("Digite um número:");
  fat := leia();

  res := 1;
  para x de fat até 1 passo -1 faça
    res := res * x;
  fim-para

  imprima("fatorial de ",fat," é igual a ",res);
fim
```

Em comparação com a estrutura "for" de linguagens com sintaxe baseada em C, há
diferenças não só de sintaxe, mas de implementação. Um "for" que itera sobre uma
variável numérica de 0 até (incluindo) 10, ao sair do laço, o valor dessa
variável será 11. Em G-Portugol, a variável terá o valor 10 ao sair do laço.
Essa diferença acontece porque a sintaxe do "para" induz a esse comportamento,
diferente da sintaxe do "for", que tem um aspecto de mais baixo nível.

```c
for(x = 0; x <= 10; x++);
printf("%d", x); //imprime "11"
```

Código equivalente em G-Portugol:

```portugol
para x de 0 até 10 faça
fim-para

imprima(x); //imprime "10"
```

Ademais, da mesma forma que o "for", é possível que a variável de controle tenha
seu valor alterado pelos comandos aninhados. Isso permite que o laço seja
encerrado prematuramente, como também é comum em estruturas como "enquanto". A
utilidade dessa técnica está no fato de G-Portugol não incorporar mecanismos
para refinar o controle de laços (como "break" e "continue", encontrados em
linguagens populares).

## Funções

Subprogramas em G-Portugol são implementados no modelo de funções, que podem ter
zero ou mais parâmetros de qualquer tipo, tanto primitivos quanto complexos
(vetores e matrizes). Opcionalmente, elas podem definir valores de retorno, que
devem ser de tipo primitivo. Tanto o retorno de dados como a passagem de
argumentos são feitos *por valor*.

Para retorno prematuro ou retorno de dados, a palavra-chave `retorne` é usada.
Para funções que retornam dados, `retorne` deve ser seguido de um operando, que
é uma expressão cujo valor deve ser compatível com o tipo da função. Já em
funções que não declaram um tipo de retorno explicitamente (equivalente a
funções de retorno "void" em C), `retorne` deve ser usado sem operando.

```{code-block} portugol
:caption: Exemplo de algoritmo que utiliza funções.
:name: programa-fatorial-recursivo

algoritmo fatorial_recursivo;

variáveis
  x : inteiro;
fim-variáveis

início
  imprima("Digite um número:");
  x := leia();
  imprima("fatorial de ",x," é igual a ",fatorial(x));
fim

função fatorial(z:inteiro) : inteiro
início
  se z = 1 então
    retorne 1;
  senão
    retorne z * fatorial(z-1);
  fim-se
fim
```

### Funções internas

Como já foi visto em exemplos anteriores, G-Portugol oferece duas funções
internas: `leia` e `imprima`, que permitem uso básico de entrada e saída,
respectivamente.

A função `leia` não recebe argumentos e retorna o valor lido da entrada padrão
(STDIN), o que significa, geralmente, ler os dados que o usuário digitar no
teclado, seguido do caractere "nova linha" (em geral, associado à tecla "Enter"
no teclado). O tipo de dado retornado por `leia` é implicitamente convertido
para o tipo primitivo exigido no contexto em que ela é usada.

A função `imprima` recebe um número variável de argumentos de qualquer tipo
primitivo, sendo que pelo menos um argumento deve ser passado. Os valores
passados como argumentos são convertidos para texto, concatenados na ordem
definida e enviados para STDOUT (em geral, associado ao *display* ou monitor).
Não há retorno de valor para esta função.

```{code-block} portugol
:caption: Exemplo de usos das funções internas "leia" e "imprima".
:name: programa-io

algoritmo io;

variáveis
  c: caractere;
  i: inteiro;
  r: real;
  l: literal;
  z: lógico;
  mat: matriz[2] de inteiros;
fim-variáveis

início
  imprima("digite um caractere");
  c := leia();
  imprima("Digite um número inteiro");
  i := leia();
  imprima("Digite um número real:");
  r := leia();
  imprima("Digite um texto:");
  l := leia();
  imprima("Digite um valor lógico (\"verdadeiro\" ou \"falso\"), \
    um número ou um texto:");
  z := leia();

  imprima("caractere: ",c,", inteiro: ",i, ", real: ",r,", \
    texto: ",l, ", lógico: ", z,"\n");
fim
```
