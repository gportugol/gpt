# G-Portugol

This README is also available in English.
You can access it here: [English version](README.en.md)

## Sobre a linguagem

G-Portugol é um dialeto da linguagem/pseudo-código **Portugol** (ou portugês
estruturado), que é muito usada para descrever algoritmos em português, de forma
livre e espontânea. Em geral, livros dedicados ao ensino de algoritmos, lógica e
estruturas de dados utilizam alguma forma dessa linguagem.

A proposta de G-Portugol é disponibilizar uma implementação da linguagem
portugol, fornecendo ferramentas que ofereçam recursos de edição, compilação,
execução e depuração de programas escritos nessa linguagem, de forma a favorecer
estudantes que dão os primeiros passos no aprendizado de desenvolvimento de
softwares, bem como professores que ensinam disciplinas relacionadas a
computação. Portanto, seu foco é primariamente didático.

Se encontram disponíveis atualmente um compilador, tradutor e interpretador para
a linguagem (GPT) e um ambiente visual simples
([GPTEditor](https://github.com/gportugol/gpteditor)) que permite a edição,
execução e depuração de programas escritos em G-Portugol. O primeiro é
multi-plataforma e capaz de gerar executáveis para MS Windows e sistemas
GNU/Linux. Já o segundo está disponível apenas para o ambiente KDE 3.0 do
GNU/Linux.

## O programa GPT

GPT é o programa que implementa a linguagem G-Portugol, permitindo:

- compilar algoritmos;
- traduzir algoritmos para C;
- executar algoritmos de forma interpretada;
- depurar algoritmos interativamente.

Embora se encontre usável, o GPT não está imune a bugs. Além do mais, alguns
recursos podem estar faltando. Portanto, convido-lhes a participar, enviando
sugestões, críticas, códigos-fonte, patches, ideias e algoritmos que não são
processados corretamente pelo GPT.

## Disponível no Debian e no Ubuntu

O **GPT** está disponível nos repositórios [oficiais do
Debian](https://tracker.debian.org/pkg/gpt) e nos repositórios [oficias do
Ubuntu](https://launchpad.net/ubuntu/+source/gpt). Isso permite que sua
instalação seja feita de forma simples usando o gerenciador de pacotes **APT**.

Para instalar o GPT, execute:

```bash
sudo apt install gpt
```

Após a instalação, o comando `gpt` estará disponível no terminal, permitindo
compilar, traduzir, executar e depurar programas escritos em G-Portugol.

```bash
gpt -i /usr/share/doc/gpt/examples/olamundo.gpt
```

## Manual do G-Portugol

O manual do G-Portugol, com a sintaxe da linguagem, exemplos de programas, o uso
do programa `gpt` e a gramática, está publicado no Read the Docs. Ele é
atualizado a cada release e pode ser baixado em PDF e ePub.

👉 Acesse aqui: [Manual do G-Portugol](https://gportugol.readthedocs.io/)

## Referências e citações sobre o G-Portugol

Quer saber mais sobre publicações, palestras e materiais relacionados ao
G-Portugol? Essas informações estão reunidas na wiki do projeto:

👉 [Produção bibliográfica](https://github.com/gportugol/gpt/wiki/Refer%C3%AAncias-e-cita%C3%A7%C3%B5es-sobre-o-G%E2%80%90Portugol)

## Licença

Distribuído sob os termos da [GNU General Public License v2](COPYING).
