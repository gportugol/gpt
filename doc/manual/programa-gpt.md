(cap-programa-gpt)=

# O programa GPT

## Introdução

GPT é a ferramenta principal da linguagem G-Portugol. Entre suas funções
principais estão:

- compilar algoritmos;
- traduzir algoritmos para outras linguagens;
- executar algoritmos.

Na versão atual é possível compilar algoritmos para sistemas Windows e Unices
que suportam o formato ELF e o assembler NASM. Também, há suporte apenas para a
tradução de algoritmos para a linguagem C. O resultado é um código em C 100%
equivalente ao algoritmo.

É possível, também, executar algoritmos sem gerar código ou arquivos extras, de
forma interpretada. Também, com a interpretação, é possível depurar passo a
passo o algoritmo com o uso de uma ferramenta auxiliar (um cliente de
depuração).

Vale ressaltar que, qualquer que seja o modo de uso do algoritmo (compilado,
interpretado ou traduzido), se espera que eles tenham sempre comportamentos
equivalentes. Isso é, o resultado de uma execução interpretada deve ser o mesmo
se a execução fosse feita por meio de um binário compilado.

(sec-opcoes-gerais)=

## Opções gerais

Ao executar o programa `gpt` com o argumento `-h`, é mostrado:

```text
Modo de uso: gpt [opções] algoritmos...

Opções:
   -v            mostra versão do programa
   -h            mostra esse texto
   -o <arquivo>  compila e salva programa como <arquivo> (só em x86)
   -t <arquivo>  salva o código em linguagem C como <arquivo>
   -s <arquivo>  salva o código em linguagem Assembly como <arquivo>
   -i            interpreta o algoritmo
   -d            exibe dicas no relatório de erros

   Maiores informações no manual.
```

As opções são comentadas a seguir.

`-v`
: Exibe a versão do programa `gpt`, assim como informações de copyright.

`-h`
: Exibe todas as opções suportadas pelo programa `gpt`.

`-o <arquivo>`
: Ao compilar um algoritmo, salva o executável com o nome de `<arquivo>`. Se
  essa opção for omitida na compilação, o executável recebe o nome do arquivo
  fonte sem a extensão, no diretório atual (`olamundo.gpt` gera `olamundo`, ou
  `olamundo.exe` no Windows); um fonte sem extensão gera um executável com a
  extensão `.out` (`.exe` no Windows), como `programa.out`. No Windows, um
  fonte com extensão `.exe` gera um executável cujo nome termina em `.out.exe`,
  para que o fonte não seja substituído. O código gerado é x86 de 32 bits, então
  essa opção só funciona em máquinas x86; nas demais, use a opção `-t` e compile
  o código C com o compilador do sistema.

`-t <arquivo>`
: Traduz o algoritmo para C e salva o código fonte com o nome de `<arquivo>`.

`-s <arquivo>`
: Compila o algoritmo mas não cria código executável. Salva o código em
  assembly com o nome de `<arquivo>`.

`-i`
: Executa o algoritmo diretamente, sem compilar ou criar arquivos. Opção
  conhecida como "interpretação" ou "scripting".

`-d`
: Exibe mais informações no relatório de erros, como dicas de como proceder
  para solucionar erros de sintaxe.

O último argumento (`algoritmos...`) é uma lista de arquivos contendo o código
fonte em G-Portugol, embora seja mais comum utilizar um arquivo apenas para cada
algoritmo. Quando mais de um arquivo é informado, o primeiro deve ser aquele que
contém a declaração do algoritmo e o bloco principal.

## Tratamento de erros

Em geral, as ferramentas de diversas linguagens de programação oferecem o mínimo
de informações acerca de erros de compilação, às vezes, tendo uma forma
"críptica" dificultando seu entendimento e posterior correção. Comum, também, é
a possibilidade de que determinados erros sejam reportados em localizações
distantes de onde o erro efetivamente se encontra no código fonte. Ademais, é
importante notar que nessas linguagens, os erros são reportados em inglês.

O tratamento de erros é um aspecto importante do programa GPT. A reportagem de
erros deve ser o mais clara possível e ter um formato uniforme, informando o
local exato onde o erro foi localizado. Deve, também, fornecer dicas de como
proceder para corrigir o erro, em alguns casos.

No momento, o analisador do GPT percorre o código fonte à procura de erros e,
mesmo que encontre, continua a análise à procura de mais erros. Uma outra
abordagem seria interromper a análise assim que um erro fosse encontrado.

## Execução de programas

### Compilação e geração de código executável

