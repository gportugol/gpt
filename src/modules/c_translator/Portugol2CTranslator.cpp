/*
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 */

#include "Portugol2CTranslator.hpp"
#include "SemanticAnalyzer.hpp"

#include <cstdlib>
#include <iostream>

using namespace std;
using antlr4::ParserRuleContext;
using antlr4::tree::TerminalNode;

Portugol2CTranslator::Portugol2CTranslator(SymbolTable &st)
    : _stable(st), _currentScopeType(-1),
      _currentScope(SymbolTable::GlobalScope) {}

void Portugol2CTranslator::indent() { _indent += "   "; }

void Portugol2CTranslator::unindent() {
  _indent = _indent.substr(0, _indent.length() - 3);
}

void Portugol2CTranslator::init(const string &name) {
  stringstream s;
  s << "/* algoritmo " << name << " */\n\n";
  s << "#define _GNU_SOURCE\n"; // nescessario para evitar gcc:warning em
                                // getline()
  s << "#include <stdio.h>\n";
  s << "#include <string.h>\n";
  s << "#include <stdarg.h>\n";
  s << "#include <stdlib.h>\n\n";
  s << "typedef short int boolean;\n";
  s << "#ifndef TRUE\n";
  s << " #define TRUE 1\n";
  s << "#endif\n";
  s << "#ifndef FALSE\n";
  s << " #define FALSE 0\n";
  s << "#endif\n\n";
  s << "int idx = 0;\n"
       "char** allocated = NULL;\n"
       "void collect(char* str) {\n"
       "  allocated = (char**) realloc((void*)allocated, "
       "sizeof(char**)*(idx+1));\n"
       "  if(!allocated) {\n"
       "    fprintf(stderr, \"Erro ao alocar memória. Abordando...\\n\");\n"
       "  }\n"
       "  allocated[idx++] = str;\n"
       "}\n";
  s << "void cleanup() {\n"
       "  int i;\n"
       "  for(i = 0; i < idx; i++) {\n"
       "    free(allocated[i]);\n"
       "  }\n"
       "  free(allocated);\n"
       "}\n\n";
  s << "void matrix_cpy(void *src, void* dest, int type, int size) {\n"
       "   int i;\n"
       "   int *ds,*dd;\n"
       "   double *fs,*fd;\n"
       "   char *cs,*cd;\n"
       "   char **css,**cdd;\n"
       "   boolean *bs,*bd;\n"
       "   switch(type) {\n"
       "     case 'i':\n"
       "       ds = (int*) src;\n"
       "       dd = (int*) dest;\n"
       "       for(i = 0; i < size; i++) dd[i] = ds[i];\n"
       "       break;\n"
       "     case 'f':\n"
       "       fs = (double*) src;\n"
       "       fd = (double*) dest;\n"
       "       for(i = 0; i < size; i++) fd[i] = fs[i];\n"
       "       break;\n"
       "     case 'c':\n"
       "       cs = (char*) src;\n"
       "       cd = (char*) dest;\n"
       "       for(i = 0; i < size; i++) cd[i] = cs[i];\n"
       "       break;\n"
       "     case 's':\n"
       "       css = (char**) src;\n"
       "       cdd = (char**) dest;\n"
       "       for(i = 0; i < size; i++) cdd[i] = css[i];\n"
       "       break;\n"
       "     case 'b':\n"
       "       bs = (boolean*) src;\n"
       "       bd = (boolean*) dest;\n"
       "       for(i = 0; i < size; i++) bd[i] = bs[i];\n"
       "       break;\n"
       "     default:\n"
       "       fprintf(stderr, \"bug: tipo nao suportado: %c\\n\", type);\n"
       "       exit(1);\n"
       "   }\n"
       "}\n";
  s << "void matrix_init(void *matrix, int type, int size) {\n"
       "   int i;\n"
       "   int *d;\n"
       "   double* f;\n"
       "   char* c;\n"
       "   char** s;\n"
       "   boolean* b;\n"
       "   switch(type) {\n"
       "     case 'i':\n"
       "       d = (int*) matrix;\n"
       "       for(i = 0; i < size; i++) d[i] = 0;\n"
       "       break;\n"
       "     case 'f':\n"
       "       f = (double*) matrix;\n"
       "       for(i = 0; i < size; i++) f[i] = 0;\n"
       "       break;\n"
       "     case 'c':\n"
       "       c = (char*) matrix;\n"
       "       for(i = 0; i < size; i++) c[i] = 0;\n"
       "       break;\n"
       "     case 's':\n"
       "       s = (char**) matrix;\n"
       "       for(i = 0; i < size; i++) s[i] = 0;\n"
       "       break;\n"
       "     case 'b':\n"
       "       b = (boolean*) matrix;\n"
       "       for(i = 0; i < size; i++) b[i] = 0;\n"
       "       break;\n"
       "     default:\n"
       "       fprintf(stderr, \"bug: tipo nao suportado: %c\\n\", type);\n"
       "       exit(1);\n"
       "   }\n"
       "}\n";
  s << "void imprima(char* format, ...) {\n"
       "   va_list args;\n"
       "   va_start(args, format);\n"
       "   int d;\n"
       "   double f;\n"
       "   int c;\n"
       "   char* s;\n"
       "   int b;\n"
       "   while(*format) {\n"
       "     switch(*format) {\n"
       "       case 'd':\n"
       "         d = va_arg(args, int);\n"
       "         printf(\"%d\", d); \n"
       "         break;\n"
       "       case 'f':\n"
       "         f = va_arg(args, double);\n"
       "         printf(\"%.2f\", f);\n"
       "         break;\n"
       "       case 'c':\n"
       "         c = va_arg(args, int);\n"
       "         printf(\"%c\", c);\n"
       "         break;\n"
       "       case 's':\n"
       "         s = va_arg(args, char*);\n"
       "         if(!s) {\n"
       "           printf(\"(nulo)\");\n"
       "         } else {\n"
       "           printf(\"%s\", s);\n"
       "         }\n"
       "         break;\n"
       "       case 'b':\n"
       "         b = va_arg(args, int);\n"
       "         if(b) {\n"
       "           printf(\"verdadeiro\");\n"
       "         } else {\n"
       "           printf(\"falso\");\n"
       "         }\n"
       "         break;\n"
       "       default:\n"
       "         fprintf(stderr, \"bug: modificador nao suportado: %c\\n\", "
       "*format);\n"
       "         exit(1);\n"
       "     }\n"
       "     format++;\n"
       "   }\n"
       "   va_end(args);\n"
       "   printf(\"\\n\");\n"
       "}\n\n";
  s << "int leia_inteiro() {\n"
       "   int i = 0;\n"
       "   scanf(\"%d\", &i);\n"
       "   return i;\n"
       "}\n";
  s << "char leia_caractere() {\n"
       "   char c = 0;\n"
       "   scanf(\"%c\", &c);\n"
       "   return c;\n"
       "}\n";
  s << "double leia_real() {\n"
       "   double f = 0;\n"
       "   scanf(\"%lf\", &f);\n"
       "   return f;\n"
       "}\n";
  s << "char* leia_literal() {\n"
       "   char *lit = NULL;\n"
       "   size_t  len = 0;\n"
       "   int read;\n"
       "   if((read = getline(&lit, &len, stdin)) == -1) {\n"
       "     fprintf(stderr, \"Erro ao ler dados da entrada\\n\");\n"
       "     exit(1);\n"
       "   }\n"
       "   lit[strlen(lit)-1] = 0;\n"
       "   collect(lit);\n"
       "   return lit;\n"
       "}\n";
  s << "boolean leia_logico() {\n"
       "   char* logico;\n"
       "   logico = leia_literal();\n"
       "   if(strcmp(\"falso\",logico) == 0) {\n"
       "      return FALSE;\n"
       "   } else if(strcmp(\"0\",logico) == 0) {\n"
       "      return FALSE;\n"
       "   }\n"
       "   return TRUE;\n"
       "}\n";
  s << "int str_strlen(char* str) {\n"
       "   if(str == 0) {\n"
       "     return 0;\n"
       "   }\n"
       "   return strlen(str);\n"
       "}\n";
  s << "boolean str_comp(char* left, char* right) {\n"
       "   if (!left && !right) {\n"
       "      return TRUE;\n"
       "   }\n"
       "   if (!left || !right) {\n"
       "      return FALSE;\n"
       "   }\n"
       "   if(str_strlen(left) != str_strlen(right)) {\n"
       "     return FALSE;\n"
       "   }\n"
       "   if((str_strlen(left)==0) && (str_strlen(right)==0)) {\n"
       "     return TRUE;\n"
       "   }\n"
       "   return (strcmp(left, right)==0);\n"
       "}\n";
  s << "char* return_literal(char* str) {\n"
       "  char* lit = NULL;\n"
       "  if(!str) {\n"
       "    return NULL;\n"
       "  }\n"
       "  lit = (char*) malloc(sizeof(char)*(str_strlen(str)+1));\n"
       "  strcpy(lit, str);\n"
       "  collect(lit);\n"
       "  return lit;\n"
       "}\n\n";

  _head << s.str();

  _currentScope = SymbolTable::GlobalScope;
}

