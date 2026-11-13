#!/bin/bash
#
# GPT regression suite.
#
#   test/tester.gpt   self-checking program (must exit with code 42)
#   test/casos/       valid programs, run in the three modes:
#                       NAME.gpt            source
#                       NAME.entrada        stdin (optional)
#                       NAME.args           extra command line files, one per
#                                           line (optional, relative to casos/)
#                       NAME.saida          expected stdout (interpreter)
#                       NAME.saida.nativo   expected stdout of the native
#                                           binary, when it differs
#                       NAME.saida.c        same for the C translation
#                       NAME.codigo[.nativo|.c]  exit code (default: 0)
#                       NAME.erro[.nativo|.c]    expected stderr (optional)
#                       NAME.pular          modes to skip ("interp", "nativo",
#                                           "c"); "windows" skips the whole
#                                           case on Windows and "windows:MODE"
#                                           skips one mode there
#   test/erros/       invalid programs:
#                       NAME.gpt            source
#                       NAME.args           extra command line files, one per
#                                           line (optional, relative to erros/)
#                       NAME.msg            expected stderr of "gpt -s"
#                       NAME.pular          "windows" to skip the case on
#                                           Windows (its console cannot show
#                                           the characters in the message)
#
# Files whose name starts with "_" are helpers and are not run.
#
# The native mode builds for the machine running the tests. GPT_TEST_ARCH
# (x86-64 or arm64) builds for another one instead, and GPT_TEST_RUNNER is
# the command that runs what it builds, for example qemu-aarch64.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GPT="$SCRIPT_DIR/../src/gpt"

FAILURES=0
PASSED=0

if [ ! -f "$GPT" ]; then
	echo "ERRO: gpt binário não encontrado em $GPT"
	echo "Execute 'make' primeiro."
	exit 1
fi

TMP=$(mktemp -d "${TMPDIR:-/tmp}/gpt_test.XXXXXX") || exit 1
trap 'rm -rf "$TMP"' EXIT

# Detect architecture / platform
ARCH=$(uname -m)
HOST_ARCH=""
case "$ARCH" in
x86_64 | amd64) HOST_ARCH=x86-64 ;;
i?86)
	# Same check as gpt: an i386 chroot on an amd64 kernel says i686 only
	# under the linux32 personality, and a 32-bit kernel says it without one.
	PERSONA=0
	read -r PERSONA 2>/dev/null </proc/self/personality
	[ $((16#$PERSONA & 0xff)) -eq 8 ] && HOST_ARCH=x86-64
	;;
aarch64 | arm64) HOST_ARCH=arm64 ;;
esac
NATIVE_ARCH="$HOST_ARCH"
GPT_A=()
RUNNER=()
if [ -n "$GPT_TEST_ARCH" ]; then
	NATIVE_ARCH="$GPT_TEST_ARCH"
	GPT_A=(-a "$GPT_TEST_ARCH")
	read -r -a RUNNER <<<"$GPT_TEST_RUNNER"
fi
CAN_EXEC_NATIVE=0
[ -n "$NATIVE_ARCH" ] && CAN_EXEC_NATIVE=1

# The program gpt -o starts first: nasm, or the binutils assembler, named
# after the target when it is not the machine's own.
case "$NATIVE_ARCH" in
x86-64) ASSEMBLER=nasm ;;
arm64)
	ASSEMBLER=as
	[ "$ARCH" != aarch64 ] && [ "$ARCH" != arm64 ] && ASSEMBLER=aarch64-linux-gnu-as
	;;
esac
HAVE_ASSEMBLER=0
if [ -n "$ASSEMBLER" ] && command -v "$ASSEMBLER" &>/dev/null; then
	HAVE_ASSEMBLER=1
fi

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
#       [<stderr file> <expected .erro>]
check() {
	local desc="$1" got="$2" rc="$3" exp="$4" exp_rc="$5" got_err="$6" exp_err="$7"
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
	if [ -f "$exp_err" ]; then
		got_err=$(norm "$got_err")
		exp_err=$(norm "$exp_err")
		if ! cmp -s "$got_err" "$exp_err"; then
			fail "$desc: stderr diferente do esperado"
			diff "$exp_err" "$got_err" | head -20
			return
		fi
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
	local base="$1" mode="$2" token
	[ -f "$base.pular" ] || return 1
	for token in $(cat "$base.pular"); do
		token=${token%$'\r'}
		case "$token" in
		"$mode") return 0 ;;
		windows | "windows:$mode") [ $ON_WINDOWS -eq 1 ] && return 0 ;;
		esac
	done
	return 1
}

# read_args <.args file> <prefix>: the extra files, one per line, into ARGS.
# git checks the file out with CRLF on Windows, so a trailing \r is dropped.
read_args() {
	ARGS=()
	[ -f "$1" ] || return 0
	local a
	while IFS= read -r a || [ -n "$a" ]; do
		a=${a%$'\r'}
		[ -n "$a" ] && ARGS+=("$2$a")
	done <"$1"
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
	read_args "$base.args" "$dir/"
	local extra=("${ARGS[@]}")

	# interpretador
	if ! skip_mode "$base" "interp"; then
		$TIMEOUT "$GPT" -i "$src" "${extra[@]}" <"$stdin" >"$TMP/$name.i.out" 2>"$TMP/$name.i.err"
		check "$name (-i)" "$TMP/$name.i.out" $? \
			"$(expected_for "$base" saida interp)" "$(expected_for "$base" codigo interp)" \
			"$TMP/$name.i.err" "$(expected_for "$base" erro interp)"
	fi

	# binário nativo
	if ! skip_mode "$base" "nativo"; then
		if [ $CAN_EXEC_NATIVE -eq 0 ]; then
			echo "⚠ $name (-o): pulado (gpt recusa -o na arquitetura $ARCH)"
		elif $TIMEOUT "$GPT" "${GPT_A[@]}" -o "$TMP/$name.bin" "$src" "${extra[@]}" >"$TMP/$name.o.build" 2>&1; then
			$TIMEOUT "${RUNNER[@]}" "$TMP/$name.bin" <"$stdin" >"$TMP/$name.o.out" 2>"$TMP/$name.o.err"
			check "$name (-o)" "$TMP/$name.o.out" $? \
				"$(expected_for "$base" saida nativo)" "$(expected_for "$base" codigo nativo)" \
				"$TMP/$name.o.err" "$(expected_for "$base" erro nativo)"
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
			if $TIMEOUT "$GPT" -t "$TMP/$name.c" "$src" "${extra[@]}" >"$TMP/$name.t.build" 2>&1; then
				ok "$name (-t): tradução gerada (compilação do C pulada nesta plataforma)"
			else
				fail "$name (-t): tradução falhou"
			fi
		elif ! $TIMEOUT "$GPT" -t "$TMP/$name.c" "$src" "${extra[@]}" >"$TMP/$name.t.build" 2>&1; then
			fail "$name (-t): tradução falhou"
			head -5 "$TMP/$name.t.build"
		elif ! gcc -w -o "$TMP/$name.cbin" "$TMP/$name.c" >"$TMP/$name.gcc" 2>&1; then
			fail "$name (-t): C gerado não compila"
			head -5 "$TMP/$name.gcc"
		else
			$TIMEOUT "$TMP/$name.cbin" <"$stdin" >"$TMP/$name.t.out" 2>"$TMP/$name.t.err"
			check "$name (-t)" "$TMP/$name.t.out" $? \
				"$(expected_for "$base" saida c)" "$(expected_for "$base" codigo c)" \
				"$TMP/$name.t.err" "$(expected_for "$base" erro c)"
		fi
	fi
}

