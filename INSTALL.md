# Instalação Usando os Fontes

## Pré-requisitos

- ANTLR 4:
  Ferramenta para construção de compiladores (ferramenta `antlr4`, que precisa
  de Java, e o runtime C++ `libantlr4-runtime`; testado com v4.9 e v4.13).
  <http://www.antlr.org>

- Perl Compatible Regular Expressions (testado com v6.4):
  Biblioteca de expressões regulares.
  <http://www.pcre.org/>

- NASM - The Netwide Assembler (testado com v0.89.39):
  Assembler usado para compilação.
  <http://sourceforge.net/projects/nasm>

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
padrão do sistema. Se o `antlr4` estiver em outro lugar, informe-o pela
variável de ambiente `ANTLR4_CMD`:

```bash
ANTLR4_CMD=/caminho/para/antlr4 ./configure
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