void Portugol2CTranslator::setScope(const string &scope) {
  _currentScope = scope;
}

void Portugol2CTranslator::addPrototype(const string &str) {
  _head << str << endl;
}

void Portugol2CTranslator::writeln(const string &str, bool doIndent) {
  if (doIndent)
    _txt << _indent;
  _txt << str << endl;
}

void Portugol2CTranslator::write(const string &str, bool doIndent) {
  if (doIndent)
    _txt << _indent;
  _txt << str;
}

void Portugol2CTranslator::writeInitStms() {
  _txt << _scope_init_stms.str();
  _scope_init_stms.str("");
}

void Portugol2CTranslator::addInitStm(const string &s) {
  _scope_init_stms << "   " << s << "\n";
}

string Portugol2CTranslator::translateFunctionName(const string &id, int type) {
  Symbol s = _stable.getSymbol(SymbolTable::GlobalScope, id);
  if (s.isBuiltin) {
    if (s.lexeme == "leia") {
      switch (type) {
      case TIPO_REAL:
        return "leia_real";
      case TIPO_LITERAL:
        return "leia_literal";
      case TIPO_CARACTERE:
        return "leia_caractere";
      case TIPO_LOGICO:
        return "leia_logico";
      case TIPO_INTEIRO:
      default:
        return "leia_inteiro";
      }
    } else { // imprima
      return id;
    }
  } else {
    string ret = "_";
    ret += id;
    return ret;
  }
}