echo "========================================"
echo "tester.gpt"
echo "========================================"
cd "$SCRIPT_DIR" || exit 1
run_caso "$SCRIPT_DIR/tester.gpt" "$SCRIPT_DIR/tester"

if $TIMEOUT "$GPT" "${GPT_A[@]}" -s "$TMP/tester.asm" tester.gpt >/dev/null 2>&1 && [ -f "$TMP/tester.asm" ]; then
	ok "tester.gpt (-s): geração de assembly"
	if [ $HAVE_ASSEMBLER -eq 0 ]; then
		echo "⚠ tester.gpt (-s): montagem pulada (${ASSEMBLER:-montador} não encontrado)"
	elif if [ "$ASSEMBLER" = nasm ]; then
		nasm -O1 -fbin -o "$TMP/tester_asm_bin" "$TMP/tester.asm"
	else
		"$ASSEMBLER" -o "$TMP/tester_asm.o" "$TMP/tester.asm"
	fi >"$TMP/tester.nasm" 2>&1; then
		ok "tester.gpt (-s): montagem com $ASSEMBLER"
	else
		fail "tester.gpt (-s): montagem com $ASSEMBLER falhou"
		head -5 "$TMP/tester.nasm"
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
# A source read from a pipe cannot seek back, so looking for a byte order
# mark must not lose its first bytes. /dev/stdin is a POSIX path.
if [ $ON_WINDOWS -eq 0 ]; then
	for bom in "" $'\xEF\xBB\xBF'; do
		{ printf '%s' "$bom"; cat "$SCRIPT_DIR/casos/olamundo.gpt"; } |
			$TIMEOUT "$GPT" -i /dev/stdin >"$TMP/pipe.out" 2>&1
		check "olamundo.gpt (-i) via pipe${bom:+, com BOM}" "$TMP/pipe.out" $? \
			"$SCRIPT_DIR/casos/olamundo.saida" ""
	done
fi
# The default gnu17 leaves trigraphs alone, but -std=c11 replaces them even
# inside string literals.
if [ $ON_WINDOWS -eq 0 ] && [ $HAVE_GCC -eq 1 ]; then
	if $TIMEOUT "$GPT" -t "$TMP/trigraphs_c11.c" "$SCRIPT_DIR/casos/trigraphs.gpt" >/dev/null 2>&1 &&
		gcc -std=c11 -w -o "$TMP/trigraphs_c11" "$TMP/trigraphs_c11.c" >/dev/null 2>&1; then
		$TIMEOUT "$TMP/trigraphs_c11" </dev/null >"$TMP/trigraphs_c11.out" 2>&1
		check "trigraphs (-t) com gcc -std=c11" "$TMP/trigraphs_c11.out" $? \
			"$SCRIPT_DIR/casos/trigraphs.saida" ""
	else
		fail "trigraphs (-t) com gcc -std=c11: tradução ou compilação falhou"
	fi
fi
# Control bytes inside a string literal (the \n and \r escapes, a raw 0x1A
# and others), written here so that no editor or line-end conversion touches
# them. Windows is skipped: its C runtime stops reading a text-mode file at
# a 0x1A byte.
if [ $ON_WINDOWS -eq 0 ]; then
	printf 'algoritmo bytes_de_controle;\nin\303\255cio\n  imprima("a\\nb\\rc\032d\032e\001f\177g'"'"'h");\nfim\n' \
		>"$TMP/bytes_de_controle.gpt"
	printf 'a\nb\rc\032d\032e\001f\177g'"'"'h\n' >"$TMP/bytes_de_controle.saida"
	run_caso "$TMP/bytes_de_controle.gpt" "$TMP/bytes_de_controle"
fi
# A literal longer than the 1 MiB the native heap grows by gets a chunk of
# its own size. Written here: a file this large is refused by pre-commit.
{
	printf 'algoritmo literal_grande;\nvari\303\241veis\n  s, t, u : literal;\nfim-vari\303\241veis\nin\303\255cio\n'
	printf '  t := leia();\n  s := longo();\n  u := leia();\n  imprima(s = longo(), " ", t, " ", u);\nfim\n'
	printf 'fun\303\247\303\243o longo() : literal\nin\303\255cio\n  retorne "'
	head -c 1200000 /dev/zero | tr '\0' x
	printf '";\nfim\n'
} >"$TMP/literal_grande.gpt"
printf 'um\ndois\n' >"$TMP/literal_grande.entrada"
printf 'verdadeiro um dois\n' >"$TMP/literal_grande.saida"
run_caso "$TMP/literal_grande.gpt" "$TMP/literal_grande"
echo ""

echo "========================================"
echo "Erros de compilação (test/erros)"
echo "========================================"
# run from erros/ so that messages carry only the file name
cd "$SCRIPT_DIR/erros" || exit 1
for src in *.gpt; do
	name=$(basename "$src" .gpt)
	case "$name" in _*) continue ;; esac
	if [ $ON_WINDOWS -eq 1 ] && [ -f "$name.pular" ] && grep -qw "windows" "$name.pular"; then
		continue
	fi
	read_args "$name.args" ""
	$TIMEOUT "$GPT" -s "$TMP/$name.asm" "$src" "${ARGS[@]}" </dev/null >"$TMP/$name.e.out" 2>"$TMP/$name.e.err"
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
echo "Ajuda e versão"
echo "========================================"
SAIDA=$($TIMEOUT "$GPT" -v 2>&1 </dev/null)
RESULT=$?
if [ $RESULT -eq 0 ] && grep -q "Compilador G-Portugol" <<<"$SAIDA"; then
	ok "-v"
else
	fail "-v: código $RESULT (esperado: 0 e a versão)"
	echo "$SAIDA"
fi

SAIDA=$($TIMEOUT "$GPT" -h 2>&1 </dev/null)
RESULT=$?
if [ $RESULT -eq 0 ] && grep -q "Modo de uso" <<<"$SAIDA"; then
	ok "-h"
else
	fail "-h: código $RESULT (esperado: 0 e a ajuda)"
	echo "$SAIDA"
fi

