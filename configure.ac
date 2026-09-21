#Nota windows:
#aclocal do cygwin fica feliz somente
#tiver quebra de linha no format UNIX

AC_PREREQ([2.71])

AC_INIT([gpt],[1.3.0])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS([config.h])
AC_LANG(C++)
dnl Keep the CXXFLAGS given by the user; AC_PROG_CXX would default them
dnl to "-g -O2" when the variable is unset.
: ${CXXFLAGS=}
AC_PROG_CXX
LT_INIT


dnl------------------------------
dnl debug options
dnl------------------------------

AC_ARG_ENABLE(debug,
        AS_HELP_STRING([--enable-debug=ARG],[enables debug symbols (yes|no|full) [default=no]]),
[
  case $enableval in
    yes)
      use_debug_code="yes"
      use_debug_define=yes
      ;;
    full)
      use_debug_code="full"
      use_debug_define=yes
      ;;
    *)
      use_debug_code="no"
      use_debug_define=no
      ;;
  esac
],
  [use_debug_code="no"
    use_debug_define=no
])

if test "$use_debug_code" != "no"; then
  if test $use_debug_code = "full"; then
    CXXFLAGS="-g3 $CXXFLAGS"
  else
    CXXFLAGS="-g -O2 $CXXFLAGS"
  fi
else
  CXXFLAGS="-O2 $CXXFLAGS"
fi

if test "$use_debug_define" = "yes"; then
  CXXFLAGS="-DDEBUG $CXXFLAGS"
fi


dnl------------------------------
dnl C++ standard
dnl------------------------------

dnl The ANTLR4 C++ runtime needs C++17 (std::any) from 4.10 on, and GCC
dnl before 11 and Apple clang still default to an older standard.
AC_MSG_CHECKING([for a C++17 compiler])
gpt_cxx17=no
for gpt_std_flag in "" -std=gnu++17 -std=c++17; do
  gpt_save_CXXFLAGS=$CXXFLAGS
  CXXFLAGS="$CXXFLAGS${gpt_std_flag:+ $gpt_std_flag}"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if __cplusplus < 201703L
#error "C++17 required"
#endif
#include <any>
]], [[std::any a = 1; return 0;]])],
    [gpt_cxx17=yes], [CXXFLAGS=$gpt_save_CXXFLAGS])
  if test $gpt_cxx17 = yes; then
    break
  fi
done
if test $gpt_cxx17 != yes; then
  AC_MSG_RESULT([no])
  AC_MSG_ERROR(
  [
    O compilador $CXX nao aceita C++17, exigido pelo runtime C++ do ANTLR4.
    Use um compilador mais novo ou informe a opcao em CXXFLAGS.
  ])
elif test -n "$gpt_std_flag"; then
  AC_MSG_RESULT([yes, with $gpt_std_flag])
else
  AC_MSG_RESULT([yes])
fi


dnl------------------------------
dnl Check SO
dnl------------------------------

if test $version_type = "windows"; then
   SO_WINDOWS="yes"
else
   SO_WINDOWS="no"
fi

AM_CONDITIONAL(BUILD_DEBUGGER, test x$SO_WINDOWS = xno)

dnl------------------------------
dnl Checks for ANTLR4
dnl------------------------------

AC_ARG_VAR([ANTLR4_CMD], [antlr4 tool (default: the antlr4 found in PATH)])
AC_ARG_VAR([ANTLR4_INC],
  [preprocessor flags for the ANTLR4 C++ runtime headers,
   e.g. -isystem /opt/antlr4/include/antlr4-runtime (default: autodetected)])
AC_ARG_VAR([ANTLR4_LIB],
  [linker flags for the ANTLR4 C++ runtime,
   e.g. -L/opt/antlr4/lib -lantlr4-runtime (default: autodetected)])

AC_PATH_PROG(ANTLR4_CMD, antlr4)

if test "x${ANTLR4_CMD}" = "x"; then
  AC_MSG_ERROR(
  [
    O programa "antlr4" nao foi encontrado no seu sistema (PATH).
    GPT precisa do ANTLR4 (versao 4.x) instalado.
    No Debian/Ubuntu: sudo apt-get install antlr4
    Baixe em: http://www.antlr.org
  ])
fi

AC_MSG_CHECKING([for ANTLR4])
AC_MSG_RESULT([$ANTLR4_CMD])

dnl Check for ANTLR4 C++ runtime library
dnl On MSYS2 the runtime lives under the active environment's prefix
dnl ($MINGW_PREFIX, e.g. /ucrt64). It ships only an import library there,
dnl which the AC_CHECK_LIB test cannot find under -static in LDFLAGS.
dnl Everywhere else the link test honours LDFLAGS and the default library
dnl path, whatever the architecture directory is.
AC_MSG_CHECKING([for ANTLR4 C++ runtime library])

if test -n "$ANTLR4_LIB"; then
  AC_MSG_RESULT([$ANTLR4_LIB (from ANTLR4_LIB)])