string Portugol2CTranslator::translateFunctionParams(const string &id,
                                                     list<Production> &params) {
  Symbol s = _stable.getSymbol(SymbolTable::GlobalScope, id);

  if (s.isBuiltin && (id == "imprima")) {
    stringstream first_arg;
    first_arg << "\"";
    for (list<Production>::iterator it = params.begin(); it != params.end();
         ++it) {
      switch ((*it).first) {
      case TIPO_INTEIRO:
        first_arg << "d";
        break;
      case TIPO_REAL:
        first_arg << "f";
        break;
      case TIPO_CARACTERE:
        first_arg << "c";
        break;
      case TIPO_LITERAL:
        first_arg << "s";
        break;
      case TIPO_LOGICO:
        first_arg << "b";
        break;
      }
    }
    first_arg << "\"";
    params.push_front(Production(TIPO_LITERAL, first_arg.str()));
  }

  stringstream ret;
  string v;
  for (list<Production>::iterator it = params.begin(); it != params.end();
       ++it) {
    ret << v << (*it).second;
    v = ", ";
  }
  return ret.str();
}

string Portugol2CTranslator::translateType(int type) {
  string str;
  switch (type) {
  case TIPO_NULO:
    str = "void";
    break;
  case TIPO_INTEIRO:
    str = "int";
    break;
  case TIPO_REAL:
    str = "double";
    break;
  case TIPO_CARACTERE:
    str = "char";
    break;
  case TIPO_LITERAL:
    str = "char*";
    break;
  case TIPO_LOGICO:
    str = "boolean";
    break;
  default:
    cerr << "Erro interno: tipo nao suportado (pt2c::translateType)." << endl;
    exit(1);
  }
  return str;
}

string Portugol2CTranslator::translateBinExpr(const Production &left,
                                              const Production &right,
                                              size_t optoken) {
  stringstream ret;
  if ((left.first != TIPO_LITERAL) && (right.first != TIPO_LITERAL)) {
    switch (optoken) {
    case PortugolParser::T_IGUAL:
      ret << left.second << "==" << right.second;
      break;
    case PortugolParser::T_DIFERENTE:
      ret << left.second << "!=" << right.second;
      break;
    case PortugolParser::T_MAIOR:
      ret << left.second << ">" << right.second;
      break;
    case PortugolParser::T_MENOR:
      ret << left.second << "<" << right.second;
      break;
    case PortugolParser::T_MAIOR_EQ:
      ret << left.second << ">=" << right.second;
      break;
    case PortugolParser::T_MENOR_EQ:
      ret << left.second << "<=" << right.second;
      break;
    default:
      cerr << "Erro interno: op nao suportado (pt2c::translateBinExpr)."
           << endl;
      exit(1);
    }
    return ret.str();
  }

  switch (optoken) {
  case PortugolParser::T_IGUAL:
    ret << "str_comp(" << left.second << "," << right.second << ")";
    break;
  case PortugolParser::T_DIFERENTE:
    ret << "!str_comp(" << left.second << "," << right.second << ")";
    break;
  case PortugolParser::T_MAIOR:
    ret << "(str_strlen(" << left.second << ") > str_strlen(" << right.second
        << "))";
    break;
  case PortugolParser::T_MENOR:
    ret << "(str_strlen(" << left.second << ") < str_strlen(" << right.second
        << "))";
    break;
  case PortugolParser::T_MAIOR_EQ:
    ret << "(str_strlen(" << left.second << ") >= str_strlen(" << right.second
        << "))";
    break;
  case PortugolParser::T_MENOR_EQ:
    ret << "(str_strlen(" << left.second << ") <= str_strlen(" << right.second
        << "))";
    break;
  default:
    cerr << "Erro interno: op nao suportado (pt2c::translateBinExpr)." << endl;
    exit(1);
  }
  return ret.str();
}

