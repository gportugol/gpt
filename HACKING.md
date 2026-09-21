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
- nasm
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

O pacote `antlr4` do Debian/Ubuntu instala a ferramenta (que roda em Java) e o
`libantlr4-runtime-dev` instala o runtime C++. As duas versões precisam ser
compatíveis (o CI usa o Debian trixie, onde ambas são 4.13).

### 2. Configurar e compilar no Debian/Ubuntu

```shell
autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
```

### 3. Testar no Debian/Ubuntu

A suíte de regressão compara a saída do interpretador, do binário nativo e da
tradução para C com as saídas esperadas em `test/casos/`, e as mensagens de
erro com `test/erros/`:

```shell
bash test/run_test.sh
```

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

Abra o terminal **MSYS2 MinGW 64-bit**.

```shell
pacman -Syu --noconfirm
pacman -S --noconfirm \
  autoconf automake libtool make \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-gcc-libs \
  mingw-w64-x86_64-pcre2 pkg-config \
  mingw-w64-x86_64-antlr4-runtime-cpp \
  mingw-w64-x86_64-nasm \
  mingw-w64-i686-gcc mingw-w64-i686-crt-git \
  mingw-w64-i686-headers-git mingw-w64-i686-winpthreads-git \
  unzip wget
```

O pacote `mingw-w64-x86_64-antlr4-runtime-cpp` fornece o runtime C++ do
ANTLR4. Os pacotes `mingw-w64-i686-*` fornecem o `gcc` de 32 bits usado para
ligar os executáveis gerados pelo `gpt -o` (código x86 de 32 bits).

### 3. Instalar Java (necessário para a ferramenta ANTLR4)

A ferramenta `antlr4`, que gera o lexer e o parser durante o build, roda em
Java. Instale um JDK (por exemplo, o [Temurin 21](https://adoptium.net)) e
garanta que `java` esteja no `PATH` do terminal MSYS2.

### 4. Instalar a ferramenta ANTLR4

Baixe o jar da mesma versão do runtime instalado pelo `pacman` (verifique com
`pacman -Qi mingw-w64-x86_64-antlr4-runtime-cpp`) e crie o script `antlr4`
que o `configure` procura no `PATH`:

```shell
mkdir -p /usr/local/lib /usr/local/bin
wget -O /usr/local/lib/antlr4-complete.jar \
  https://www.antlr.org/download/antlr-4.13.2-complete.jar
printf '#!/bin/bash\njava -jar /usr/local/lib/antlr4-complete.jar "$@"\n' \
  > /usr/local/bin/antlr4
chmod +x /usr/local/bin/antlr4
```

### 5. NASM

O NASM já foi instalado pelo `pacman` (`mingw-w64-x86_64-nasm`). O `gpt`
procura o `nasm` no `PATH` ou no mesmo diretório do executável `gpt.exe`.

### 6. Compilar o GPT no Windows

```shell
export CXXFLAGS="-O2 -std=gnu++17 -static -static-libgcc -static-libstdc++"
export LDFLAGS="-static -static-libgcc -static-libstdc++"

autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
make install
```

### 7. Testar no Windows

Para rodar a suíte de regressão, o `gcc` de 32 bits precisa estar no `PATH`
(é ele que liga os binários gerados pelo `gpt -o`):

```shell
export PATH="/mingw32/bin:$PATH"
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

Documentação na pasta [doc](doc) e o manual (LaTeX) em [doc/manual](doc/manual).

### Instalar dependências no Debian/Ubuntu

```shell
sudo apt install -y latex-make texlive-latex-base texlive-lang-portuguese latex2html
```

### Compilar o manual

Para PDF:

```shell
cd doc/manual
pdflatex manual.tex
```

Para HTML:

```shell
cd doc/manual
latex2html manual.tex
```
