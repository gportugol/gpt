# Desenvolvendo

## Instalando dependências em GNU/Linux

Para desenvolver o GPT é necessário instalar os seguintes softwares:

- g++
- make
- automake (v1.9 ou superior)
- autoconf
- libtool
- antlr (v2.6.x ou superior)
- pcre e pcrecpp
- nasm

Para satisfazer estas dependências no (K)Ubuntu ou Debian, pode-se executar o
seguinte comando:

```bash
apt install build-essential autoconf automake libtool pkg-config \
  libantlr-dev libpcre2-dev nasm
```

## Instalando dependências em MS Windows

Utilizamos os softwares MingW/MSYS para o desenvolvimento do projeto neste
sistema operacional. Eis um passo-a-passo para ter o ambiente de desenvolvimento
operacional:

1. Instale um SDK do Java

2. Instale o MingW:

   Exemplo:
   - Baixe o installer "mingw-get":
     <http://sourceforge.net/downloads/mingw/Automated%20MinGW%20Installer/mingw-get/mingw-get-0.1-mingw32-alpha-2-bin.tar.gz>
   - Descompacte em `c:\mingw` (ou no driver escolhido)
   - No prompt de comando execute:

     ```powershell
     cd c:\mingw
     path=c:\mingw\bin;%path%
     mingw-get.exe install mingwrt w32api binutils gcc g++ mingw32-make
     ```

     Isso deverá instalar os pacotes necessários do MingW

3. Instale o MSYS

   Exemplo:
   - Baixe e instale:
     <http://downloads.sourceforge.net/mingw/MSYS-1.0.11.exe>

4. Instale o MSYS DTK (necessário para ter o autoconf & cia):

   Exemplo:
   - Baixe e instale:
     <http://downloads.sourceforge.net/mingw/msysDTK-1.0.1.exe>

5. Instale o ANTLR v2.x a partir do fonte

   Exemplo:
   - Baixe o pacote `antlr-2.7.7.tar.gz` em `c:\msys\1.0\home\<usuario>`:
     <http://www.antlr2.org/download/antlr-2.7.7.tar.gz>

   - Execute o shell do MSYS
   - Descompacte, compile e instale o antlr:

     ```powershell
     tar xvfz antlr-2.7.7.tar.gz
     cd antlr-2.7.7; ./configure && make && make install
     ```

   - Crie um link simbólico para o antlr:

     ```powershell
     ln -s /usr/local/bin/antlr /usr/local/bin/runantlr
     ```

6. Instale a biblioteca PCRE

   Exemplo:
   - Baixe o pacote `pcre-8.10.tar.gz` em `c:\msys\1.0\home\<usuario>`:
     <ftp://ftp.csx.cam.ac.uk/pub/software/programming/pcre/pcre-8.10.tar.gz>

   - Execute o shell do MSYS
   - Descompacte, compile e instale o pcre:

     ```powershell
     tar xvfz pcre-8.10.tar.gz
     cd pcre-8.10; ./configure && make && make install
     ```

   Agora, com os compiladores e bibliotecas presentes e o MSYS como shell, o
   código fonte do GPT pode ser compilado.

7. Instale o NASM <http://www.nasm.us> para usar o GPT como compilador.

## Iniciando o desenvolvimento

Se você estiver utilizando o código fonte do repositório, é necessário fazer o
setup do sistema de construção com o seguinte comando:

```bash
make -f Makefile.cvs
```

ou

```bash
autoconf
```

Isto criará os `Makefile.in` necessários e o shell script `configure` utilizado
para criar os Makefiles usados pelo programa `make` para automatizar a
compilação do projeto.

Se estiver utilizando o código fonte de uma versão específica (obtida por meio
de um pacote tar.gz, por exemplo), o script `configure` já estará disponível.

**NOTA:** se estiver obtendo erros nos arquivos `Makefile.am` ao executar `make
-f Makefile.cvs`, verifique a versão do automake sendo utilizada:

```bash
automake --version
```

Se o comando acima informar uma versão inferior à 1.9, desinstale esta versão,
execute manualmente o automake1.9 ou faça as devidas configurações para que a
versão correta seja utilizada.

## Configurando e construindo

Agora, basta seguir as instruções do arquivo `INSTALL`, executando o `configure`
com as opções desejadas e, em seguida, executando `make` e `make install`, se
quiser instalar os arquivos no sistema.

## Unit Testing

- Todo código que pode se beneficiar de testes automatizados **deve** ter testes
  automatizados.
- Mensagens de commit para arquivos de teste são opcionais — use o bom senso.

(TODO: explicar a infraestrutura de testes quando houver)

## Documentando

Os arquivos de documentação ficam no diretório `doc`.

O manual está em `doc/manual` e é escrito em LaTeX. Em ambientes
Debian/(K)Ubuntu, instale:

- latex-make
- texlive-latex-base
- latex2html

Os arquivos do manual ficam em `doc/manual`, e o arquivo principal é
`manual.tex`.

Compilar:

```bash
latex manual.tex
```

Gerar PDF:

```bash
pdflatex manual.tex
```

Gerar HTML:

```bash
latex2html manual.tex
```

## Distribuindo

Para a distribuição de uma nova versão do GPT, siga os passos:

1. **Atualizar documentação**
   - Modificar a versão no manual

2. **Checar se todos os arquivos estão no repositório**

   ```bash
   svn status
   ```

3. **Testar a versão do SVN em outros ambientes**

4. **Mudar a versão no arquivo `configure.ac`**
   - Atualizar parâmetros de `AC_INIT` e `AM_INIT_AUTOMAKE`
   - Executar:

     ```bash
     autoconf && automake
     ```

5. **Atualizar NEWS**
   - O arquivo deve ser atualizado manualmente.

6. **Atualizar ChangeLog**
   - Instalar `php-cli`
   - Executar:

     ```bash
      php stuff/svn2cl.php > ChangeLog
     ```

7. **Criar pacotes**
   - **tar.gz**

     ```bash
     make distclean; mkdir build; cd build; \
     ../configure && make && make distcheck
     ```

   - **debian**
     (TODO)

   - **MS Windows**
     - Usar o Inno Setup (<http://www.jrsoftware.org/isdl.php>)
     - A configuração está em `/gpt/packages/win_setup/`
     - Verificar terminação de linha nos arquivos de texto

8. **Commit e tag**

   ```bash
   svn commit -m "Congelando a versao 1.1"
   svn copy https://username@svn.berlios.de/svnroot/repos/gpt/trunk/gpt \
           https://username@svn.berlios.de/svnroot/repos/gpt/tags/gpt-1.1 \
    -m "Release 1.1"
   ```

9. **Upload e publicação**
   - Fazer upload dos arquivos para gpt.berlios.de
   - Atualizar o site e publicar as novidades
