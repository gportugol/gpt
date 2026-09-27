#!/bin/bash
#
# GPT regression suite.
#
#   test/tester.gpt   self-checking program (must exit with code 42)
#   test/casos/       valid programs, run in the three modes:
#                       NAME.gpt            source
#                       NAME.entrada        stdin (optional)
#                       NAME.args           extra command line files
#                                           (optional, relative to casos/)
#                       NAME.saida          expected stdout (interpreter)
#                       NAME.saida.nativo   expected stdout of the native
#                                           binary, when it differs
#                       NAME.saida.c        same for the C translation
#                       NAME.codigo[.nativo|.c]  exit code (default: 0)
#                       NAME.pular          modes to skip ("interp", "nativo",
#                                           "c"), or "windows" to skip the
#                                           whole case on Windows
#   test/erros/       invalid programs:
#                       NAME.gpt            source
#                       NAME.msg            expected stderr of "gpt -s"
#
# Files whose name starts with "_" are helpers and are not run.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GPT="$SCRIPT_DIR/../src/gpt"
TMP="${TMPDIR:-/tmp}/gpt_test_$$"

FAILURES=0
PASSED=0

if [ ! -f "$GPT" ]; then
	echo "ERRO: gpt binário não encontrado em $GPT"
	echo "Execute 'make' primeiro."
	exit 1
fi

mkdir -p "$TMP"
trap 'rm -rf "$TMP"' EXIT

# Detect architecture / platform
ARCH=$(uname -m)
CAN_EXEC_X86=0
case "$ARCH" in
i?86 | x86_64 | amd64)
	CAN_EXEC_X86=1
	;;
esac

ON_WINDOWS=0
case "$(uname -o 2>/dev/null)" in
	Msys | Cygwin) ON_WINDOWS=1 ;;
esac

HAVE_GCC=0
if command -v gcc &>/dev/null; then
	HAVE_GCC=1
fi

if command -v timeout &>/dev/null; then
	TIMEOUT="timeout 60"
else
	TIMEOUT=""
fi

ok() {
	echo "✓ $1"
	PASSED=$((PASSED + 1))
}

fail() {
	echo "✗ $1"
	FAILURES=$((FAILURES + 1))
}

# check <description> <stdout file> <exit code> <expected .saida> <expected .codigo>
check() {
	local desc="$1" got="$2" rc="$3" exp="$4" exp_rc="$5"
	local want_rc=0
	if [ -f "$exp_rc" ]; then
		want_rc=$(tr -d '[:space:]' <"$exp_rc")
	fi
	if [ "$rc" != "$want_rc" ]; then
		fail "$desc: código de saída $rc (esperado: $want_rc)"
		return
	fi
	got=$(norm "$got")
	exp=$(norm "$exp")
	if ! cmp -s "$got" "$exp"; then
		fail "$desc: saída diferente da esperada"
		diff "$exp" "$got" | head -20
		return
	fi
	ok "$desc"
}

# On Windows both git (autocrlf) and MinGW programs produce CRLF while the
# native binaries write LF, and gpt converts accented characters of its
# messages to the OEM code page (CP437 on the CI runner), where ã, õ and the
# accented capitals other than É do not exist and become the plain letter:
# fold those, then compare without carriage returns and non-ASCII bytes.
norm() {
	if [ $ON_WINDOWS -eq 1 ]; then
		sed 's/ã/a/g; s/õ/o/g; s/[ÁÀÂÃ]/A/g; s/Ê/E/g; s/Í/I/g; s/[ÓÔÕ]/O/g; s/Ú/U/g' <"$1" |
			tr -d '\r\200-\377' >"$TMP/$(basename "$1").lf"
		echo "$TMP/$(basename "$1").lf"
	else
		echo "$1"
	fi
}

# expected file specific to the mode, when there is one
expected_for() {
	local base="$1" ext="$2" mode="$3"
	if [ -f "$base.$ext.$mode" ]; then
		echo "$base.$ext.$mode"
	else
		echo "$base.$ext"
	fi
}

skip_mode() {
	local base="$1" mode="$2"
	[ -f "$base.pular" ] || return 1
	if [ $ON_WINDOWS -eq 1 ] && grep -qw "windows" "$base.pular"; then
		return 0
	fi
	grep -qw "$mode" "$base.pular"
}

