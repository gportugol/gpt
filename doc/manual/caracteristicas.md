(cap-caracteristicas)=

# Características Gerais

A linguagem em si não difere fundamentalmente ou apresenta novidades em relação
ao uso popular de portugol. Semelhante a linguagens como Pascal e C, é uma
linguagem imperativa, com comandos de controle de fluxo, manipulação de dados e
recursos de entrada e saída básicos. A linguagem é *case sensitive*, o que
significa que, por exemplo, uma função chamada "leia" é diferente de uma função
chamada "Leia".

## Tipos de dados

Tipos primitivos como inteiro e literal são suportados. Literais são tipos
integrais, e não, tipos construídos pelo usuário. A
{numref}`tb-tipos-primitivos` apresenta os tipos com respectivos exemplos de
dados:

```{table} Tipos de dados primitivos
:name: tb-tipos-primitivos

| Tipo      | Exemplos       |
| --------- | -------------- |
| inteiro   | `19`           |
| real      | `2.5`          |
| caractere | `'a'`          |
| literal   | `"uma frase"`  |
| lógico    | `verdadeiro`   |
```

Com esses tipos, pode-se criar conjuntos como vetores ou matrizes "n"
dimensionais. Tipos mais complexos não são suportados.

## Estruturas de controle

O conjunto de estruturas de controle são os mais primitivos. Uma estrutura
condicional (se/senão) e três estruturas de repetição (enquanto/repita/para) são
suportadas. Embora a estrutura "para" seja uma especialização de um laço simples
como o "enquanto", ela foi implementada, visto que uma variedade de livros e
muitos professores os discutem.

```{table} Estruturas de controle
:name: tb-estruturas-controle

| Estrutura         | Tipo        |
| ----------------- | ----------- |
| se/então/senão    | condicional |
| enquanto          | repetição   |
| repita            | repetição   |
| para              | repetição   |
```

## Subprogramas (funções)

Subprogramas são como funções em C. Podem receber qualquer número de parâmetros,
sejam tipos primitivos ou vetores/matrizes, e podem retornar apenas valores
primitivos. Entretanto, não é permitido declarar funções aninhadas ou funções
com parâmetros variáveis. Vale ressaltar que passagem de parâmetros é sempre
feita por *valor*.