elif test -n "$MINGW_PREFIX" && { test -f "$MINGW_PREFIX/lib/libantlr4-runtime.a" || test -f "$MINGW_PREFIX/lib/libantlr4-runtime.dll.a"; }; then
  ANTLR4_LIB="-L$MINGW_PREFIX/lib -lantlr4-runtime"
  AC_MSG_RESULT([yes (in $MINGW_PREFIX/lib)])
else
  AC_MSG_RESULT([checking with linker...])
  AC_CHECK_LIB([antlr4-runtime], [main],
    [ANTLR4_LIB="-lantlr4-runtime"],
    [])
fi

if test "x$ANTLR4_LIB" = "x"; then
  AC_MSG_ERROR(
    [
      A biblioteca libantlr4-runtime nao foi encontrada.
      GPT precisa da biblioteca C++ runtime do ANTLR4 instalada.
      No Debian/Ubuntu: sudo apt-get install libantlr4-runtime-dev
      No MSYS2 (UCRT64): pacman -S mingw-w64-ucrt-x86_64-antlr4-runtime-cpp
      Em outro prefixo, informe LDFLAGS=-L/prefixo/lib ou
      ANTLR4_LIB="-L/prefixo/lib -lantlr4-runtime".
      Baixe em: http://www.antlr.org
    ])
fi

dnl Check for ANTLR4 headers. The sources include "antlr4-runtime.h", which
dnl Debian and MSYS2 install under an antlr4-runtime/ directory; -isystem
dnl also silences the warnings of the runtime headers. Other prefixes come
dnl from CPPFLAGS or from ANTLR4_INC, and every candidate is compiled so
dnl that the error names the headers, not the first source file. VERSION
dnl comes from confdefs.h and clashes with the runtime's own VERSION.
AC_MSG_CHECKING([for antlr4-runtime.h])

if test -n "$ANTLR4_INC"; then
  gpt_antlr4_inc_dirs=env
else
  gpt_antlr4_inc_dirs="cppflags /usr/include/antlr4-runtime $MINGW_PREFIX/include/antlr4-runtime /usr/local/include/antlr4-runtime"
fi
gpt_antlr4_inc_found=no
gpt_save_CPPFLAGS=$CPPFLAGS
for gpt_dir in $gpt_antlr4_inc_dirs; do
  case $gpt_dir in
    env) ;;
    cppflags) ANTLR4_INC="" ;;
    *)
      test -f "$gpt_dir/antlr4-runtime.h" || continue
      ANTLR4_INC="-isystem $gpt_dir" ;;
  esac
  CPPFLAGS="$gpt_save_CPPFLAGS $ANTLR4_INC"
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#undef VERSION
#include "antlr4-runtime.h"
]], [[]])],
    [gpt_antlr4_inc_found=yes])
  if test $gpt_antlr4_inc_found = yes; then
    break
  fi
done
CPPFLAGS=$gpt_save_CPPFLAGS

if test $gpt_antlr4_inc_found = yes; then
  if test -n "$ANTLR4_INC"; then
    AC_MSG_RESULT([yes ($ANTLR4_INC)])
  else
    AC_MSG_RESULT([yes])
  fi
else
  ANTLR4_INC=""
  AC_MSG_RESULT([no])
  AC_MSG_ERROR(
    [
      O header antlr4-runtime.h nao foi encontrado ou nao compila.
      GPT precisa dos headers da biblioteca C++ runtime do ANTLR4 instalados.
      No Debian/Ubuntu: sudo apt-get install libantlr4-runtime-dev
      No MSYS2 (UCRT64): pacman -S mingw-w64-ucrt-x86_64-antlr4-runtime-cpp
      Em outro prefixo, informe CPPFLAGS=-I/prefixo/include/antlr4-runtime ou
      ANTLR4_INC="-isystem /prefixo/include/antlr4-runtime".
      Baixe em: http://www.antlr.org
    ])
fi

