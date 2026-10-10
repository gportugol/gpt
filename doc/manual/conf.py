import re
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
# The repository has no .gitignore, so keep _ext/ free of __pycache__.
sys.dont_write_bytecode = True
sys.path.insert(0, str(HERE / "_ext"))


def read_version():
    configure_ac = (HERE.parents[1] / "configure.ac").read_text(encoding="utf-8")
    return re.search(r"AC_INIT\(\[gpt\],\s*\[([^\]]+)\]\)", configure_ac).group(1)


project = "G-Portugol"
author = "Thiago Silva e G-Portugol contributors"
copyright = "2003-2009 Thiago Silva, 2026 G-Portugol contributors"
release = read_version()
version = release

# Read the Docs passes its project language as -D language=..., which wins
# over this value, so the project there must be set to Portuguese (Brazil).
language = "pt_BR"
extensions = ["myst_parser", "portugol_lexer"]
myst_enable_extensions = ["deflist"]
exclude_patterns = ["_build"]

numfig = True
numfig_format = {
    "code-block": "Programa %s",
    "table": "Tabela %s",
}

html_theme = "sphinx_rtd_theme"
html_title = f"Manual do G-Portugol {release}"

latex_documents = [
    ("index", "manual.tex", "G-Portugol", author, "manual"),
]
latex_appendices = ["gramatica"]
latex_elements = {
    "papersize": "a4paper",
    "pointsize": "11pt",
    "releasename": "Versão",
}

epub_basename = "manual"
