# G-Portugol

Este arquivo também está disponível em português.
Você pode acessá-lo aqui: [Versão em Português](README.md)

## About the Language

G-Portugol is a dialect of the **Portugol** pseudo-code language (or structured
Portuguese), which is widely used to describe algorithms in Portuguese in a
simple and natural way. In general, books dedicated to teaching algorithms,
logic, and data structures use some form of this language.

The purpose of G-Portugol is to provide an implementation of the Portugol
language, offering tools for editing, compiling, running, and debugging programs
written in this language. It is designed to support students taking their first
steps in software development, as well as teachers of computer science–related
courses. Therefore, its focus is primarily educational.

Currently available are a compiler, translator, and interpreter for the language
(**GPT**), as well as a simple visual environment
([GPTEditor](https://github.com/gportugol/gpteditor)) that allows editing,
execution, and debugging of programs written in G-Portugol. The first is
multi-platform and can generate executables for MS Windows and GNU/Linux
systems, while the second is available only for the KDE 3.0 environment on
GNU/Linux.

## The GPT Program

**GPT** is the program that implements the G-Portugol language, allowing users to:

- compile algorithms;
- translate algorithms into C;
- execute algorithms in interpreted mode;
- debug algorithms interactively.

Although usable, GPT is not immune to bugs. In addition, some features may still
be missing. Therefore, contributions are welcome — feel free to send
suggestions, feedback, source code, patches, ideas, and algorithms that are not
being processed correctly by GPT.

## Available on Debian and Ubuntu

**GPT** is available in the [official Debian
repositories](https://tracker.debian.org/pkg/gpt) and in the [official Ubuntu
repositories](https://launchpad.net/ubuntu/+source/gpt). This allows easy
installation using the **APT** package manager.

To install GPT, run:

```bash
sudo apt install gpt
```

After installation, the `gpt` command will be available in the terminal, allowing
you to compile, translate, run, and debug programs written in G-Portugol.

```bash
gpt -i /usr/share/doc/gpt/examples/olamundo.gpt
```

## G-Portugol Manual

The G-Portugol manual, written in Portuguese, covers the language syntax,
example programs, how to use the `gpt` program, and the grammar. It is published
on Read the Docs: the link below opens the `latest` version, built from
`master`, and the manual of the most recent release is the `stable` version.
Both can be downloaded as PDF or ePub.

👉 Access it here: [G-Portugol Manual](https://gportugol.readthedocs.io/)

## References and Citations about G-Portugol

Interested in learning more about publications, talks, and materials related to
G-Portugol? This information is gathered in the project’s wiki:

👉 [Bibliographic Production](https://github.com/gportugol/gpt/wiki/Refer%C3%AAncias-e-cita%C3%A7%C3%B5es-sobre-o-G%E2%80%90Portugol)

## License

Distributed under the terms of the [GNU General Public License v2](COPYING).
