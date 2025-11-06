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
- libantlr-dev (ANTLR 2.x)
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
  libantlr-dev libpcre2-dev nasm
```

### 2. Configurar e compilar no Debian/Ubuntu

```shell
autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
```

### 3. Testar no Debian/Ubuntu

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
  tar unzip wget
```

### 3. Instalar Java (necessário para ANTLR 2.x)

```shell
wget https://download.java.net/java/GA/jdk25.0.1/2fbf10d8c78e40bd87641c434705079d/8/GPL/openjdk-25.0.1_windows-x64_bin.zip
unzip openjdk-25.0.1_windows-x64_bin.zip
export PATH=$PATH:$(pwd)/jdk-25.0.1/bin
```

### 4. Compilar ANTLR 2.7.7 no Windows (com patches aplicados)

O ANTLR 2 é muito antigo e não compila corretamente no `Mingw64` sem correções.
Precisamos aplicar dois patches usando o `sed`.

#### Baixar e extrair

```shell
wget http://www.antlr2.org/download/antlr-2.7.7.tar.gz
tar xvfz antlr-2.7.7.tar.gz
cd antlr-2.7.7
```

#### Patch 1 — incluir \_stricmp no Windows

Insere no topo de `CharScanner.hpp`:

```shell
sed -i \
  '1i \
  #ifdef _WIN32\n\
  #include <string.h>\n\
  #define strcasecmp _stricmp\n\
  #endif' \
  lib/cpp/antlr/CharScanner.hpp
```

#### Patch 2 — substituir binary_function obsoleto

```shell
sed -i \
  's/struct CharScannerLiteralsLess : public binary_function<string, string, bool>/\
  struct CharScannerLiteralsLess { \
    bool operator()(const std::string& x, const std::string& y) const { \
      return strcasecmp(x.c_str(), y.c_str()) < 0; \
    } \
  };/g' \
  lib/cpp/antlr/CharScanner.hpp
```

Esses patches corrigem:

- Incompatibilidade com `binary_function` removido no C++17
- Ausência de `strcasecmp` no Windows

#### Compilar o ANTLR com flags estáticas

```shell
export CXXFLAGS="-O2 -std=gnu++14 -static -static-libgcc -static-libstdc++"
export LDFLAGS="-static -static-libgcc -static-libstdc++"

autoreconf -fi
./configure --prefix=/usr/local
make -j$(nproc)
make install
```

Criar o alias usado pelo GPT:

```shell
ln -s /usr/local/bin/antlr /usr/local/bin/runantlr || true
```

### 5. Instalar o NASM

```shell
wget https://www.nasm.us/pub/nasm/releasebuilds/0.99.06/nasm-0.99.06-win32.zip
unzip nasm-0.99.06-win32.zip
cp nasm-0.99.06/nasm.exe /mingw64/bin/
```

### 6. Compilar o GPT no Windows

```shell
export CXXFLAGS="-O2 -std=gnu++14 -static -static-libgcc -static-libstdc++"
export LDFLAGS="-static -static-libgcc -static-libstdc++"

autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
make install
```

### 7. Testar no Windows

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
sudo apt install -y latex-make texlive-latex-base latex2html
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
