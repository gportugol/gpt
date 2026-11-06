# Desenvolvendo

Este documento descreve como configurar o ambiente de desenvolvimento local do
GPT usando as mesmas etapas utilizadas no pipeline do [GitHub
Actions](.github/workflows/build.yml) para GNU/Linux e Windows.

Dependências:

- g++
- make
- autoconf
- automake
- libtool
- pkg-config
- antlr4 (ferramenta ANTLR 4.x; precisa de um runtime Java)
- libantlr4-runtime-dev (runtime C++ do ANTLR 4.x)
- libpcre2-dev
- nasm (para gerar executáveis x86-64; em arm64 o `gpt -o` usa o `as` e o
  `ld` do binutils, que vêm com o `build-essential`)
- wget

## Baixar o fonte

```shell
git clone https://github.com/gportugol/gpt.git
```

## GNU/Linux

O GPT pode ser compilado nativamente em distribuições GNU/Linux, como o
Debian/Ubuntu, usando os pacotes:

### 1. Instalar dependências no Debian/Ubuntu

```shell
sudo apt install -y \
  build-essential autoconf automake libtool pkg-config \
  antlr4 libantlr4-runtime-dev libpcre2-dev nasm
```

O pacote `antlr4` instala a ferramenta (que roda em Java) e o
`libantlr4-runtime-dev` instala o runtime C++. O runtime precisa ser 4.9 ou
mais novo e a ferramenta da mesma versão 4.x do runtime (com o runtime 4.9, a
ferramenta 4.7.2 das distribuições também serve), porque o lexer e o parser
gerados por uma versão não funcionam com o runtime de outra; o `configure`
recusa outras combinações. Os pacotes do Debian 12 (4.7.2 e 4.9), do Ubuntu
22.04 (4.7.2 e 4.9) e do Debian 13 (4.9.2 e 4.9, usado no CI) funcionam. O
Ubuntu 24.04 traz a ferramenta 4.9.2 com o runtime 4.10, e o build falha;
nele, não instale o pacote `antlr4`: mantenha o runtime da distribuição e rode
a ferramenta a partir do jar da mesma versão, como no Windows:

```shell
sudo apt install -y default-jre-headless
sudo wget -O /usr/local/lib/antlr4-complete.jar \
  https://www.antlr.org/download/antlr-4.10-complete.jar
printf '#!/bin/sh\nexec java -jar /usr/local/lib/antlr4-complete.jar "$@"\n' |
  sudo tee /usr/local/bin/antlr4 >/dev/null
sudo chmod +x /usr/local/bin/antlr4
```

Troque `4.10` pela versão que `dpkg -s libantlr4-runtime-dev` mostrar. Se o
script ficar fora do `PATH`, informe-o ao `configure` pela variável
`ANTLR4_CMD`.

### 2. Configurar e compilar no Debian/Ubuntu

```shell
autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
```

O CI compila no Linux com `-Wall -Wextra -Werror`, então qualquer aviso novo
do compilador quebra o build. Para conferir antes de enviar:

```shell
./configure --prefix=/usr/local CXXFLAGS="-Wall -Wextra -Werror"
make -j$(nproc)
```

### 3. Testar no Debian/Ubuntu

A suíte de regressão compara a saída do interpretador, do binário nativo e da
tradução para C com as saídas esperadas em `test/casos/`, e as mensagens de
erro com `test/erros/`:

```shell
bash test/run_test.sh
```

Numa máquina x86-64, o backend arm64 pode ser testado com o binutils cruzado e
o QEMU, como faz o CI. O QEMU não acusa o `sp` desalinhado que o hardware
rejeita, então só o job no runner arm64 prova o alinhamento da pilha:

```shell
sudo apt install -y binutils-aarch64-linux-gnu qemu-user
GPT_TEST_ARCH=arm64 GPT_TEST_RUNNER=qemu-aarch64 bash test/run_test.sh
```

O CI também compila e testa o gpt num container `debian:trixie` `linux/386`,
o userland do pacote i386 do Debian sobre um kernel amd64: uma vez com o
`uname -m` dizendo `x86_64` e outra sob `setarch i686`, a persona `linux32`
que o sbuild usa. Nos dois casos o `gpt -o` gera x86-64.

#### Interpretador no Debian/Ubuntu

```shell
src/gpt -i exemplos/olamundo.gpt
```

#### Compilador no Debian/Ubuntu

```shell
src/gpt -o olamundo exemplos/olamundo.gpt
./olamundo
```

## Windows (MSYS2 / Mingw-w64)

