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
		if [ $CAN_EXEC_X86 -eq 0 ]; then
			echo "⚠ $name (-o): pulado (gpt recusa -o na arquitetura $ARCH)"
		elif $TIMEOUT "$GPT" -o "$TMP/$name.bin" "$src" "${extra[@]}" >"$TMP/$name.o.build" 2>&1; then
			$TIMEOUT "$TMP/$name.bin" <"$stdin" >"$TMP/$name.o.out" 2>"$TMP/$name.o.err"
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
echo ""

echo "========================================"
echo "Erros de compilação (test/erros)"
echo "========================================"
# run from erros/ so that messages carry only the file name
cd "$SCRIPT_DIR/erros" || exit 1
for src in *.gpt; do
	name=$(basename "$src" .gpt)
	case "$name" in _*) continue ;; esac
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
if [ $CAN_EXEC_X86 -eq 1 ]; then
	# Hiding nasm by emptying PATH also hides the MinGW DLLs that gpt.exe
	# loads from /ucrt64/bin, where HACKING.md installs nasm, so a failing
	# nasm goes first in PATH instead. gpt starts nasm without a shell, and on
	# Windows CreateProcess only looks for nasm.exe, so there it is a program.
	mkdir -p "$TMP/nasm_falso"
	printf '#!/bin/sh\nexit 1\n' >"$TMP/nasm_falso/nasm"
	chmod +x "$TMP/nasm_falso/nasm"
	if [ $ON_WINDOWS -eq 1 ]; then
		printf 'int main(void) { return 1; }\n' >"$TMP/nasm_falso.c"
		gcc -o "$TMP/nasm_falso/nasm.exe" "$TMP/nasm_falso.c"
	fi
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
echo "nasm ao lado do gpt"
echo "========================================"
# gpt -o runs the nasm in its own directory before the one in PATH. Copies of
# gpt and of the real nasm go to a directory with a space in its name and the
# failing nasm stays first in PATH, so only the copy next to gpt can work.
NASM_REAL=$(command -v nasm)
if [ $CAN_EXEC_X86 -eq 0 ] || [ -z "$NASM_REAL" ]; then
	echo "⚠ -o com o nasm ao lado do gpt: pulado (sem x86 ou sem NASM)"
else
	EXE=""
	[ $ON_WINDOWS -eq 1 ] && EXE=".exe"
	[ -f "$NASM_REAL.exe" ] && NASM_REAL="$NASM_REAL.exe"
	IRMAO="$TMP/nasm irmao"
	mkdir -p "$IRMAO"
	cp "$GPT$EXE" "$IRMAO/gpt$EXE"
	cp "$NASM_REAL" "$IRMAO/nasm$EXE"
	ERRO=$(PATH="$TMP/nasm_falso:$PATH" $TIMEOUT "$IRMAO/gpt$EXE" -o "$IRMAO/olamundo$EXE" casos/olamundo.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && "$IRMAO/olamundo$EXE" | grep -q "Mundo"; then
		ok "-o com o nasm ao lado do gpt, num diretório com espaço"
	else
		fail "-o com o nasm ao lado do gpt: código $RESULT (esperado: 0 e o programa montado)"
		echo "$ERRO"
	fi
fi
echo ""

echo "========================================"
echo "Nome de saída com caracteres especiais"
echo "========================================"
# The name given to -o reaches nasm and chmod as it is, never through a shell:
# nothing in it may run, and the program must be written under that name.
if [ $CAN_EXEC_X86 -eq 0 ] || [ -z "$NASM_REAL" ]; then
	echo "⚠ -o com caracteres especiais: pulado (sem x86 ou sem NASM)"
else
	cp casos/olamundo.gpt "$TMP/especial.gpt"
	for nome in 'saida$(touch marcador)' 'saida`touch marcador`' \
		'saida; touch marcador' 'saida com espaco' 'saida%OS%'; do
		rm -f "$TMP/marcador"
		ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" -o "$nome" especial.gpt 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && [ -f "$TMP/$nome" ] && [ ! -e "$TMP/marcador" ]; then
			ok "-o \"$nome\""
		else
			fail "-o \"$nome\": código $RESULT (esperado: 0, o programa com esse nome e nenhum comando executado)"
			echo "$ERRO"
		fi
	done

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
	ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" especial.gpt 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && [ ! -e "$TMP/olamundo$EXE" ] && "$TMP/especial$EXE" | grep -q "Mundo"; then
		ok "sem -o: programa com o nome do arquivo fonte"
	else
		fail "sem -o: código $RESULT (esperado: 0 e o programa em especial$EXE)"
		echo "$ERRO"
	fi
	ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" semext 2>&1 >/dev/null)
	RESULT=$?
	if [ $RESULT -eq 0 ] && cmp -s "$TMP/semext" casos/olamundo.gpt && "$TMP/$SEMEXT" | grep -q "Mundo"; then
		ok "sem -o: fonte sem extensão continua intacto"
	else
		fail "sem -o com fonte sem extensão: código $RESULT (esperado: 0, o fonte intacto e o programa em $SEMEXT)"
		echo "$ERRO"
	fi
	# Outside Windows a backslash is an ordinary character in a file name.
	if [ $ON_WINDOWS -eq 0 ]; then
		cp casos/olamundo.gpt "$TMP/aula\\prog.gpt"
		rm -f "$TMP/prog" "$TMP/aula\\prog"
		ERRO=$(cd "$TMP" && $TIMEOUT "$GPT" 'aula\prog.gpt' 2>&1 >/dev/null)
		RESULT=$?
		if [ $RESULT -eq 0 ] && [ ! -e "$TMP/prog" ] && "$TMP/aula\\prog" | grep -q "Mundo"; then
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
echo "Segmentos do executável"
echo "========================================"
# The generated ELF must declare a non-executable stack and keep the code
# and the writable data in separate segments on separate pages.
if [ $CAN_EXEC_X86 -eq 0 ] || [ $ON_WINDOWS -eq 1 ] || [ -z "$NASM_REAL" ] || ! command -v readelf &>/dev/null; then
	echo "⚠ segmentos do executável: pulado (sem x86, no Windows, sem NASM ou sem readelf)"
elif ! $TIMEOUT "$GPT" -o "$TMP/segmentos.bin" casos/olamundo.gpt >"$TMP/segmentos.build" 2>&1; then
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
	if grep -qE '^\s*LOAD\s.*\sR E\s' <<<"$SEGS" && grep -qE '^\s*LOAD\s+0x[0-9a-f]+ 0x[0-9a-f]+000 .*\sRW\s' <<<"$SEGS"; then
		ok "código R E e dados RW em páginas distintas"
	else
		fail "código R E e dados RW em páginas distintas"
		echo "$SEGS"
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
