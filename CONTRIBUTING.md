# Contribuindo para o G-Portugol

Obrigado por considerar contribuir com o G-Portugol. São bem-vindas correções,
melhorias na linguagem e no compilador, testes, exemplos e documentação.

## Antes de começar

- Pesquise issues e pull requests existentes para evitar trabalho duplicado.
- Para uma dúvida, ideia ampla ou proposta de mudança de linguagem, abra uma
  issue antes de investir em uma implementação grande.
- Para uma vulnerabilidade, siga a [política de segurança](SECURITY.md); não a
  reporte em uma issue pública.

## Ambiente de desenvolvimento

As dependências e as instruções completas para GNU/Linux e Windows/MSYS2 estão
em [HACKING.md](HACKING.md). Em GNU/Linux, o fluxo usual é:

```bash
git clone https://github.com/gportugol/gpt.git
cd gpt
autoreconf -i
./configure --prefix=/usr/local
make -j$(nproc)
bash test/run_test.sh
```

O projeto requer ANTLR 2.x, PCRE2, Autotools, um compilador C++ e NASM. O
script de testes verifica interpretação, compilação nativa, geração de assembly
e, quando possível, montagem com NASM. Em máquinas que não são x86, a execução
do binário nativo é pulada.

## Fazendo uma alteração

1. Crie uma branch a partir da versão atual do repositório.
2. Faça uma alteração pequena e focada, preservando o estilo do código ao redor.
3. Acrescente ou atualize testes que demonstrem a correção ou o recurso novo.
4. Execute as verificações locais descritas abaixo.
5. Envie um pull request com uma descrição clara.

Não inclua binários, diretórios de build, arquivos temporários ou outros
artefatos gerados no pull request.

### Código C++

- O código-fonte deve permanecer compatível com GNU/Linux e Windows/MSYS2.
- Preserve os avisos de licença/copyright existentes nos arquivos modificados.
- Use UTF-8 para fontes, exemplos e documentação.
- A configuração de pre-commit aplica `clang-format` a arquivos C/C++ e verifica
  espaços finais, conflitos de merge e arquivos grandes. Execute, quando
  disponível:

  ```bash
  pre-commit run --all-files
  ```

### Gramáticas ANTLR

Os arquivos em `src/modules/parser/`, `interpreter/`, `c_translator/` e `x86`
incluem fontes geradas durante o build. Para alterar a sintaxe ou os walkers,
edite as gramáticas `.g` correspondentes, não os arquivos gerados como
`PortugolLexer.cpp`, `PortugolParser.cpp`, `SemanticWalker.cpp`,
`InterpreterWalker.cpp`, `Portugol2CWalker.cpp` ou `X86Walker.cpp`.

Após mudar uma gramática, execute um build que force a regeneração e teste todos
os modos de execução afetados. O lexer gera tipos de token consumidos pelos
módulos de tradução, interpretação e x86.

### Testes e exemplos

Adicione casos de regressão em `test/tester.gpt` quando for adequado, ou crie um
teste específico em `test/`. Para alterações que afetem o comportamento da
linguagem, cubra tanto o comportamento esperado quanto o caso que antes falhava.

Você pode testar manualmente um exemplo compilado:

```bash
src/gpt -i exemplos/olamundo.gpt
src/gpt -o olamundo exemplos/olamundo.gpt && ./olamundo
```

Remova o executável temporário depois do teste.

### Documentação

Atualize a documentação junto com mudanças visíveis aos usuários:

- opções de linha de comando: `README.md`, manual e páginas man aplicáveis;
- sintaxe ou semântica: manual em `doc/manual/` e exemplos, se necessário;
- instruções de build: `HACKING.md` e workflows de CI, quando aplicável.

## Pull requests

Na descrição do pull request, informe:

- o problema resolvido e a abordagem adotada;
- impactos de compatibilidade ou mudanças de comportamento;
- testes e comandos de validação executados;
- limitações de ambiente, se houver, como ausência de ANTLR ou NASM.

Mantenha o pull request focado; separe refatorações não relacionadas. O CI
executa build e testes em Linux e Windows, além das verificações de pre-commit.
Responda aos comentários de revisão e atualize o patch quando necessário.

## Licença

Ao enviar uma contribuição, você concorda que ela será distribuída sob a
[GNU General Public License v2](COPYING), licença do projeto.
