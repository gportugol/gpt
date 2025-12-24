#Nota windows:
#aclocal do cygwin fica feliz somente
#tiver quebra de linha no format UNIX

AC_PREREQ([2.71])

AC_INIT([gpt],[1.1])
AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS([config.h])
AC_LANG(C++)
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

CXXFLAGS=
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

AC_SUBST(ANTLR4_CMD)

dnl Check for ANTLR4 C++ runtime library
dnl On MSYS2/MinGW, we need to add /mingw64/lib to the library path
ANTLR4_LIB=""

AC_MSG_CHECKING([for ANTLR4 C++ runtime library])

dnl First check if library exists in standard path or /mingw64/lib
if test -f "/mingw64/lib/libantlr4-runtime.a" || test -f "/mingw64/lib/libantlr4-runtime.dll.a"; then
  ANTLR4_LIB="-L/mingw64/lib -lantlr4-runtime"
  AC_MSG_RESULT([yes (in /mingw64/lib)])
elif test -f "/usr/lib/libantlr4-runtime.so" || test -f "/usr/lib/libantlr4-runtime.a"; then
  ANTLR4_LIB="-lantlr4-runtime"
  AC_MSG_RESULT([yes])
elif test -f "/usr/lib/x86_64-linux-gnu/libantlr4-runtime.so" || test -f "/usr/lib/x86_64-linux-gnu/libantlr4-runtime.a"; then
  ANTLR4_LIB="-lantlr4-runtime"
  AC_MSG_RESULT([yes])
else
  dnl Fallback: try AC_CHECK_LIB
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
      No MSYS2: pacman -S mingw-w64-x86_64-antlr4-runtime-cpp
      Baixe em: http://www.antlr.org
    ])
fi

dnl Check for ANTLR4 headers - they are in a subdirectory on Debian/MSYS2
dnl Use -isystem to suppress warnings from ANTLR4 headers
AC_MSG_CHECKING([for antlr4-runtime.h])
if test -f "/usr/include/antlr4-runtime/antlr4-runtime.h"; then
  ANTLR4_INC="-isystem /usr/include/antlr4-runtime"
  AC_MSG_RESULT([yes (in /usr/include/antlr4-runtime)])
elif test -f "/mingw64/include/antlr4-runtime/antlr4-runtime.h"; then
  ANTLR4_INC="-isystem /mingw64/include/antlr4-runtime"
  AC_MSG_RESULT([yes (in /mingw64/include/antlr4-runtime)])
elif test -f "/usr/include/antlr4-runtime.h"; then
  ANTLR4_INC=""
  AC_MSG_RESULT([yes])
else
  AC_MSG_RESULT([no])
  AC_MSG_ERROR(
    [
      O header antlr4-runtime.h nao foi encontrado.
      GPT precisa dos headers da biblioteca C++ runtime do ANTLR4 instalados.
      No Debian/Ubuntu: sudo apt-get install libantlr4-runtime-dev
      No MSYS2: pacman -S mingw-w64-x86_64-antlr4-runtime-cpp
      Baixe em: http://www.antlr.org
    ])
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
	ANTLR4 includes : $ANTLR4_INC
	ANTLR4 lib      : $ANTLR4_LIB

	pcre includes   : $PCRE_INC
	pcre lib        : $PCRE_LIB

	CXXFLAGS        : $CXXFLAGS
	MS Windows      : $SO_WINDOWS
])