// Integer literals reach the tree with the decimal text produced by
// validateTokens(); this keeps the translator correct on its own too.
string Portugol2CTranslator::intLiteral(const string &text) {
  if (text.length() > 2 && text[0] == '0') {
    int base = 0;
    switch (text[1]) {
    case 'c':
    case 'C':
      base = 8;
      break;
    case 'x':
    case 'X':
      base = 16;
      break;
    case 'b':
    case 'B':
      base = 2;
      break;
    }
    if (base != 0) {
      int base10 = strtoul(text.substr(2).c_str(), NULL, base);
      stringstream s;
      s << base10;
      return s.str();
    }
  }
  return text;
}

// The ANTLR2 lexer stripped the quotes of T_STRING_LIT/T_CARAC_LIT but kept
// the escape sequences.
string Portugol2CTranslator::unquote(const string &text) {
  if (text.length() >= 2) {
    return text.substr(1, text.length() - 2);
  }
  return text;
}

vector<string>
Portugol2CTranslator::dimensoes(PortugolParser::Tipo_declContext *ctx) {
  vector<string> dims;
  if (auto m = dynamic_cast<PortugolParser::TipoMatrizContext *>(ctx)) {
    for (auto d : m->tp_matriz()->dimensoes()->T_INT_LIT()) {
      dims.push_back(intLiteral(d->getText()));
    }
  }
  return dims;
}

char Portugol2CTranslator::matrixTypeChar(int type) {
  switch (type) {
  case TIPO_INTEIRO:
    return 'i';
  case TIPO_REAL:
    return 'f';
  case TIPO_CARACTERE:
    return 'c';
  case TIPO_LITERAL:
    return 's';
  case TIPO_LOGICO:
    return 'b';
  }
  return 0;
}

// Type of an intermediate result inside a flat expression chain, following
// SemanticEval::evaluateExpr (the analyzer only annotates whole nodes).
int Portugol2CTranslator::binaryType(int left, int right, size_t optoken) {
  switch (optoken) {
  case PortugolParser::T_IGUAL:
  case PortugolParser::T_DIFERENTE:
  case PortugolParser::T_MAIOR:
  case PortugolParser::T_MENOR:
  case PortugolParser::T_MAIOR_EQ:
  case PortugolParser::T_MENOR_EQ:
  case PortugolParser::T_KW_OU:
  case PortugolParser::T_KW_E:
    return TIPO_LOGICO;
  default:
    break;
  }
  if (left == TIPO_REAL || right == TIPO_REAL)
    return TIPO_REAL;
  if (left == TIPO_INTEIRO || right == TIPO_INTEIRO)
    return TIPO_INTEIRO;
  if (left == TIPO_CARACTERE || right == TIPO_CARACTERE)
    return TIPO_CARACTERE;
  if (left == TIPO_LOGICO || right == TIPO_LOGICO)
    return TIPO_LOGICO;
  return TIPO_NULO;
}

string Portugol2CTranslator::translate(PortugolParser::AlgoritmoContext *tree) {
  _head.str("");
  _txt.str("");
  _scope_init_stms.str("");
  _indent = "";
  _currentScopeType = -1;

  init(tree->declaracao_algoritmo()->T_IDENTIFICADOR()->getText());

  if (tree->var_decl_block()) {
    variaveis(tree->var_decl_block()->var_decl());
  }

  principal(tree->stm_block());

  for (auto f : tree->func_decls()) {
    funcDecls(f);
  }

  return _head.str() + _txt.str();
}

