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
compatíveis (o CI usa o Debian trixie, onde ambas são 4.9).

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
wget https://www.nasm.us/pub/nasm/releasebuilds/0.99.05/nasm-0.99.05-win32.zip
echo "cc9d5e2890e230f51db617e414b9a5e6e107d1cd3a80d19018c8a53bde277408  nasm-0.99.05-win32.zip" | sha256sum -c -
unzip nasm-0.99.05-win32.zip
cp nasm.exe /ucrt64/bin/
```

O `gpt -o` no Windows precisa desse NASM antigo, o mesmo que o CI usa e
distribui no zip: as macros que montam o executável PE não são aceitas pelo
NASM 2.16 nem pelo 3.01. O `gpt` procura o `nasm` no mesmo diretório do
`gpt.exe` e depois no `PATH`.

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

O projeto no Read the Docs deve usar o idioma Portuguese (Brazil) e ter `stable`
como versão padrão. O RTD passa o idioma do projeto ao Sphinx, sobrepondo o
`language` do `conf.py`; com outro idioma, a interface, a busca e o PDF saem
nesse idioma.

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
