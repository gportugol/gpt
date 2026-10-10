import re

from pygments.lexer import RegexLexer, words
from pygments.token import (
    Comment,
    Keyword,
    Name,
    Number,
    Operator,
    Punctuation,
    String,
    Whitespace,
)

# Keep in sync with the tokens{} section of src/modules/parser/lexer.g.
BLOCK_ENDS = ("fim-variáveis", "fim-enquanto", "fim-para", "fim-se")
TYPES = (
    "inteiro",
    "real",
    "caractere",
    "literal",
    "lógico",
    "inteiros",
    "reais",
    "caracteres",
    "literais",
    "lógicos",
    "matriz",
)
KEYWORDS = (
    "algoritmo",
    "variáveis",
    "início",
    "fim",
    "se",
    "então",
    "senão",
    "enquanto",
    "faça",
    "para",
    "de",
    "até",
    "passo",
    "repita",
    "função",
    "retorne",
)


class PortugolLexer(RegexLexer):
    name = "G-Portugol"
    aliases = ["portugol", "gpt"]
    filenames = ["*.gpt"]
    flags = re.MULTILINE | re.DOTALL

    tokens = {
        "root": [
            (r"\s+", Whitespace),
            (r"//.*?$", Comment.Single),
            (r"/\*.*?\*/", Comment.Multiline),
            (r'"(\\.|[^"\\\n])*"', String),
            (r"'(\\.|[^'\\])?'", String.Char),
            (r"0[xX][0-9a-fA-F]+", Number.Hex),
            (r"0[cC][0-7]+", Number.Oct),
            (r"0[bB][01]+", Number.Bin),
            (r"\d+\.\d+", Number.Float),
            (r"\d+", Number.Integer),
            (words(BLOCK_ENDS, suffix=r"\b"), Keyword),
            (words(TYPES, suffix=r"\b"), Keyword.Type),
            (words(("verdadeiro", "falso"), suffix=r"\b"), Keyword.Constant),
            (words(("ou", "e", "não"), suffix=r"\b"), Operator.Word),
            (words(KEYWORDS, suffix=r"\b"), Keyword),
            (words(("imprima", "leia"), suffix=r"\b"), Name.Builtin),
            (r"[a-zA-Z_]\w*", Name),
            (r":=|<>|>=|<=|&&|\|\||[-+*/%=<>&|^~]", Operator),
            (r"[()\[\];:,]", Punctuation),
        ],
    }


def setup(app):
    app.add_lexer("portugol", PortugolLexer)
    return {"parallel_read_safe": True, "parallel_write_safe": True}