void Portugol2CTranslator::variaveis(
    const vector<PortugolParser::Var_declContext *> &decls) {
  stringstream str;
  stringstream init;

  for (auto decl : decls) {
    int type = SemanticAnalyzer::tipoDecl(decl->tipo_decl());
    vector<string> dims = dimensoes(decl->tipo_decl());

    if (dims.empty()) { // primitivo
      for (auto id : decl->T_IDENTIFICADOR()) {
        str << translateType(type) << " _" << id->getText() << " = 0;";
        writeln(str.str());
        str.str("");
      }
    } else { // matriz
      for (auto id : decl->T_IDENTIFICADOR()) {
        str << translateType(type) << " _" << id->getText();
        for (auto &dim : dims) {
          str << "[" << dim << "]";
        }
        str << ";";
        writeln(str.str());

        init << "matrix_init(_" << id->getText() << ", ";
        init << "'" << matrixTypeChar(type) << "', ";

        int tsize = 1;
        for (auto &dim : dims) {
          tsize = tsize * atoi(dim.c_str());
        }
        init << tsize << ");";
        addInitStm(init.str());
        init.str("");
        str.str("");
      }
    }
  }
}

void Portugol2CTranslator::principal(PortugolParser::Stm_blockContext *ctx) {
  writeln("\nint main(void) {");
  indent();

  stmBlock(ctx);

  writeln("cleanup();");
  writeln("return EXIT_SUCCESS;\n}");
  unindent();
}

void Portugol2CTranslator::stmBlock(PortugolParser::Stm_blockContext *ctx) {
  writeInitStms();
  stmList(ctx->stm_list());
}

void Portugol2CTranslator::stmList(PortugolParser::Stm_listContext *ctx) {
  for (auto s : ctx->stm()) {
    stm(s);
  }
}

void Portugol2CTranslator::stm(PortugolParser::StmContext *ctx) {
  if (auto s = dynamic_cast<PortugolParser::StmAtribuicaoContext *>(ctx)) {
    stmAttr(s->stm_attr());
  } else if (auto s =
                 dynamic_cast<PortugolParser::StmChamadaFuncContext *>(ctx)) {
    Production fc = fcall(s->fcall(), TIPO_ALL);
    writeln(fc.second + ";");
  } else if (auto s = dynamic_cast<PortugolParser::StmRetornoContext *>(ctx)) {
    stmRet(s->stm_ret());
  } else if (auto s =
                 dynamic_cast<PortugolParser::StmCondicionalContext *>(ctx)) {
    stmSe(s->stm_se());
  } else if (auto s = dynamic_cast<PortugolParser::StmEnquantoContext *>(ctx)) {
    stmEnquanto(s->stm_enquanto());
  } else if (auto s = dynamic_cast<PortugolParser::StmRepitaContext *>(ctx)) {
    stmRepita(s->stm_repita());
  } else if (auto s = dynamic_cast<PortugolParser::StmParaContext *>(ctx)) {
    stmPara(s->stm_para());
  }
}

void Portugol2CTranslator::stmAttr(PortugolParser::Stm_attrContext *ctx) {
  Production lv = lvalue(ctx->lvalue());
  int expecting_type = lv.first;
  Production e = expr(ctx->expr(), expecting_type);

  stringstream str;
  str << lv.second << " = " << e.second << ";";
  writeln(str.str());
}

Portugol2CTranslator::Production
Portugol2CTranslator::lvalue(PortugolParser::LvalueContext *ctx) {
  Production p;
  stringstream s;

  string id = ctx->T_IDENTIFICADOR()->getText();
  p.first = _stable.getSymbol(_currentScope, id, true).type.primitiveType();
  s << "_" << id;

  if (ctx->array_sub()) {
    for (auto e : ctx->array_sub()->expr()) {
      Production sub = expr(e, TIPO_INTEIRO);
      s << "[" << sub.second << "]";
    }
  }

  p.second = s.str();
  return p;
}

Portugol2CTranslator::Production
Portugol2CTranslator::fcall(PortugolParser::FcallContext *ctx, int expct_type) {
  Production p;
  list<Production> lp;
  int count = 0;

  string id = ctx->T_IDENTIFICADOR()->getText();
  p.second = translateFunctionName(id, expct_type);
  Symbol f = _stable.getSymbol(SymbolTable::GlobalScope, id); // so we get
                                                              // the params
  p.first = f.type.primitiveType();
  int type = f.param.paramType(count++);

  if (ctx->fargs()) {
    for (auto e : ctx->fargs()->expr()) {
      lp.push_back(expr(e, type));
      type = f.param.paramType(count++);
    }
  }

  p.second += "(";
  p.second += translateFunctionParams(id, lp);
  p.second += ")";
  return p;
}