O programa GPT é capaz de gerar código executável para arquiteturas compatíveis
com x86 e em dois formatos: ELF (*Executable and Linkable Format*) e PE
(*Portable Executable*). Sistemas Unix, em geral, suportam o formato ELF, e o
formato PE é conhecido nos sistemas Microsoft Windows. Após o processo de
análise, o compilador gera código em Assembly para, então, usar o NASM (Netwide
Assembler) como *backend* para montar e criar um executável válido.
Consequentemente, não existe etapa de linkagem. A fase de otimização de código
também não foi implementada.

Para usar esse recurso, é necessário que o NASM esteja instalado no sistema. Ele
pode ser encontrado em <https://www.nasm.us/>. No Windows e no GNU/Linux, o GPT
procura o `nasm` primeiro no diretório do próprio executável e depois nos
diretórios do `PATH`.

### Tradução para a linguagem C

É possível usar o GPT para traduzir algoritmos escritos em G-Portugol para
linguagem C. O código resultante, entretanto, não é criado com o intuito de ser
facilmente lido, visto que a tradução não é direta. Isso é, o comportamento
esperado pelo algoritmo deve refletir o comportamento do código em C. Sendo
assim, códigos extras são adicionados no arquivo resultante, para, por exemplo,
permitir passagem de matrizes por valor, inicialização automática de variáveis e
outras abstrações.

A tradução para C é limitada no que diz respeito a nomes e identificadores. É
possível, por exemplo, declarar uma variável em G-Portugol com o nome de
`printf`. Ao traduzir para C e tentar compilar o código resultante, o compilador
pode emitir avisos e erros, visto que `printf` é uma função da biblioteca
padrão, usada no código C resultante. Da mesma forma, identificadores com
*underlines* antes ou depois em seus nomes (como `__leia_texto`) devem ser
evitados, pois muitos identificadores internos utilizam essas convenções de
nomeação, e seu uso pode acarretar em conflitos durante a tradução.

### Interpretação de código

O programa GPT permite que o algoritmo seja executado sem gerar código binário.
Esse modo é conhecido como "interpretação" e linguagens como Perl, PHP e Ruby
utilizam esta técnica.

Esse modo permite depurar algoritmos passo a passo (por meio de um *client
debugger* como o GPTEditor) e inspecionar variáveis e a pilha de funções
enquanto o algoritmo está em execução.

A única diferença na execução de algoritmos em modo interpretado em relação a
outros modos é que as matrizes/vetores têm seus subscritos checados (*bound
checking*). Isso é, erros de execução são emitidos se um índice não existir em
uma matriz/vetor.

#### Depuração interativa

A depuração interativa é feita em modo "interpretação", portanto não gera código
binário, executando o algoritmo diretamente. Para depurar interativamente um
algoritmo é necessário um programa extra: o *client debugger*. Atualmente, o
programa GPTEditor suporta a depuração interativa.

Entre os recursos disponíveis, pode-se citar a execução passo a passo em 3 modos
(comumente conhecidos como "step into", "step over" e "step out"), inspeção de
variáveis locais/globais e pontos de parada (*breakpoints*).

A depuração ocorre tendo o programa GPT se comunicando via socket com o cliente
(ex: GPTEditor), iniciando transmissão de dados entre esses dois pontos. As
informações enviadas pelo GPT (variáveis, breakpoints, etc) usam o formato
baseado em XML, enquanto o cliente envia comandos simples (o que evita que o
programa GPT necessite de um XML parser).

Por exemplo, a pilha de funções pode ser representada da seguinte forma:

```xml
<stackinfo>
  <entry id="0" function="@global" line="10"/>
  <entry id="1" function="funcTeste" line="18"/>
</stackinfo>
```

### Processando algoritmos divididos em múltiplos arquivos

A partir da versão 1.0, o GPT suporta processar algoritmos divididos em
múltiplos arquivos. Esse recurso é possível utilizando duas formas:

- passando os arquivos como opções na linha de comando;
- utilizando a variável de ambiente `GPT_INCLUDE`.

A primeira forma é explicada na seção {ref}`sec-opcoes-gerais`. A segunda forma
pretende facilitar a utilização de funções que devem estar disponíveis por
padrão a cada execução/compilação dos algoritmos. Pode-se definir a variável de
ambiente `GPT_INCLUDE` contendo vários caminhos de arquivos separados por `:`
(ou `;` no Windows). Em sistemas Unix, por exemplo, pode-se criar essa variável
da seguinte forma (utilizando Bash):

```shell
export GPT_INCLUDE="/usr/local/lib/gpt/base.gpt:/usr/local/lib/gpt/util.gpt"
```

Onde `base.gpt` e `util.gpt` são arquivos contendo funções escritas em
G-Portugol.

Os arquivos passados pela linha de comando e/ou que se encontram na variável
`GPT_INCLUDE` são concatenados e processados como se o algoritmo estivesse em
apenas um arquivo. Portanto, arquivos extras não devem ter declaração de
algoritmo, bloco de variáveis globais ou bloco principal.