dnl The generated code and the serialized ATN changed in 4.10, so from then
dnl on the tool and the runtime must be the same 4.x release: the 4.9.2 tool
dnl of Ubuntu 24.04 produces a parser its 4.10 runtime does not compile.
dnl Version.h exists from 4.10 on; a runtime without it is taken as 4.9, the
dnl oldest release GPT builds with, and it also accepts the 4.7.2 tool of
dnl Debian 12 and Ubuntu 22.04 (same ATN format; the test suite passes).
AC_MSG_CHECKING([for the ANTLR4 C++ runtime version])
CPPFLAGS="$gpt_save_CPPFLAGS $ANTLR4_INC"
gpt_antlr4_rt_major=
gpt_antlr4_rt_minor=
AC_COMPUTE_INT([gpt_antlr4_rt_major], [ANTLRCPP_VERSION_MAJOR],
  [[#undef VERSION
#include "Version.h"]], [gpt_antlr4_rt_major=])
AC_COMPUTE_INT([gpt_antlr4_rt_minor], [ANTLRCPP_VERSION_MINOR],
  [[#undef VERSION
#include "Version.h"]], [gpt_antlr4_rt_minor=])
CPPFLAGS=$gpt_save_CPPFLAGS
if test -n "$gpt_antlr4_rt_major" && test -n "$gpt_antlr4_rt_minor"; then
  gpt_antlr4_rt_version="$gpt_antlr4_rt_major.$gpt_antlr4_rt_minor"
  AC_MSG_RESULT([$gpt_antlr4_rt_version])
else
  gpt_antlr4_rt_version=4.9
  AC_MSG_RESULT([4.9 or older])
fi

AC_MSG_CHECKING([for the antlr4 tool version])
gpt_antlr4_tool_version=`$ANTLR4_CMD 2>&1 | sed -n 's/^ANTLR Parser Generator *Version *\([[0-9]][[0-9]]*\.[[0-9]][[0-9]]*\).*/\1/p' | sed 1q`
if test -z "$gpt_antlr4_tool_version"; then
  AC_MSG_RESULT([unknown])
  AC_MSG_WARN([could not read the version of $ANTLR4_CMD; it has to be $gpt_antlr4_rt_version, like the runtime])
else
  AC_MSG_RESULT([$gpt_antlr4_tool_version])
  gpt_antlr4_match=no
  case $gpt_antlr4_rt_version in
    4.9)
      case $gpt_antlr4_tool_version in
        4.7 | 4.8 | 4.9) gpt_antlr4_match=yes ;;
      esac ;;
    *)
      test "$gpt_antlr4_tool_version" = "$gpt_antlr4_rt_version" &&
        gpt_antlr4_match=yes ;;
  esac
  if test $gpt_antlr4_match != yes; then
    AC_MSG_ERROR(
      [
        A ferramenta antlr4 ($ANTLR4_CMD) e da versao $gpt_antlr4_tool_version e o runtime C++
        do ANTLR4 e da versao $gpt_antlr4_rt_version. O lexer e o parser gerados por uma
        versao nao funcionam com o runtime de outra: GPT precisa do runtime
        4.9 ou mais novo e da ferramenta na mesma versao 4.x do runtime (com
        o runtime 4.9, a ferramenta pode ser de 4.7 a 4.9).
        Use o jar da versao $gpt_antlr4_rt_version (https://www.antlr.org/download/) por um
        script que execute "java -jar", informado em ANTLR4_CMD ou no PATH
        como antlr4, ou instale o runtime da versao $gpt_antlr4_tool_version. Veja HACKING.md.
      ])
  fi
fi

AC_SUBST(ANTLR4_INC)
AC_SUBST(ANTLR4_LIB)


dnl------------------------------
dnl checks for PCRE
dnl------------------------------

# AC_MSG_CHECKING(for pcre)

AC_CHECK_PROG(has_pcre, pcre2-config, yes)

if test "x$has_pcre" = "xyes"; then
  PCRE_CONFIG="pcre2-config"
else
  AC_MSG_ERROR(
  [
    GPT precisa da biblioteca PCRE instalada
    Baixe em: http://www.pcre.org/
  ])
fi

#pcrecpp

PCRE_INC=`${PCRE_CONFIG} --cflags`
PCRE_LIB=`${PCRE_CONFIG} --libs8`
AC_SUBST(PCRE_INC)
AC_SUBST(PCRE_LIB)


dnl------------------------------
dnl checks if we want to install
dnl the libs and headers
dnl------------------------------

AC_ARG_ENABLE([install-devel],
    [AS_HELP_STRING([--enable-install-devel],[instala bibliotecas e headers])],
    [INSTALL_DEVEL="yes"],
    [INSTALL_DEVEL="no"]
)

AM_CONDITIONAL(INSTALL_DEVEL, test x$INSTALL_DEVEL = xyes)


dnl------------------------------
dnl the end
dnl------------------------------

AC_CONFIG_FILES([Makefile
        src/Makefile
        doc/Makefile
        doc/man/Makefile
        doc/man/pt_BR/Makefile
        exemplos/Makefile
        lib/Makefile
        src/modules/Makefile
        src/modules/parser/Makefile
        src/modules/interpreter/Makefile
        src/modules/c_translator/Makefile
        src/modules/x86/Makefile
      ])
AC_OUTPUT

AC_MSG_NOTICE([
Sumario:
	ANTLR4 command  : $ANTLR4_CMD
	ANTLR4 version  : ${gpt_antlr4_tool_version:-unknown} (runtime $gpt_antlr4_rt_version)
	ANTLR4 includes : $ANTLR4_INC
	ANTLR4 lib      : $ANTLR4_LIB

	pcre includes   : $PCRE_INC
	pcre lib        : $PCRE_LIB

	CXXFLAGS        : $CXXFLAGS
	MS Windows      : $SO_WINDOWS
])