# run_caso <.gpt file> <prefix of the expected files>
run_caso() {
	local src="$1" base="$2"
	local name
	name=$(basename "$src" .gpt)
	local dir
	dir=$(dirname "$src")
	local stdin=/dev/null
	[ -f "$base.entrada" ] && stdin="$base.entrada"
	local extra=""
	if [ -f "$base.args" ]; then
		for a in $(cat "$base.args"); do
			extra="$extra $dir/$a"
		done
	fi

	# interpretador
	if ! skip_mode "$base" "interp"; then
		$TIMEOUT "$GPT" -i "$src" $extra <"$stdin" >"$TMP/$name.i.out" 2>"$TMP/$name.i.err"
		check "$name (-i)" "$TMP/$name.i.out" $? \
			"$(expected_for "$base" saida interp)" "$(expected_for "$base" codigo interp)"
	fi

	# binário nativo
	if ! skip_mode "$base" "nativo"; then
		if [ $CAN_EXEC_X86 -eq 0 ]; then
			echo "⚠ $name (-o): pulado (gpt recusa -o na arquitetura $ARCH)"
		elif $TIMEOUT "$GPT" -o "$TMP/$name.bin" "$src" $extra >"$TMP/$name.o.build" 2>&1; then
			$TIMEOUT "$TMP/$name.bin" <"$stdin" >"$TMP/$name.o.out" 2>"$TMP/$name.o.err"
			check "$name (-o)" "$TMP/$name.o.out" $? \
				"$(expected_for "$base" saida nativo)" "$(expected_for "$base" codigo nativo)"
		else
			fail "$name (-o): compilação falhou"
			head -5 "$TMP/$name.o.build"
		fi
	fi

	# tradução para C
	if ! skip_mode "$base" "c"; then
		# Every translated program carries leia_literal(), which calls the
		# POSIX getline() that MinGW-w64 lacks, so on Windows the generated C
		# neither compiles nor links.
		if [ $ON_WINDOWS -eq 1 ] || [ $HAVE_GCC -eq 0 ]; then
			if $TIMEOUT "$GPT" -t "$TMP/$name.c" "$src" $extra >"$TMP/$name.t.build" 2>&1; then
				ok "$name (-t): tradução gerada (compilação do C pulada nesta plataforma)"
			else
				fail "$name (-t): tradução falhou"
			fi
		elif ! $TIMEOUT "$GPT" -t "$TMP/$name.c" "$src" $extra >"$TMP/$name.t.build" 2>&1; then
			fail "$name (-t): tradução falhou"
			head -5 "$TMP/$name.t.build"
		elif ! gcc -w -o "$TMP/$name.cbin" "$TMP/$name.c" >"$TMP/$name.gcc" 2>&1; then
			fail "$name (-t): C gerado não compila"
			head -5 "$TMP/$name.gcc"
		else
			$TIMEOUT "$TMP/$name.cbin" <"$stdin" >"$TMP/$name.t.out" 2>"$TMP/$name.t.err"
			check "$name (-t)" "$TMP/$name.t.out" $? \
				"$(expected_for "$base" saida c)" "$(expected_for "$base" codigo c)"
		fi
	fi
}

echo "========================================"
echo "tester.gpt"
echo "========================================"
cd "$SCRIPT_DIR" || exit 1
run_caso "$SCRIPT_DIR/tester.gpt" "$SCRIPT_DIR/tester"

if $TIMEOUT "$GPT" -s "$TMP/tester.asm" tester.gpt >/dev/null 2>&1 && [ -f "$TMP/tester.asm" ]; then
	ok "tester.gpt (-s): geração de assembly"
	if command -v nasm &>/dev/null; then
		NASM_FORMAT=bin
		[ $ON_WINDOWS -eq 1 ] && NASM_FORMAT=win32
		if nasm -O1 -f $NASM_FORMAT -o "$TMP/tester_asm_bin" "$TMP/tester.asm" >"$TMP/tester.nasm" 2>&1; then
			ok "tester.gpt (-s): montagem com NASM"
		else
			fail "tester.gpt (-s): montagem com NASM falhou"
			head -5 "$TMP/tester.nasm"
		fi
	else
		echo "⚠ tester.gpt (-s): montagem pulada (NASM não encontrado)"
	fi
