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
if $GPT -s tester.asm tester.gpt && [ -f tester.asm ]; then
	echo "✓ Geração de assembly OK"
	# Montar o assembly gerado é um teste de verdade, e não um aviso.
	#
	# A versão anterior tratava a falha do NASM como "pode ser normal em
	# ARM". Não é: o `nasm -fbin` MONTA x86 em qualquer arquitetura
	# hospedeira, porque não precisa executar o resultado. Medido neste
	# mesmo script em aarch64, com o master: monta sem erro.
	#
	# Executar o binário é outra coisa, e essa sim continua pulada fora de
	# x86, logo acima. A distinção entre montar e executar é justamente o
	# que faltava.
	if command -v nasm &>/dev/null; then
		if nasm -O1 -fbin -o tester_asm_bin tester.asm 2>/dev/null; then
			echo "✓ Assembly com NASM OK"
			rm -f tester_asm_bin
		else
			echo "✗ NASM FALHOU ao montar o assembly gerado"
			FAILURES=$((FAILURES + 1))
		fi
	else
		# Ausência de ferramenta não é defeito do compilador.
		echo "⚠ Pulando montagem (NASM não encontrado)"
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
if $GPT -t tester.c tester.gpt && [ -f tester.c ]; then
	echo "✓ Tradução para C OK"
	rm -f tester.c
else
	# Antes isto era um aviso com a nota "(ANTLR4 migration pending)", e
	# FAILURES não era incrementado: o tradutor podia quebrar por completo
	# com o CI verde. Medido: no master a tradução para C FUNCIONA, então
	# a nota estava desatualizada além de silenciar a falha.
	echo "✗ Tradução para C FALHOU"
	FAILURES=$((FAILURES + 1))
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