void Portugol2CTranslator::stmRet(PortugolParser::Stm_retContext *ctx) {
  int expecting_type = TIPO_NULO;
  bool isGlobalEscope = _currentScope == SymbolTable::GlobalScope;
  if (isGlobalEscope) {
    expecting_type = TIPO_INTEIRO; // o retorno no bloco principal é do
                                   // TIPO_INTEIRO
  } else {
    expecting_type =
        _stable.getSymbol(SymbolTable::GlobalScope, _currentScope, true)
            .type.primitiveType();
  }

  Production e;
  if (auto r = dynamic_cast<PortugolParser::RetorneComExprContext *>(ctx)) {
    e = expr(r->expr(), expecting_type);
  }

  stringstream str;
  str << "return ";
  if (_currentScopeType == TIPO_LITERAL) {
    str << "return_literal(" << e.second << ")";
  } else {
    str << e.second;
  }
  str << ";";
  writeln(str.str());
}

void Portugol2CTranslator::stmSe(PortugolParser::Stm_seContext *ctx) {
  Production e = expr(ctx->expr(), TIPO_LOGICO);

  stringstream str;
  str << "if(" << e.second << ") {";
  writeln(str.str());
  indent();

  stmList(ctx->stm_list());

  unindent();
  write("}");

  if (ctx->senao_part()) {
    writeln(" else {", false);
    indent();

    stmList(ctx->senao_part()->stm_list());

    unindent();
    writeln("}");
  }
  writeln("");
}

void Portugol2CTranslator::stmEnquanto(
    PortugolParser::Stm_enquantoContext *ctx) {
  Production e = expr(ctx->expr(), TIPO_LOGICO);

  stringstream str;
  str << "while(" << e.second << ") {";
  writeln(str.str());
  indent();

  stmList(ctx->stm_list());

  unindent();
  writeln("}");
}

void Portugol2CTranslator::stmRepita(PortugolParser::Stm_repitaContext *ctx) {
  stringstream str;
  str << "do{";
  writeln(str.str());
  indent();

  stmList(ctx->stm_list());

  Production e = expr(ctx->expr(), TIPO_LOGICO);

  unindent();
  str.str("");
  str << "}while (! " << e.second << ");";
  writeln(str.str());
}

void Portugol2CTranslator::stmPara(PortugolParser::Stm_paraContext *ctx) {
  Production var = lvalue(ctx->lvalue());
  Production de = expr(ctx->inicio, TIPO_INTEIRO);
  Production ate = expr(ctx->fim, TIPO_INTEIRO);

  bool haspasso = false;
  bool crescente = true;
  string passo;
  if (ctx->passo()) {
    haspasso = true;
    if (ctx->passo()->T_MENOS()) {
      crescente = false; // decrescente
    }
    passo = intLiteral(ctx->passo()->T_INT_LIT()->getText());
  }

  stringstream str;
  if (!haspasso) {
    str << "for(" << var.second << "=" << de.second << ";" << var.second
        << "<=" << ate.second << ";" << var.second << "+=" << 1 << ") {";
  } else {
    if (crescente) {
      str << "for(" << var.second << "=" << de.second << ";" << var.second
          << "<=" << ate.second << ";" << var.second << "+=" << passo << ") {";
    } else {
      str << "for(" << var.second << "=" << de.second << ";" << var.second
          << ">=" << ate.second << ";" << var.second << "-=" << passo << ") {";
    }
  }
  writeln(str.str());
  indent();

  stmList(ctx->stm_list());

  unindent();
  writeln("}");
  str.str("");
  str << var.second << " = " << ate.second << ";";
  writeln(str.str());
}