# Without arguments gpt shows the help too, but as a usage error.
SAIDA=$($TIMEOUT "$GPT" 2>&1 </dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && grep -q "Modo de uso" <<<"$SAIDA"; then
	ok "sem argumentos"
else
	fail "sem argumentos: código $RESULT (esperado: 1 e a ajuda)"
	echo "$SAIDA"
fi
echo ""

echo "========================================"
echo "Conexão com o depurador"
echo "========================================"
# gpt resolves the host given with -H through getaddrinfo and tries every
# address it returns. The server below answers each stop with step_into.
if [ $ON_WINDOWS -eq 1 ] || ! command -v python3 &>/dev/null; then
	echo "⚠ conexão com o depurador: pulado (sem python3 ou no Windows)"
else
	cat >"$TMP/conexao.py" <<'EOF'
import socket
import sys

family = socket.AF_INET6 if sys.argv[1] == "6" else socket.AF_INET
srv = socket.socket(family)
try:
    srv.bind(("::1" if family == socket.AF_INET6 else "127.0.0.1", 0))
except OSError:
    print(0, flush=True)
    sys.exit(0)
srv.listen(1)
srv.settimeout(30)
print(srv.getsockname()[1], flush=True)
conn, _ = srv.accept()
conn.settimeout(30)
data = b""
stops = 0
while True:
    chunk = conn.recv(4096)
    if not chunk:
        break
    data += chunk
    while True:
        header = data.find(b"\0")
        if header < 0:
            break
        size = int(data[:header])
        if len(data) < header + 1 + size + 1:
            break
        if data[header + 1:header + 1 + size].startswith(b"<stackinfo>"):
            stops += 1
            cmd = b"step_into"
            conn.sendall(str(len(cmd) + 1).encode() + b"\0" + cmd + b"\0")
        data = data[header + 2 + size:]
sys.exit(0 if stops >= 2 else "esperava ao menos duas paradas, houve %d" % stops)
EOF
	cat >"$TMP/conexao_laco.gpt" <<'EOF'
algoritmo conexao_laco;
variáveis
  i : inteiro;
fim-variáveis
início
  i := 0;
  enquanto i < 2000 faça
    imprima("linha ", i);
    i := i + 1;
  fim-enquanto
fim
EOF
	# conecta_depurador <família> <host> <programa> <texto esperado>
	# Prints "pulado" when the loopback family is not available.
	conecta_depurador() {
		local familia="$1" host="$2" programa="$3" esperado="$4" servidor_pid
		coproc SRV { python3 "$TMP/conexao.py" "$familia" 2>"$TMP/conexao.err"; }
		servidor_pid=$SRV_PID
		read -r PORTA <&"${SRV[0]}"
		if [ "$PORTA" = "0" ]; then
			wait "$servidor_pid"
			echo pulado
			return 0
		fi
		SAIDA=$($TIMEOUT "$GPT" -i -H "$host" -P "$PORTA" "$programa" 2>&1 </dev/null)
		RESULT=$?
		wait "$servidor_pid"
		local servidor=$?
		if [ $RESULT -eq 0 ] && [ $servidor -eq 0 ] && grep -q "$esperado" <<<"$SAIDA" &&
			! grep -q "comando desconhecido" <<<"$SAIDA"; then
			return 0
		fi
		echo "código $RESULT, servidor $servidor (esperado: 0, 0 e a saída do programa)"
		echo "$SAIDA"
		cat "$TMP/conexao.err"
		return 1
	}
	for caso in "4 localhost IPv4" "4 127.0.0.1 IPv4" "6 ::1 IPv6"; do
		read -r familia host nome <<<"$caso"
		RESULTADO=$(conecta_depurador "$familia" "$host" casos/control.gpt "depois: 5")
		RC=$?
		if [ "$RESULTADO" = "pulado" ]; then
			echo "⚠ -H $host: pulado (sem $nome em loopback)"
		elif [ $RC -eq 0 ]; then
			ok "-H $host com um servidor $nome"
		else
			fail "-H $host com um servidor $nome"
			echo "$RESULTADO"
		fi
	done

	SAIDA=$($TIMEOUT "$GPT" -i -H nome-que-nao-existe.invalid -P 5000 casos/olamundo.gpt 2>&1 </dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && grep -q "não foi possível resolver o depurador em nome-que-nao-existe.invalid:5000" <<<"$SAIDA" &&
		grep -q "Mundo" <<<"$SAIDA"; then
		ok "-H com um host que não resolve"
	else
		fail "-H com um host que não resolve: código $RESULT (esperado: 0, o aviso e a saída do programa)"
		echo "$SAIDA"
	fi

	PORTA=$(python3 -c 'import socket; s = socket.socket(); s.bind(("127.0.0.1", 0)); print(s.getsockname()[1])')
	SAIDA=$($TIMEOUT "$GPT" -i -H 127.0.0.1 -P "$PORTA" casos/olamundo.gpt 2>&1 </dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && grep -q "não foi possível conectar ao depurador em 127.0.0.1:$PORTA" <<<"$SAIDA" &&
		grep -q "Mundo" <<<"$SAIDA"; then
		ok "-H para uma porta sem servidor"
	else
		fail "-H para uma porta sem servidor: código $RESULT (esperado: 0, o aviso e a saída do programa)"
		echo "$SAIDA"
	fi

	# A closed stdout must kill gpt with SIGPIPE as it does without -H, not
	# be swallowed by a handler that only closes the debugger socket. CI
	# runners start the job with SIGPIPE ignored and exec keeps that, so the
	# default action is restored before gpt starts.
	if [ "$(uname -s)" = Linux ]; then
		coproc SRV { python3 "$TMP/conexao.py" 4 2>/dev/null; }
		servidor_pid=$SRV_PID
		read -r PORTA <&"${SRV[0]}"
		python3 -c 'import os, signal, sys; signal.signal(signal.SIGPIPE, signal.SIG_DFL); os.execvp(sys.argv[1], sys.argv[1:])' \
			$TIMEOUT "$GPT" -i -H 127.0.0.1 -P "$PORTA" "$TMP/conexao_laco.gpt" </dev/null 2>/dev/null | head -1 >/dev/null
		RESULT=${PIPESTATUS[0]}
		wait "$servidor_pid"
		if [ $RESULT -eq 141 ]; then
			ok "-H com a saída padrão fechada"
		else
			fail "-H com a saída padrão fechada: código $RESULT (esperado: 141, SIGPIPE)"
		fi
	fi
fi
echo ""


echo "========================================"
echo "Recusas (código de saída 1)"
echo "========================================"
# The refusal checks grep ASCII-only fragments of gpt's diagnostics: on
# Windows, gpt converts stderr to the OEM code page before writing it.
if [ $CAN_EXEC_NATIVE -eq 1 ]; then
	# Hiding the assembler by emptying PATH also hides the MinGW DLLs that
	# gpt.exe loads from /ucrt64/bin, where HACKING.md installs nasm, so a
	# failing one goes first in PATH instead. gpt starts it without a shell,
	# and on Windows CreateProcess only looks for nasm.exe, so there it is a
	# program.
	mkdir -p "$TMP/nasm_falso"
	printf '#!/bin/sh\nexit 1\n' >"$TMP/nasm_falso/$ASSEMBLER"
	chmod +x "$TMP/nasm_falso/$ASSEMBLER"
	if [ $ON_WINDOWS -eq 1 ]; then
		printf 'int main(void) { return 1; }\n' >"$TMP/nasm_falso.c"
		gcc -o "$TMP/nasm_falso/nasm.exe" "$TMP/nasm_falso.c"
	fi
	ERRO=$(PATH="$TMP/nasm_falso:$PATH" $TIMEOUT "$GPT" "${GPT_A[@]}" -o "$TMP/recusa.bin" tester.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 1 ] && grep -q "montar o programa com o $ASSEMBLER" <<<"$ERRO"; then
		ok "-o com o $ASSEMBLER falhando"
	else
		fail "-o com o $ASSEMBLER falhando: código $RESULT (esperado: 1 e o diagnóstico do gpt)"
		echo "$ERRO"
	fi
	# Neither next to gpt nor in PATH, the assembler is reported missing.
	# libtool's wrapper script (--enable-install-devel) needs PATH itself.
	if [ $ON_WINDOWS -eq 0 ] && [ "$(head -c 2 "$GPT")" != "#!" ]; then
		mkdir -p "$TMP/path_vazio"
		ERRO=$($TIMEOUT env PATH="$TMP/path_vazio" "$GPT" "${GPT_A[@]}" -o "$TMP/recusa.bin" tester.gpt 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 1 ] && grep -q "encontrar o $ASSEMBLER" <<<"$ERRO"; then
			ok "-o sem o $ASSEMBLER no PATH"
		else
			fail "-o sem o $ASSEMBLER no PATH: código $RESULT (esperado: 1 e o aviso de que ele não foi encontrado)"
			echo "$ERRO"
		fi
		# A directory named after the assembler next to gpt is not taken for
		# it, so the one in PATH still builds the program.
		mkdir -p "$TMP/dir_irmao/$ASSEMBLER"
		cp "$GPT" "$TMP/dir_irmao/gpt"
		ERRO=$($TIMEOUT "$TMP/dir_irmao/gpt" "${GPT_A[@]}" -o "$TMP/dir_irmao.bin" tester.gpt 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && [ -x "$TMP/dir_irmao.bin" ]; then
			ok "-o com um diretório $ASSEMBLER ao lado do gpt"
		else
			fail "-o com um diretório $ASSEMBLER ao lado do gpt: código $RESULT (esperado: 0 e o $ASSEMBLER do PATH)"
			echo "$ERRO"
		fi
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

ERRO=$($TIMEOUT "$GPT" -a sparc -s "$TMP/recusa.s" tester.gpt 2>&1 >/dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && grep -qF '"sparc" (use x86-64 ou arm64)' <<<"$ERRO"; then
	ok "-a com uma arquitetura desconhecida"
else
	fail "-a com uma arquitetura desconhecida: código $RESULT (esperado: 1 e o diagnóstico do gpt)"
	echo "$ERRO"
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

# Each quote or "/*" after an unterminated one used to scan the rest of the
# line or file again, so these 80 KB inputs took over half a minute.
{
	printf 'algoritmo aspas;\ninício\n  imprima("'
	yes '\"' | head -n 40000 | tr -d '\n'
	printf ');\nfim\n'
} >"$TMP/aspas_abertas.gpt"
ERRO=$(cd "$TMP" && ${TIMEOUT:+timeout 10} "$GPT" -s "$TMP/aspas.s" aspas_abertas.gpt 2>&1 </dev/null >/dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && grep -q "^aspas_abertas.gpt:3 - Faltando fechar aspas" <<<"$ERRO"; then
	ok "-s com 40000 aspas sem fechar"
else
	fail "-s com 40000 aspas sem fechar: código $RESULT (esperado: 1 em menos de 10 s)"
	echo "$ERRO"
fi
{
	printf 'algoritmo comentarios;\ninício\n'
	yes '/* ' | head -n 27000 | tr -d '\n'
	printf '\nfim\n'
} >"$TMP/comentarios_abertos.gpt"
ERRO=$(cd "$TMP" && ${TIMEOUT:+timeout 10} "$GPT" -s "$TMP/comentarios.s" comentarios_abertos.gpt 2>&1 </dev/null >/dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && grep -q "^comentarios_abertos.gpt:5 - AVISO: coment" <<<"$ERRO" &&
	grep -q " iniciado na linha 3 " <<<"$ERRO"; then
	ok "-s com 27000 comentários sem fechar"
else
	fail "-s com 27000 comentários sem fechar: código $RESULT (esperado: 1 em menos de 10 s)"
	echo "$ERRO"
fi

ERRO=$(cd erros && $TIMEOUT "$GPT" -i call_var_as_func.gpt 2>&1 </dev/null >/dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && grep -q '^call_var_as_func.gpt:6 - "a" ' <<<"$ERRO"; then
	ok "-i com variável chamada como função"
else
	fail "-i com variável chamada como função: código $RESULT (esperado: 1 e o erro da linha 6)"
	echo "$ERRO"
fi

ERRO=$(cd erros && $TIMEOUT "$GPT" -d -s "$TMP/dica.asm" syn_virgula_literal.gpt 2>&1 </dev/null >/dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && grep -q 'antes de "x"\.' <<<"$ERRO"; then
	ok "-d com literal sem vírgula antes"
else
	fail "-d com literal sem vírgula antes: código $RESULT (esperado: 1 e a dica com \"x\")"
	echo "$ERRO"
fi

# Only the first 10 errors are shown, and recovering from each syntax error
# walks the whole rule stack, so these 6000 errors inside 97 nested calls
# took half a minute while the parser went on after the last error it could
# show.
{
	printf 'algoritmo erros;\nvariáveis\n  x: inteiro;\nfim-variáveis\ninício\n'
	yes "x := $(printf 'f(%.0s' $(seq 97));" | head -n 6000
	printf 'fim\n'
} >"$TMP/muitos_erros.gpt"
ERRO=$(cd "$TMP" && ${TIMEOUT:+timeout 10} "$GPT" -s "$TMP/muitos_erros.s" muitos_erros.gpt 2>&1 </dev/null >/dev/null)
RESULT=$?
if [ $RESULT -eq 1 ] && [ "$(grep -c '^muitos_erros.gpt:' <<<"$ERRO")" -eq 10 ] &&
	grep -q '^muitos_erros.gpt:15 - Esperando ' <<<"$ERRO"; then
	ok "-s com 6000 erros de sintaxe"
else
	fail "-s com 6000 erros de sintaxe: código $RESULT (esperado: 1 e 10 erros em menos de 10 s)"
	echo "$ERRO"
fi
echo ""

echo "========================================"
echo "Depurador"
echo "========================================"
# gpt connects to the host given with -H, sends "<length>\0<body>" messages
# and reads commands framed the same way. A peer that sends a header or a
# body larger than gpt accepts is dropped and the program runs to the end, as
# it does when nobody answers on the port; a peer that answers each stop with
# step_into drives the program to its normal end.
if [ $ON_WINDOWS -eq 1 ] || ! command -v python3 &>/dev/null; then
	echo "⚠ depurador: pulado (sem python3 ou no Windows)"
else
	cat >"$TMP/depurador.py" <<'EOF'
import socket
import sys
import xml.etree.ElementTree as ET

mode = sys.argv[1]
srv = socket.socket()
srv.settimeout(30)
srv.bind(("127.0.0.1", 0))
srv.listen(1)
print(srv.getsockname()[1], flush=True)
conn, _ = srv.accept()
conn.settimeout(30)


def frames():
    data = b""
    while True:
        chunk = conn.recv(4096)
        if not chunk:
            return
        data += chunk
        while True:
            header = data.find(b"\0")
            if header < 0:
                break
            size = int(data[:header])
            if len(data) < header + 1 + size + 1:
                break
            yield data[header + 1:header + 1 + size]
            data = data[header + 2 + size:]


def send(cmd):
    # gpt reads exactly the declared length and stops at the NUL, so the
    # length counts the terminator.
    conn.sendall(str(len(cmd) + 1).encode() + b"\0" + cmd + b"\0")


def first_file():
    for frame in frames():
        if frame.startswith(b"<stackinfo>"):
            return ET.fromstring(frame).find("entry").get("file").encode()


if mode == "cabecalho":
    conn.sendall(b"9" * 4000 + b"\0")
elif mode == "corpo":
    body = b"A" * 70000
    conn.sendall(str(len(body)).encode() + b"\0" + body)
elif mode in ("passos", "xml"):
    stops = 0
    seen = b""
    malformed = []
    for frame in frames():
        seen += frame
        if mode == "xml":
            try:
                ET.fromstring(frame)
            except ET.ParseError as err:
                malformed.append("%s: %r" % (err, frame))
        if frame.startswith(b"<stackinfo>"):
            stops += 1
            send(b"step_into")
    if stops < 2:
        sys.exit("esperava ao menos duas paradas, houve %d" % stops)
    if mode == "xml":
        with open(sys.argv[2], "wb") as out:
            out.write(seen)
        if malformed:
            sys.exit("XML malformado: " + malformed[0])
elif mode == "pontos":
    # The breakpoint on the loop body stops the program twice; once it is
    # removed, the program runs to the end. Removing one that was never set
    # and setting the same one again do not count toward the limit, or the
    # connection would be dropped before the first hit.
    stops = hits = 0
    for frame in frames():
        if not frame.startswith(b"<stackinfo>"):
            continue
        stops += 1
        entry = ET.fromstring(frame).find("entry")
        if stops == 1:
            name = entry.get("file").encode()
            for _ in range(1001):
                send(b'breakpoint cmd=remove file="' + name + b'" line=9')
            for _ in range(1001):
                send(b'breakpoint cmd=add file="' + name + b'" line=8')
        elif entry.get("line") == "8":
            hits += 1
            if hits == 2:
                send(b'breakpoint cmd=remove file="' + name + b'" line=8')
        send(b"continue")
    if (stops, hits) != (3, 2):
        sys.exit("esperava 3 paradas, 2 no ponto de parada; houve %d e %d" % (stops, hits))
elif mode in ("limite", "nome"):
    name = first_file()
    try:
        if mode == "nome":
            send(b'breakpoint cmd=add file="' + b"x" * 5000 + b'" line=1')
        else:
            for line in range(1001):
                send(b'breakpoint cmd=add file="' + name + b'" line=%d' % (1000 + line))
        send(b"continue")
        for frame in frames():
            pass
    except OSError:
        pass
conn.close()
EOF
	# run_depurador <modo> <programa> <texto esperado na saída>
	run_depurador() {
		local modo="$1" programa="$2" esperado="$3"
		local entrada=/dev/null
		[ -f "$TMP/depurador.$modo.entrada" ] && entrada="$TMP/depurador.$modo.entrada"
		coproc SRV { python3 "$TMP/depurador.py" "$modo" "$TMP/depurador.$modo.xml" 2>"$TMP/depurador.$modo.err"; }
		read -r PORTA <&"${SRV[0]}"
		SAIDA=$($TIMEOUT "$GPT" -i -H 127.0.0.1 -P "$PORTA" "$programa" 2>&1 <"$entrada")
		RESULT=$?
		wait "$SRV_PID"
		local servidor=$?
		if [ $RESULT -eq 0 ] && [ $servidor -eq 0 ] && grep -q "$esperado" <<<"$SAIDA" &&
			! grep -q "comando desconhecido" <<<"$SAIDA"; then
			return 0
		fi
		echo "código $RESULT, servidor $servidor (esperado: 0, 0 e a saída do programa sem comando desconhecido)"
		echo "$SAIDA"
		cat "$TMP/depurador.$modo.err"
		return 1
	}
	for modo in cabecalho corpo; do
		if run_depurador "$modo" casos/olamundo.gpt "Mundo" >"$TMP/depurador.$modo.out"; then
			ok "-H com $modo maior que o buffer"
		else
			fail "-H com $modo maior que o buffer"
			cat "$TMP/depurador.$modo.out"
		fi
	done
	if run_depurador passos casos/control.gpt "depois: 5" >"$TMP/depurador.passos.out"; then
		ok "-H com um host que avança passo a passo"
	else
		fail "-H com um host que avança passo a passo"
		cat "$TMP/depurador.passos.out"
	fi

	cat >"$TMP/depurador_laco.gpt" <<'EOF'
algoritmo depurador_laco;
variáveis
  i : inteiro;
fim-variáveis
início
  i := 0;
  enquanto i < 5 faça
    i := i + 1;
  fim-enquanto
  imprima("fim ", i);
fim
EOF
	if run_depurador pontos "$TMP/depurador_laco.gpt" "fim 5" >"$TMP/depurador.pontos.out"; then
		ok "-H com um ponto de parada repetido, adicionado e removido"
	else
		fail "-H com um ponto de parada repetido, adicionado e removido"
		cat "$TMP/depurador.pontos.out"
	fi
	# A peer that sets more breakpoints, or longer file names, than any
	# editor would is dropped like one that sends a malformed message.
	for caso in "limite:mais de 1000 pontos de parada" "nome:nome de arquivo longo demais"; do
		modo=${caso%%:*}
		if run_depurador "$modo" casos/control.gpt "${caso#*:}" >"$TMP/depurador.$modo.out"; then
			ok "-H com ${caso#*:}"
		else
			fail "-H com ${caso#*:}"
			cat "$TMP/depurador.$modo.out"
		fi
	done

	# A value with a quote must not close the XML attribute carrying it, and
	# a control character or a byte that is not UTF-8 typed into leia() is not
	# allowed in XML at all, and every message has to parse as XML.
	printf 'x\033y\351z\357\277\276w\n' >"$TMP/depurador.xml.entrada"
	if run_depurador xml casos/_depurador_xml.gpt "fim" >"$TMP/depurador.xml.out"; then
		ok "-H com o programa que exercita o escape do XML"
	else
		fail "-H com o programa que exercita o escape do XML"
		cat "$TMP/depurador.xml.out"
	fi
	# check_xml <descrição> <expressão esperada no XML> <atributo a mostrar>
	check_xml() {
		if grep -aq "$2" "$TMP/depurador.xml.xml"; then
			ok "-H com $1"
		else
			fail "-H com $1 (esperado no XML: $2)"
			grep -a -o "<var name=\"$3\"[^>]*>" "$TMP/depurador.xml.xml" | tail -1
		fi
	}
	check_xml "aspas no valor de uma variável" 'name="s"[^>]*value="a&quot; &lt;b&gt;"' s
	check_xml "caractere de controle e bytes inválidos no valor de uma variável" 'name="t"[^>]*value="x�y�z�w"' t
	check_xml "UTF-8 válido no valor de uma variável" 'name="v"[^>]*value="ação"' v
	check_xml "e comercial e tabulação no valor de uma variável" 'name="u"[^>]*value="a&amp;b&#9;c"' u
	check_xml "aspas num elemento de matriz" '<var name="m"[^>]*><values><var name="0" primitive="true" value="&quot;"/>' m
fi
echo ""

echo "========================================"
echo "Sinal durante a montagem"
echo "========================================"
# A gpt -o stopped by a signal while the assembler runs stops the assembler,
# removes its temporary files from TMPDIR and dies of that same signal. The
# slow assembler below goes first in PATH and records its pid when it
# starts. Background jobs of a script ignore SIGINT, so it is not tried.
if [ $CAN_EXEC_NATIVE -eq 0 ] || [ $ON_WINDOWS -eq 1 ]; then
	echo "⚠ sinal durante a montagem: pulado (sem backend nativo ou no Windows)"
else
	mkdir -p "$TMP/montador_lento"
	printf '#!/bin/sh\necho $$ >"%s"\nexec sleep 30\n' "$TMP/montador.pid" >"$TMP/montador_lento/$ASSEMBLER"
	chmod +x "$TMP/montador_lento/$ASSEMBLER"
	for sinal in TERM HUP; do
		mkdir -p "$TMP/tmpdir_$sinal"
		: >"$TMP/montador.pid"
		PATH="$TMP/montador_lento:$PATH" TMPDIR="$TMP/tmpdir_$sinal" \
			"$GPT" "${GPT_A[@]}" -o "$TMP/sinal.bin" casos/olamundo.gpt >/dev/null 2>&1 &
		GPT_PID=$!
		for _ in $(seq 100); do
			[ -s "$TMP/montador.pid" ] && break
			sleep 0.1
		done
		MONTADOR=$(cat "$TMP/montador.pid")
		kill -"$sinal" $GPT_PID
		# bash reports a job killed by SIGHUP on stderr
		{ wait $GPT_PID; } 2>/dev/null
		RESULT=$?
		ESPERADO=$((128 + $(kill -l "$sinal")))
		SOBRAS=$(ls -A "$TMP/tmpdir_$sinal" | tr '\n' ' ')
		if [ -n "$MONTADOR" ] && [ $RESULT -eq $ESPERADO ] && [ -z "$SOBRAS" ] && ! kill -0 "$MONTADOR" 2>/dev/null; then
			ok "SIG$sinal durante a montagem: montador parado e TMPDIR vazio"
		else
			fail "SIG$sinal durante a montagem: código $RESULT, sobras: ${SOBRAS:-nenhuma} (esperado: $ESPERADO, o montador parado e TMPDIR vazio)"
			[ -n "$MONTADOR" ] && kill "$MONTADOR" 2>/dev/null
		fi
	done
fi
echo ""

echo "========================================"
echo "nasm ao lado do gpt"
echo "========================================"
# gpt -o runs the nasm in its own directory before the one in PATH. Copies of
# gpt and of the real nasm go to a directory with a space in its name and the
# failing nasm stays first in PATH, so only the copy next to gpt can work.
NASM_REAL=$(command -v nasm)
if [ "$NATIVE_ARCH" != x86-64 ] || [ -z "$NASM_REAL" ]; then
	echo "⚠ -o com o nasm ao lado do gpt: pulado (sem x86 ou sem NASM)"
else
	EXE=""
	[ $ON_WINDOWS -eq 1 ] && EXE=".exe"
	[ -f "$NASM_REAL.exe" ] && NASM_REAL="$NASM_REAL.exe"
	IRMAO="$TMP/nasm irmao"
	mkdir -p "$IRMAO"
	# Under --enable-install-devel src/gpt is libtool's wrapper script, which
	# only works from src/: the program is src/.libs/gpt and finds the
	# uninstalled libgportugol through LD_LIBRARY_PATH.
	GPT_PROG="$GPT$EXE"
	GPT_LIBS=""
	if [ "$(head -c 2 "$GPT")" = "#!" ]; then
		GPT_PROG="$SCRIPT_DIR/../src/.libs/gpt"
		GPT_LIBS="$SCRIPT_DIR/../src/modules/.libs"
	fi
	cp "$GPT_PROG" "$IRMAO/gpt$EXE"
	cp "$NASM_REAL" "$IRMAO/nasm$EXE"
	ERRO=$(
		[ -n "$GPT_LIBS" ] && export LD_LIBRARY_PATH="$GPT_LIBS${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
		PATH="$TMP/nasm_falso:$PATH" $TIMEOUT "$IRMAO/gpt$EXE" -o "$IRMAO/olamundo$EXE" casos/olamundo.gpt 2>&1 >/dev/null
	)
	RESULT=$?
	if [ $RESULT -eq 0 ] && "$IRMAO/olamundo$EXE" | grep -q "Mundo"; then
		ok "-o com o nasm ao lado do gpt, num diretório com espaço"
	else
		fail "-o com o nasm ao lado do gpt: código $RESULT (esperado: 0 e o programa montado)"
		echo "$ERRO"
	fi
	# With a bare name, CreateProcess tries the current directory before PATH.
	# The failing nasm.exe sits in the current directory here, and only the
	# nasm in PATH can build the program.
	if [ $ON_WINDOWS -eq 1 ]; then
		ERRO=$(cd "$TMP/nasm_falso" && $TIMEOUT "$GPT" -o "$TMP/plantado.exe" "$SCRIPT_DIR/casos/olamundo.gpt" 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && "$TMP/plantado.exe" | grep -q "Mundo"; then
			ok "-o com um nasm.exe no diretório atual usa o do PATH"
		else
			fail "-o com um nasm.exe no diretório atual: código $RESULT (esperado: 0 e o programa montado pelo nasm do PATH)"
			echo "$ERRO"
		fi
	fi
fi
echo ""

echo "========================================"
echo "Nome de saída com caracteres especiais"
echo "========================================"
# The name given to -o reaches the assembler, the linker and chmod as it is,
# never through a shell: nothing in it may run, and the program must be
# written under that name. GNU ld reads "@resposta" as the options in the
# file "resposta", here an output name that would leave the marker, and
# rewrites "-lsaida" to "--library=saida".
if [ $CAN_EXEC_NATIVE -eq 0 ] || [ $HAVE_ASSEMBLER -eq 0 ]; then
	echo "⚠ -o com caracteres especiais: pulado (sem backend nativo ou sem ${ASSEMBLER:-montador})"
else
	cp casos/olamundo.gpt "$TMP/especial.gpt"
	echo marcador >"$TMP/resposta"
	for nome in 'saida$(touch marcador)' 'saida`touch marcador`' \
		'saida; touch marcador' 'saida com espaco' 'saida%OS%' \
		'@resposta' '-lsaida'; do
		rm -f "$TMP/marcador"
		ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" "${GPT_A[@]}" -o "$nome" especial.gpt 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && [ -f "$TMP/$nome" ] && [ ! -e "$TMP/marcador" ]; then
			ok "-o \"$nome\""
		else
			fail "-o \"$nome\": código $RESULT (esperado: 0, o programa com esse nome e nenhum comando executado)"
			echo "$ERRO"
		fi
	done
	# The temporary files reach the tools too, under a TMPDIR that gpt reads
	# only outside Windows. ld looks for an object under "=tmp" or
	# "$SYSROOTtmp" in its sysroot.
	if [ $ON_WINDOWS -eq 0 ]; then
		for dir in -tmp @tmp =tmp '$SYSROOTtmp'; do
			mkdir -p "$TMP/$dir"
			rm -f "$TMP/saida_tmpdir"
			ERRO=$(cd "$TMP" && TMPDIR="$dir" $TIMEOUT "$GPT" "${GPT_A[@]}" -o saida_tmpdir especial.gpt 2>&1 >/dev/null)
			RESULT=$?
			if [ $RESULT -eq 0 ] && "${RUNNER[@]}" "$TMP/saida_tmpdir" | grep -q "Mundo"; then
				ok "-o com TMPDIR=$dir"
			else
				fail "-o com TMPDIR=$dir: código $RESULT (esperado: 0 e o programa montado)"
				echo "$ERRO"
			fi
		done
	fi

	# Without -o the program takes the name of the source file, not the name
	# written inside it, so a source cannot choose which file it replaces. A
	# source without an extension gets one instead of being overwritten.
	EXE=""
	SEMEXT="semext.out"
	if [ $ON_WINDOWS -eq 1 ]; then
		EXE=".exe"
		SEMEXT="semext.exe"
	fi
	cp casos/olamundo.gpt "$TMP/semext"
	rm -f "$TMP/especial$EXE" "$TMP/$SEMEXT" "$TMP/olamundo$EXE"
	ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" "${GPT_A[@]}" especial.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && [ ! -e "$TMP/olamundo$EXE" ] && "${RUNNER[@]}" "$TMP/especial$EXE" | grep -q "Mundo"; then
		ok "sem -o: programa com o nome do arquivo fonte"
	else
		fail "sem -o: código $RESULT (esperado: 0 e o programa em especial$EXE)"
		echo "$ERRO"
	fi
	cp casos/olamundo.gpt "$TMP/@resposta.gpt"
	rm -f "$TMP/@resposta$EXE" "$TMP/marcador"
	ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" "${GPT_A[@]}" @resposta.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && [ ! -e "$TMP/marcador" ] && "${RUNNER[@]}" "$TMP/@resposta$EXE" | grep -q "Mundo"; then
		ok "sem -o: fonte com nome começando com @"
	else
		fail "sem -o com fonte @resposta.gpt: código $RESULT (esperado: 0 e o programa em @resposta$EXE)"
		echo "$ERRO"
	fi
	ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" "${GPT_A[@]}" semext 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && cmp -s "$TMP/semext" casos/olamundo.gpt && "${RUNNER[@]}" "$TMP/$SEMEXT" | grep -q "Mundo"; then
		ok "sem -o: fonte sem extensão continua intacto"
	else
		fail "sem -o com fonte sem extensão: código $RESULT (esperado: 0, o fonte intacto e o programa em $SEMEXT)"
		echo "$ERRO"
	fi
	# Outside Windows a backslash is an ordinary character in a file name.
	if [ $ON_WINDOWS -eq 0 ]; then
		cp casos/olamundo.gpt "$TMP/aula\\prog.gpt"
		rm -f "$TMP/prog" "$TMP/aula\\prog"
		ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" "${GPT_A[@]}" 'aula\prog.gpt' 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && [ ! -e "$TMP/prog" ] && "${RUNNER[@]}" "$TMP/aula\\prog" | grep -q "Mundo"; then
			ok "sem -o: barra invertida no nome do fonte"
		else
			fail "sem -o com barra invertida: código $RESULT (esperado: 0 e o programa em aula\\prog)"
			echo "$ERRO"
		fi
	fi
	if [ $ON_WINDOWS -eq 1 ]; then
		# On Windows the caller appends .exe, so a source already named *.exe
		# must not be replaced by its own program.
		cp casos/olamundo.gpt "$TMP/semexe.exe"
		rm -f "$TMP/semexe.out.exe"
		ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" semexe.exe 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && cmp -s "$TMP/semexe.exe" casos/olamundo.gpt && "$TMP/semexe.out.exe" | grep -q "Mundo"; then
			ok "sem -o: fonte .exe no Windows não é sobrescrito"
		else
			fail "sem -o com fonte .exe: código $RESULT (esperado: 0, o fonte intacto e o programa em semexe.out.exe)"
			echo "$ERRO"
		fi
	fi
fi

# An empty name for -s or -t is an error, not a request for the default name.
cp casos/olamundo.gpt "$TMP/vazio.gpt"
rm -f "$TMP/vazio" "$TMP/vazio.out"
for opcao in -s -t; do
	ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" "$opcao" "" vazio.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -ne 0 ] && [ ! -e "$TMP/vazio" ] && [ ! -e "$TMP/vazio.out" ]; then
		ok "$opcao com nome vazio é rejeitado"
	else
		fail "$opcao com nome vazio: código $RESULT (esperado: erro e nenhum arquivo criado)"
		echo "$ERRO"
	fi
done
echo ""

echo "========================================"
echo "Saída igual a uma entrada"
echo "========================================"
# gpt refuses to write over one of its input files, whatever name leads to
# it, before writing anything. GPT_INCLUDE files are inputs too.
cp casos/olamundo.gpt "$TMP/proprio.gpt"
echo "// funções extras" >"$TMP/extra.gpt"
ln -f "$TMP/proprio.gpt" "$TMP/duro.gpt"
ENTRADAS=(proprio.gpt extra.gpt)
CASOS_SAIDA=("-s proprio.gpt proprio.gpt" "-t ./proprio.gpt proprio.gpt"
	"-s duro.gpt proprio.gpt" "-t extra.gpt proprio.gpt extra.gpt"
	"GPT_INCLUDE=extra.gpt -s extra.gpt proprio.gpt")
if [ $ON_WINDOWS -eq 0 ]; then
	ln -sf proprio.gpt "$TMP/link.gpt"
	CASOS_SAIDA+=("-t link.gpt proprio.gpt")
fi
if [ $CAN_EXEC_NATIVE -eq 1 ]; then
	# Without -o the program of proprio.gpt is called proprio (proprio.exe on
	# Windows), here also the name of the second input.
	PROPRIO=proprio
	[ $ON_WINDOWS -eq 1 ] && PROPRIO=proprio.exe
	cp "$TMP/extra.gpt" "$TMP/$PROPRIO"
	ENTRADAS+=("$PROPRIO")
	CASOS_SAIDA+=("-o proprio.gpt proprio.gpt" "proprio.gpt $PROPRIO")
fi
for f in "${ENTRADAS[@]}"; do
	cp "$TMP/$f" "$TMP/$f.orig"
done
for caso in "${CASOS_SAIDA[@]}"; do
	# cp writes into the existing file, so duro.gpt stays a hard link
	for f in "${ENTRADAS[@]}"; do
		cp "$TMP/$f.orig" "$TMP/$f"
	done
	read -r -a ARGS <<<"$caso"
	AMBIENTE=()
	if [[ ${ARGS[0]} == *=* ]]; then
		AMBIENTE=("${ARGS[0]}")
		ARGS=("${ARGS[@]:1}")
	fi
	ERRO=$(cd "$TMP" && env "${AMBIENTE[@]}" $TIMEOUT "$GPT" "${GPT_A[@]}" "${ARGS[@]}" 2>&1 >/dev/null)
	RESULT=$?
	MUDOU=""
	for f in "${ENTRADAS[@]}"; do
		cmp -s "$TMP/$f" "$TMP/$f.orig" || MUDOU="$MUDOU $f"
	done
	if [ $RESULT -eq 1 ] && [ -z "$MUDOU" ] && grep -q "um dos arquivos de entrada" <<<"$ERRO"; then
		ok "gpt $caso: recusado"
	else
		fail "gpt $caso: código $RESULT, alterados:${MUDOU:- nenhum} (esperado: 1, a recusa e nenhuma entrada alterada)"
		echo "$ERRO"
	fi
done
echo ""

echo "========================================"
echo "Variáveis globais"
echo "========================================"
# Global variables start zeroed, so they take no room in the executable,
# where the 2000000 slots of globais_grandes would need 16 MB.
if [ $CAN_EXEC_NATIVE -eq 0 ] || [ $HAVE_ASSEMBLER -eq 0 ]; then
	echo "⚠ tamanho do executável: pulado (sem backend nativo ou sem ${ASSEMBLER:-montador})"
elif ! $TIMEOUT "$GPT" "${GPT_A[@]}" -o "$TMP/globais.bin" casos/globais_grandes.gpt >"$TMP/globais.build" 2>&1; then
	fail "tamanho do executável: compilação falhou"
	head -5 "$TMP/globais.build"
else
	TAMANHO=$(wc -c <"$TMP/globais.bin")
	if [ "$TAMANHO" -lt 1048576 ]; then
		ok "matriz global fora do executável ($TAMANHO bytes)"
	else
		fail "matriz global fora do executável: $TAMANHO bytes (esperado: menos de 1 MiB)"
	fi
fi
echo ""

echo "========================================"
echo "Tamanho das matrizes"
echo "========================================"
# A matrix takes up to 2^26 elements in every mode. A native executable
# also takes up to 1 GiB of global variables and 1 GiB of variables in each
# function: two such matrices fill either, and one variable more is refused.
cat >"$TMP/teto.gpt" <<'EOF'
algoritmo teto;
variáveis
  m : matriz[8192][8192] de inteiros;
fim-variáveis
início
  m[8191][8191] := 5;
  imprima(m[8191][8191] + m[0][0]);
fim
EOF
SAIDA=$($TIMEOUT "$GPT" -i "$TMP/teto.gpt" 2>&1 </dev/null)
RESULT=$?
if [ $RESULT -eq 0 ] && [ "$SAIDA" = 5 ]; then
	ok "matriz com 2^26 elementos (-i)"
else
	fail "matriz com 2^26 elementos (-i): código $RESULT (esperado: 0 e 5)"
	echo "$SAIDA"
fi
if [ $CAN_EXEC_NATIVE -eq 0 ]; then
	echo "⚠ 1 GiB de variáveis (-s): pulado (gpt recusa -s na arquitetura $ARCH)"
else
	for escopo in globais locais; do
		for extra in "" "c : inteiro;"; do
			if [ $escopo = globais ]; then
				printf 'algoritmo teto;\nvariáveis\n  a, b : matriz[67108864] de inteiros;\n  %s\nfim-variáveis\ninício\nfim\n' "$extra"
			else
				printf 'algoritmo teto;\ninício\n  f();\nfim\nfunção f()\n  a, b : matriz[67108864] de inteiros;\n  %s\ninício\nfim\n' "$extra"
			fi >"$TMP/teto_$escopo.gpt"
			ERRO=$($TIMEOUT "$GPT" "${GPT_A[@]}" -s "$TMP/teto_$escopo.asm" "$TMP/teto_$escopo.gpt" 2>&1 </dev/null)
			RESULT=$?
			if [ -z "$extra" ]; then
				if [ $RESULT -eq 0 ]; then
					ok "1 GiB de variáveis $escopo (-s)"
				else
					fail "1 GiB de variáveis $escopo (-s): código $RESULT (esperado: 0)"
					echo "$ERRO"
				fi
			elif [ $RESULT -eq 1 ] && grep -q "ocupam mais de 1 GiB" <<<"$ERRO"; then
				ok "mais de 1 GiB de variáveis $escopo é recusado (-s)"
			else
				fail "mais de 1 GiB de variáveis $escopo (-s): código $RESULT (esperado: 1 e o limite)"
				echo "$ERRO"
			fi
		done
	done
fi
echo ""

echo "========================================"
echo "Segmentos do executável"
echo "========================================"
# The generated ELF must declare a non-executable stack and keep the code
# and the writable data in separate segments on separate pages, of 4 KiB on
# x86-64 and of up to 64 KiB on arm64, whose kernels may use larger pages.
if [ $CAN_EXEC_NATIVE -eq 0 ] || [ $ON_WINDOWS -eq 1 ] || [ $HAVE_ASSEMBLER -eq 0 ] || ! command -v readelf &>/dev/null; then
	echo "⚠ segmentos do executável: pulado (sem backend nativo, no Windows, sem ${ASSEMBLER:-montador} ou sem readelf)"
elif ! $TIMEOUT "$GPT" "${GPT_A[@]}" -o "$TMP/segmentos.bin" casos/olamundo.gpt >"$TMP/segmentos.build" 2>&1; then
	fail "segmentos do executável: compilação falhou"
	head -5 "$TMP/segmentos.build"
else
	SEGS=$(readelf -lW "$TMP/segmentos.bin" 2>/dev/null)
	if grep -qE '^\s*GNU_STACK\s.*\sRW\s' <<<"$SEGS"; then
		ok "PT_GNU_STACK sem execução"
	else
		fail "PT_GNU_STACK sem execução: cabeçalho ausente ou executável"
		echo "$SEGS"
	fi
	PAGINA=4096
	[ "$NATIVE_ARCH" = arm64 ] && PAGINA=65536
	read -r _ _ CODIGO _ _ CODIGO_TAM _ <<<"$(grep -E '^\s*LOAD\s.*\sR E\s' <<<"$SEGS")"
	read -r _ _ DADOS _ <<<"$(grep -E '^\s*LOAD\s.*\sRW\s' <<<"$SEGS")"
	if [ -n "$CODIGO" ] && [ -n "$DADOS" ] &&
		[ $(((CODIGO + CODIGO_TAM - 1) / PAGINA)) -lt $((DADOS / PAGINA)) ]; then
		ok "código R E e dados RW em páginas distintas"
	else
		fail "código R E e dados RW em páginas distintas"
		echo "$SEGS"
	fi
fi
echo ""

echo "========================================"
echo "Permissões do executável"
echo "========================================"
# Like ld, gpt -o makes only a regular file executable: /dev/null keeps its
# mode even when the suite runs as root. The program written through a
# symbolic link, or over a file without read permission, must still run.
if [ $CAN_EXEC_NATIVE -eq 0 ] || [ $ON_WINDOWS -eq 1 ] || [ $HAVE_ASSEMBLER -eq 0 ]; then
	echo "⚠ permissões do executável: pulado (sem backend nativo, no Windows ou sem ${ASSEMBLER:-montador})"
else
	ANTES=$(ls -l /dev/null | cut -c1-10)
	ERRO=$($TIMEOUT "$GPT" "${GPT_A[@]}" -o /dev/null casos/olamundo.gpt 2>&1 >/dev/null)
	RESULT=$?
	DEPOIS=$(ls -l /dev/null | cut -c1-10)
	if [ $RESULT -eq 0 ] && [ "$ANTES" = "$DEPOIS" ]; then
		ok "-o /dev/null mantém o modo do dispositivo"
	else
		fail "-o /dev/null: código $RESULT, modo $ANTES virou $DEPOIS (esperado: 0 e o mesmo modo)"
		echo "$ERRO"
	fi

	# The program is written through the link, except that ld replaces the
	# link with a new file when its target is not empty.
	: >"$TMP/alvo_do_link"
	ln -s alvo_do_link "$TMP/link_de_saida"
	ERRO=$($TIMEOUT "$GPT" "${GPT_A[@]}" -o "$TMP/link_de_saida" casos/olamundo.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && "${RUNNER[@]}" "$TMP/link_de_saida" | grep -q "Mundo"; then
		ok "-o com um link simbólico gera um programa executável"
	else
		fail "-o com um link simbólico: código $RESULT (esperado: 0 e o programa executável pelo link)"
		echo "$ERRO"
	fi

	# ld refuses to open an empty output it cannot read, so only nasm gets
	# here.
	if [ "$NATIVE_ARCH" = x86-64 ]; then
		: >"$TMP/so_escrita"
		chmod 200 "$TMP/so_escrita"
		ERRO=$($TIMEOUT "$GPT" "${GPT_A[@]}" -o "$TMP/so_escrita" casos/olamundo.gpt 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && [ -x "$TMP/so_escrita" ]; then
			ok "-o sobre um arquivo sem permissão de leitura"
		else
			fail "-o sobre um arquivo sem permissão de leitura: código $RESULT (esperado: 0 e o arquivo executável)"
			echo "$ERRO"
		fi
	fi
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
