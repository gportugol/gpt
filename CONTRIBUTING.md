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

O projeto requer ANTLR 4.x (ferramenta `antlr4` e runtime C++), PCRE2,
Autotools, um compilador C++ e NASM. O script de testes executa os programas de
`test/casos/` no interpretador, como binário nativo e como tradução para C,
comparando as saídas com as esperadas, e verifica as mensagens de erro dos
programas de `test/erros/`. Em máquinas que não são x86, a execução do binário
nativo é pulada.

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

### Gramática ANTLR

O lexer, o parser e os visitors em `src/modules/parser/` são gerados durante o
build a partir de `Portugol.g4`. Para alterar a sintaxe, edite a gramática, não
os arquivos gerados (`PortugolLexer.cpp`, `PortugolParser.cpp`,
`PortugolVisitor.cpp`, `PortugolBaseVisitor.cpp`). A análise semântica, o
interpretador, o gerador x86 e o tradutor para C são classes C++ que percorrem
a árvore sintática gerada.

Após mudar a gramática, execute um build que force a regeneração e teste todos
os modos de execução afetados.

### Testes e exemplos

Adicione casos de regressão em `test/casos/` (programa `.gpt`, entrada
`.entrada` opcional, saída esperada `.saida` e código de saída `.codigo`) ou em
`test/erros/` (programa `.gpt` e mensagem esperada `.msg`), ou em
`test/tester.gpt` quando for adequado. Para alterações que afetem o
comportamento da linguagem, cubra tanto o comportamento esperado quanto o caso
que antes falhava.

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