Portugol2CTranslator::Production
Portugol2CTranslator::combine(const Production &left, const Production &right,
                              size_t optoken) {
  Production p;
  switch (optoken) {
  case PortugolParser::T_KW_OU:
    p.second = "(" + left.second + "||" + right.second + ")";
    break;
  case PortugolParser::T_KW_E:
    p.second = "(" + left.second + "&&" + right.second + ")";
    break;
  case PortugolParser::T_BIT_OU:
    p.second = "(" + left.second + "|" + right.second + ")";
    break;
  case PortugolParser::T_BIT_XOU:
    p.second = "(" + left.second + "^" + right.second + ")";
    break;
  case PortugolParser::T_BIT_E:
    p.second = "(" + left.second + "&" + right.second + ")";
    break;
  case PortugolParser::T_IGUAL:
  case PortugolParser::T_DIFERENTE:
  case PortugolParser::T_MAIOR:
  case PortugolParser::T_MENOR:
  case PortugolParser::T_MAIOR_EQ:
  case PortugolParser::T_MENOR_EQ:
    p.second = "(" + translateBinExpr(left, right, optoken) + ")";
    break;
  case PortugolParser::T_MAIS:
    p.second = "(" + left.second + "+" + right.second + ")";
    break;
  case PortugolParser::T_MENOS:
    p.second = "(" + left.second + "-" + right.second + ")";
    break;
  case PortugolParser::T_DIV:
    p.second = "(" + left.second + "/" + right.second + ")";
    break;
  case PortugolParser::T_MULTIP:
    p.second = "(" + left.second + "*" + right.second + ")";
    break;
  case PortugolParser::T_MOD:
    p.second = "(" + left.second + "%" + right.second + ")";
    break;
  default:
    cerr << "Erro interno: op nao suportado (pt2c::combine)." << endl;
    exit(1);
  }
  p.first = binaryType(left.first, right.first, optoken);
  return p;
}

// Folds a flat "operand (op operand)*" chain left to right, like the
// left-associative nested nodes of the ANTLR2 AST. The type of the whole
// chain comes from the SemanticAnalyzer annotation, as the walker read it
// from PortugolAST::getEvalType().
template <typename Ctx, typename Fn>
Portugol2CTranslator::Production
Portugol2CTranslator::binaryChain(Ctx *ctx, int expct_type, Fn operand) {
  Production left;
  size_t op = 0;
  bool first = true;
  int operands = 0;

  for (auto child : ctx->children) {
    if (auto t = dynamic_cast<TerminalNode *>(child)) {
      op = t->getSymbol()->getType();
    } else if (auto rule = dynamic_cast<ParserRuleContext *>(child)) {
      operands++;
      if (first) {
        left = operand(rule, expct_type);
        first = false;
      } else {
        Production right = operand(rule, expct_type);
        left = combine(left, right, op);
      }
    }
  }

  if (operands > 1) {
    left.first = _stable.getEvalType(ctx);
  }
  return left;
}

