# Instalação Usando os Fontes

## Pré-requisitos

- ANTLR 4:
  Ferramenta para construção de compiladores (ferramenta `antlr4`, que precisa
  de Java, e o runtime C++ `libantlr4-runtime`; testado com o runtime v4.9,
  v4.10 e v4.13). O runtime precisa ser 4.9 ou mais novo e a ferramenta da
  mesma versão 4.x (com o runtime 4.9, de 4.7 a 4.9); o `configure` recusa
  outras combinações.
  <http://www.antlr.org>

- Perl Compatible Regular Expressions (testado com v6.4):
  Biblioteca de expressões regulares.
  <http://www.pcre.org/>

- NASM - The Netwide Assembler (2.x ou mais novo, com suporte a x86-64):
  Assembler usado para compilação.
  <https://www.nasm.us/>

A instalação destes componentes está além do escopo deste documento.

## Compilação

Comandos para compilação e instalação padrão, assumindo um ambiente GNU
(GNU/Linux, MS Windows + MingW ou Cygwin, etc)

```bash
tar xvfz gpt-xxx.tar.gz
cd gpt-xxx
./configure
make
make install
```

Se precisar de acesso root para o diretório alvo:

```bash
su
make install
```

### Opções de configuração

#### ANTLR

O script `configure` procura o programa `antlr4` no `PATH` e a biblioteca
`libantlr4-runtime` (com os headers em `antlr4-runtime/`) nos diretórios
padrão do sistema, respeitando `CPPFLAGS` e `LDFLAGS`. Se o `antlr4` estiver
em outro lugar, informe-o pela variável de ambiente `ANTLR4_CMD`; se o
runtime estiver em outro prefixo, use `CPPFLAGS`/`LDFLAGS` ou as variáveis
`ANTLR4_INC` e `ANTLR4_LIB`:

```bash
ANTLR4_CMD=/caminho/para/antlr4 \
ANTLR4_INC="-isystem /prefixo/include/antlr4-runtime" \
ANTLR4_LIB="-L/prefixo/lib -lantlr4-runtime" ./configure
```

O runtime C++ do ANTLR4 exige C++17; o `configure` acrescenta `-std=gnu++17`
a `CXXFLAGS` quando o compilador não usa esse padrão por omissão.

#### Proteções de compilação

O `configure` testa e acrescenta, antes das flags informadas pelo usuário,
`-D_FORTIFY_SOURCE=2` (se o compilador ou o `CPPFLAGS` ainda não definirem
um nível), `-D_GLIBCXX_ASSERTIONS`, `-fstack-protector-strong`,
`-fstack-clash-protection`, `-fcf-protection`, `-mbranch-protection=standard`,
`-Wl,-z,relro` e `-Wl,-z,now`. Cada flag só entra se o compilador e o linker a
aceitarem sem nenhum aviso, então as que não se aplicam à plataforma (como as
`-z` no Windows, a `-fcf-protection` fora de x86 e a `-mbranch-protection`
fora de arm64) ficam de fora. Para não acrescentar nenhuma delas:

```bash
./configure --disable-hardening
```

#### Devel

Se você deseja que a biblioteca dinâmica (.so) e headers do compilador sejam
instalados no sistema, execute o script `configure` da seguinte forma:

```bash
./configure --enable-install-devel
```

Essa opção é necessária se você deseja utilizar a opção "análise em segundo
plano" do programa [GPTEditor](https://github.com/gportugol/gpteditor).

Nota: para desinstalar apenas os arquivos `devel` (header e libs) use:

```bash
make uninstall-devel
```

## Biblioteca padrão

Para utilizar a (pseudo) biblioteca padrão distribuída neste pacote deve-se
adicionar as variáveis de ambiente a variável `GPT_INCLUDE` contendo o caminho
do arquivo `base.gpt`. Exemplo (shell bash):

Adicione ao script de inicialização de ambiente:

```bash
export GPT_INCLUDE="/usr/local/lib/gpt/base.gpt"
```

Outros arquivos podem ser incluídos, separando os caminhos por ":".

## Outras opções

Para maiores detalhes, leia [INSTALL.default](INSTALL.default)

Se o código fonte foi baixado diretamente do repositório, é necessário gerar o
script `configure`, executando o seguinte comando:

```bash
make -f Makefile.cvs
```