else
	fail "tester.gpt (-s): geração de assembly"
fi
echo ""

echo "========================================"
echo "Casos de regressão (test/casos)"
echo "========================================"
for src in "$SCRIPT_DIR"/casos/*.gpt; do
	name=$(basename "$src" .gpt)
	case "$name" in _*) continue ;; esac
	run_caso "$src" "$SCRIPT_DIR/casos/$name"
done
echo ""

echo "========================================"
echo "Erros de compilação (test/erros)"
echo "========================================"
# run from erros/ so that messages carry only the file name
cd "$SCRIPT_DIR/erros" || exit 1
for src in *.gpt; do
	name=$(basename "$src" .gpt)
	case "$name" in _*) continue ;; esac
	$TIMEOUT "$GPT" -s "$TMP/$name.asm" "$src" </dev/null >"$TMP/$name.e.out" 2>"$TMP/$name.e.err"
	rc=$?
	if [ $rc -eq 0 ]; then
		fail "$name: programa inválido foi aceito"
	elif [ $rc -ge 128 ]; then
		fail "$name: gpt abortou (código $rc)"
		tail -3 "$TMP/$name.e.err"
	elif ! cmp -s "$(norm "$TMP/$name.e.err")" "$(norm "$name.msg")"; then
		fail "$name: mensagem diferente da esperada"
		diff "$(norm "$name.msg")" "$(norm "$TMP/$name.e.err")" | head -10
	else
		ok "$name"
	fi
done
cd "$SCRIPT_DIR" || exit 1
echo ""

echo "========================================"
echo "Recusas (código de saída 1)"
echo "========================================"
# The refusal checks grep ASCII-only fragments of gpt's diagnostics: on
# Windows, gpt converts stderr to the OEM code page before writing it.
if [ $CAN_EXEC_X86 -eq 1 ]; then
	# Hiding nasm by emptying PATH also hides the MinGW DLLs that gpt.exe
	# loads from /mingw64/bin, where HACKING.md installs nasm, so a failing
	# nasm goes first in PATH instead: cmd.exe runs the .bat, sh the script.
	mkdir -p "$TMP/nasm_falso"
	printf '#!/bin/sh\nexit 1\n' >"$TMP/nasm_falso/nasm"
	printf '@exit /b 1\r\n' >"$TMP/nasm_falso/nasm.bat"
	chmod +x "$TMP/nasm_falso/nasm"
	ERRO=$(PATH="$TMP/nasm_falso:$PATH" $TIMEOUT "$GPT" -o "$TMP/recusa.bin" tester.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 1 ] && grep -q "montar o programa com o nasm" <<<"$ERRO"; then
		ok "-o com o nasm falhando"
	else
		fail "-o com o nasm falhando: código $RESULT (esperado: 1 e o diagnóstico do gpt)"
		echo "$ERRO"
	fi
else
	ERRO=$($TIMEOUT "$GPT" -o "$TMP/recusa.bin" tester.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 1 ] && grep -q "Use -t para traduzir" <<<"$ERRO"; then
		ok "-o na arquitetura $ARCH"
	else
		fail "-o na arquitetura $ARCH: código $RESULT (esperado: 1 e o diagnóstico do gpt)"
		echo "$ERRO"
	fi
fi

cat >"$TMP/erro_sintaxe.gpt" <<'EOF'
algoritmo erro_sintaxe;

início
  isso nao e um comando
fim
EOF
ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" -i erro_sintaxe.gpt 2>&1 </dev/null >/dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && grep -q "^erro_sintaxe.gpt:4 " <<<"$ERRO"; then
	ok "-i com erro de sintaxe"
else
	fail "-i com erro de sintaxe: código $RESULT (esperado: 1 e o erro da linha 4)"
	echo "$ERRO"
fi
echo ""

echo "========================================"
echo "Resumo dos testes"
echo "========================================"
echo "$PASSED verificação(ões) passou(aram)"
if [ $FAILURES -eq 0 ]; then
	echo "✓ Todos os testes passaram!"
	exit 0
else
	echo "✗ $FAILURES verificação(ões) falhou(aram)"
	exit 1
fi