Portugol2CTranslator::Production
Portugol2CTranslator::expr(PortugolParser::ExprContext *ctx, int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprE(static_cast<PortugolParser::Expr_eContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprE(PortugolParser::Expr_eContext *ctx,
                            int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprBitOu(static_cast<PortugolParser::Expr_bit_ouContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprBitOu(PortugolParser::Expr_bit_ouContext *ctx,
                                int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprBitXou(static_cast<PortugolParser::Expr_bit_xouContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprBitXou(PortugolParser::Expr_bit_xouContext *ctx,
                                 int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprBitE(static_cast<PortugolParser::Expr_bit_eContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprBitE(PortugolParser::Expr_bit_eContext *ctx,
                               int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprIgual(static_cast<PortugolParser::Expr_igualContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprIgual(PortugolParser::Expr_igualContext *ctx,
                                int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprRelacional(
        static_cast<PortugolParser::Expr_relacionalContext *>(c), t);
  });
}

Portugol2CTranslator::Production Portugol2CTranslator::exprRelacional(
    PortugolParser::Expr_relacionalContext *ctx, int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprAd(static_cast<PortugolParser::Expr_adContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprAd(PortugolParser::Expr_adContext *ctx,
                             int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprMultip(static_cast<PortugolParser::Expr_multipContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprMultip(PortugolParser::Expr_multipContext *ctx,
                                 int expct_type) {
  return binaryChain(ctx, expct_type, [this](ParserRuleContext *c, int t) {
    return exprUnario(static_cast<PortugolParser::Expr_unarioContext *>(c), t);
  });
}

Portugol2CTranslator::Production
Portugol2CTranslator::exprUnario(PortugolParser::Expr_unarioContext *ctx,
                                 int expct_type) {
  Production p;
  Production right;

  if (auto u = dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    right = element(u->expr_elemento(), expct_type);
    p.second = "(-";
    p.second += right.second + ")";
    p.first = _stable.getEvalType(ctx);
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    right = element(u->expr_elemento(), expct_type);
    p.second = "(+";
    p.second += right.second + ")";
    p.first = _stable.getEvalType(ctx);
  } else if (auto u = dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    right = element(u->expr_elemento(), expct_type);
    p.second = "(!";
    p.second += right.second + ")";
    p.first = _stable.getEvalType(ctx);
  } else if (auto u = dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    right = element(u->expr_elemento(), expct_type);
    p.second = "(~";
    p.second += right.second + ")";
    p.first = _stable.getEvalType(ctx);
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprElementoContext *>(ctx)) {
    p = element(u->expr_elemento(), expct_type);
  }
  return p;
}

Portugol2CTranslator::Production
Portugol2CTranslator::element(PortugolParser::Expr_elementoContext *ctx,
                              int expct_type) {
  Production p;
  if (auto e = dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    p = literal(e->literal());
  } else if (auto e = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    p = fcall(e->fcall(), expct_type);
  } else if (auto e = dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    p = lvalue(e->lvalue());
  } else if (auto e =
                 dynamic_cast<PortugolParser::ElemParentesesContext *>(ctx)) {
    p = expr(e->expr(), expct_type);
  }
  return p;
}

Portugol2CTranslator::Production
Portugol2CTranslator::literal(PortugolParser::LiteralContext *ctx) {
  Production p;
  if (auto s = dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    string text = unquote(s->T_STRING_LIT()->getText());
    p.first = TIPO_LITERAL;
    if (text.length() == 0) {
      p.second = "0";
    } else {
      p.second = "\"";
      p.second += text;
      p.second += "\"";
    }
  } else if (auto i = dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    p.first = TIPO_INTEIRO;
    p.second = intLiteral(i->T_INT_LIT()->getText());
  } else if (auto r = dynamic_cast<PortugolParser::LitRealContext *>(ctx)) {
    p.first = TIPO_REAL;
    p.second = r->T_REAL_LIT()->getText();
  } else if (auto c =
                 dynamic_cast<PortugolParser::LitCaractereContext *>(ctx)) {
    string text = unquote(c->T_CARAC_LIT()->getText());
    p.first = TIPO_CARACTERE;
    if (text.length() > 0) {
      p.second = "'";
      p.second += text;
      p.second += "'";
    } else {
      p.second = "0";
    }
  } else if (dynamic_cast<PortugolParser::LitVerdadeiroContext *>(ctx)) {
    p.first = TIPO_LOGICO;
    p.second = "TRUE";
  } else if (dynamic_cast<PortugolParser::LitFalsoContext *>(ctx)) {
    p.first = TIPO_LOGICO;
    p.second = "FALSE";
  }
  return p;
}

void Portugol2CTranslator::funcDecls(PortugolParser::Func_declsContext *ctx) {
  stringstream str;
  stringstream cpy;
  stringstream decl;
  string comma;

  string id = ctx->T_IDENTIFICADOR()->getText();
  setScope(id);
  // nota: nao estamos usando a producao "rettype" para saber o tipo de
  //       retorno. Procuramos diretamente na tabela de simbolos.
  //       (conveniencia)
  _currentScopeType = _stable.getSymbol(SymbolTable::GlobalScope, id, true)
                          .type.primitiveType();
  str << translateType(_currentScopeType);
  str << " _" << id << "(";

  if (ctx->fparams()) {
    for (auto param : ctx->fparams()->fparam()) {
      int type = SemanticAnalyzer::tipoDecl(param->tipo_decl());
      vector<string> dims = dimensoes(param->tipo_decl());
      string pid = param->T_IDENTIFICADOR()->getText();

      if (dims.empty()) { // primitivo
        str << comma << translateType(type) << " _" << pid;
        comma = ", ";
      } else { // matriz
        stringstream s;
        s << comma << translateType(type) << " __" << pid;
        decl << translateType(type) << " _" << pid;

        comma = ",";
        for (auto &dim : dims) {
          s << "[" << dim << "]";
          decl << "[" << dim << "]";
        }

        decl << ";";
        addInitStm(decl.str());
        decl.str("");

        cpy << "matrix_cpy(__" << pid << ", _" << pid << ", ";
        cpy << "'" << matrixTypeChar(type) << "', ";

        int tsize = 1;
        for (auto &dim : dims) {
          tsize = tsize * atoi(dim.c_str());
        }
        cpy << tsize << ");";
        addInitStm(cpy.str());
        cpy.str("");
        str << s.str();
      }
    }
  }

  str << ")";
  stringstream prototype;
  prototype << str.str() << ";";
  addPrototype(prototype.str());

  str << " {";
  writeln(str.str());
  indent();

  if (auto vars = dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(
          ctx->fvar_decl())) {
    variaveis(vars->var_decl());
  }

  stmBlock(ctx->stm_block());

  unindent();
  writeln("}");
  setScope(SymbolTable::GlobalScope);
  _currentScopeType = -1;
}
