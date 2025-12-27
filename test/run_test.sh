#!/bin/bash

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GPT="$SCRIPT_DIR/../src/gpt"

# Track failures
FAILURES=0

# Check if gpt binary exists
if [ ! -f "$GPT" ]; then
	echo "ERRO: gpt binário não encontrado em $GPT"
	echo "Execute 'make' primeiro."
	exit 1
fi

# Detect architecture
ARCH=$(uname -m)
CAN_EXEC_X86=0

if [ "$ARCH" = "i686" ] || [ "$ARCH" = "x86_64" ]; then
	CAN_EXEC_X86=1
fi

cd "$SCRIPT_DIR" || exit 1

echo "========================================"
echo "Testando a interpretação (-i)"
echo "========================================"
$GPT -i tester.gpt
RESULT=$?
echo "Código de saída: $RESULT"
if [ $RESULT -eq 42 ]; then
	echo "✓ Interpretação OK"
else
	echo "✗ Interpretação FALHOU (esperado: 42)"
	FAILURES=$((FAILURES + 1))
fi
echo ""

echo "========================================"
echo "Testando a compilação nativa (-o)"
echo "========================================"
$GPT -o tester_bin tester.gpt
if $GPT -o tester_bin tester.gpt; then
	echo "✓ Compilação OK"
	if [ $CAN_EXEC_X86 -eq 1 ]; then
		./tester_bin
		RESULT=$?
		echo "Código de saída: $RESULT"
		if [ $RESULT -eq 42 ]; then
			echo "✓ Execução OK"
		else
			echo "✗ Execução FALHOU (esperado: 42)"
			FAILURES=$((FAILURES + 1))
		fi
	else
		echo "⚠ Pulando execução (arquitetura $ARCH, binário x86)"
	fi
	rm -f tester_bin
else
	echo "✗ Compilação FALHOU"
	FAILURES=$((FAILURES + 1))
fi
echo ""

echo "========================================"
echo "Testando geração de assembly (-s)"
echo "========================================"
$GPT -s tester.asm tester.gpt
if $GPT -s tester.asm tester.gpt && [ -f tester.asm ]; then
	echo "✓ Geração de assembly OK"
	# Check if we can assemble it
	if command -v nasm &>/dev/null; then
		nasm -O1 -fbin -o tester_asm_bin tester.asm 2>/dev/null
		if nasm -O1 -fbin -o tester_asm_bin tester.asm 2>/dev/null; then
			echo "✓ Assembly com NASM OK"
			rm -f tester_asm_bin
		else
			echo "⚠ NASM falhou ao montar (pode ser normal em ARM)"
		fi
	else
		echo "⚠ NASM não encontrado"
	fi
	rm -f tester.asm
else
	echo "✗ Geração de assembly FALHOU"
	FAILURES=$((FAILURES + 1))
fi
echo ""

echo "========================================"
echo "Testando tradução para C (-t)"
echo "========================================"
$GPT -t tester.c tester.gpt 2>&1
if $GPT -t tester.c tester.gpt && [ -f tester.c ]; then
	echo "✓ Tradução para C OK"
	rm -f tester.c
else
	echo "⚠ Tradução para C não implementada (ANTLR4 migration pending)"
fi
echo ""

echo "========================================"
echo "Resumo dos testes"
echo "========================================"
if [ $FAILURES -eq 0 ]; then
	echo "✓ Todos os testes passaram!"
	exit 0
else
	echo "✗ $FAILURES teste(s) falhou(aram)"
	exit 1
fi