O GPT é compilado no Windows usando MSYS2.

### 1. Instalar MSYS2

Baixar em <https://www.msys2.org> e instalar o MSYS2

### 2. Instalar dependências

Abra o terminal **MSYS2 UCRT64**. Os executáveis gerados nesse ambiente usam a
UCRT, que faz parte do Windows 10 e do Windows Server 2016 em diante.

```shell
pacman -Syu --noconfirm
pacman -S --noconfirm \
  autoconf automake libtool make \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-gcc-libs \
  mingw-w64-ucrt-x86_64-pcre2 pkg-config \
  mingw-w64-ucrt-x86_64-antlr4-runtime-cpp \
  unzip wget
```

O pacote `mingw-w64-ucrt-x86_64-antlr4-runtime-cpp` fornece o runtime C++ do
ANTLR4.

### 3. Instalar Java (necessário para a ferramenta ANTLR4)

A ferramenta `antlr4`, que gera o lexer e o parser durante o build, roda em
Java. Instale um JDK (por exemplo, o [Temurin 21](https://adoptium.net)) e
garanta que `java` esteja no `PATH` do terminal MSYS2.

### 4. Instalar a ferramenta ANTLR4

Baixe o jar da mesma versão do runtime instalado pelo `pacman` (verifique com
`pacman -Qi mingw-w64-ucrt-x86_64-antlr4-runtime-cpp`) e crie o script
`antlr4` que o `configure` procura no `PATH`:

```shell
mkdir -p /usr/local/lib /usr/local/bin
wget -O /usr/local/lib/antlr4-complete.jar \
  https://www.antlr.org/download/antlr-4.13.2-complete.jar
printf '#!/bin/bash\njava -jar /usr/local/lib/antlr4-complete.jar "$@"\n' \
  > /usr/local/bin/antlr4
chmod +x /usr/local/bin/antlr4
```

### 5. Instalar o NASM

```shell
pacman -S --needed mingw-w64-ucrt-x86_64-nasm
```

É o mesmo pacote que o CI instala e distribui no zip. Qualquer NASM 2.x ou
mais novo serve: o executável PE32+ é descrito campo a campo no assembly
gerado, sem macros de terceiros. O `gpt` procura o `nasm` no mesmo diretório
do `gpt.exe` e depois no `PATH`.

### 6. Compilar o GPT no Windows

```shell
export LDFLAGS="-static -static-libgcc -static-libstdc++"

autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
make install
```

### 7. Testar no Windows

Para rodar a suíte de regressão:

```shell
bash test/run_test.sh
```

#### Interpretador no Windows

```shell
gpt.exe -i exemplos/olamundo.gpt
```

#### Compilador no Windows

```shell
gpt.exe -o olamundo.exe exemplos/olamundo.gpt
./olamundo.exe
```

## Documentação

As páginas man ficam em [doc/man](doc/man). O manual é escrito em Markdown
(MyST) e gerado com Sphinx a partir de [doc/manual](doc/manual). O workflow de
release anexa o PDF a cada release. O Read the Docs publica a `master` como
`latest` e a maior tag de release como `stable`, e cancela os builds de pull
request, conforme [.readthedocs.yaml](.readthedocs.yaml). Sem a integração do
projeto no RTD com o GitHub, push e tag novos não disparam build, e ele precisa
ser iniciado no painel.

O projeto no Read the Docs deve usar o idioma Portuguese (Brazil) e ter `latest`
como versão padrão: a raiz <https://gportugol.readthedocs.io/>, usada nos links
do README, redireciona para `/pt-br/latest/`, que pode trazer mudanças ainda
não lançadas, e o manual da última release fica em `stable`. O RTD passa
o idioma do projeto ao Sphinx, sobrepondo o `language` do `conf.py`; com outro
idioma, a interface, a busca e o PDF saem nesse idioma.

### Instalar dependências

```shell
python3 -m venv .venv
. .venv/bin/activate
pip install -r doc/manual/requirements.txt
```

Para o PDF, no Debian/Ubuntu:

```shell
sudo apt install -y latexmk texlive-latex-recommended texlive-latex-extra \
  texlive-fonts-recommended texlive-lang-portuguese tex-gyre
```

### Compilar o manual

Para HTML (saída em `doc/_build/html`):

```shell
sphinx-build -M html doc/manual doc/_build -W --keep-going
```

Para PDF (saída em `doc/_build/latex/manual.pdf`):

```shell
sphinx-build -M latexpdf doc/manual doc/_build -W --keep-going
```
