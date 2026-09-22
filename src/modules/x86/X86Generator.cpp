/***************************************************************************
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
 ***************************************************************************/

#include "X86Generator.hpp"
#include "SemanticAnalyzer.hpp"

#include <sstream>
#include <stdlib.h>

using namespace std;
using antlr4::ParserRuleContext;
using antlr4::tree::TerminalNode;

X86Generator::X86Generator(SymbolTable &st) : stable(st), x86(st) {}

string X86Generator::generate(PortugolParser::AlgoritmoContext *tree) {
  algoritmo(tree);
  return x86.source();
}

int X86Generator::calcMatrixOffset(int c, list<int> &dims) {
  int res = 1;
  list<int>::reverse_iterator it = dims.rbegin();
  while (--c) {
    res *= (*it);
    it++;
  }
  return res;
}

// Integer literals reach the tree with the decimal text produced by
// validateTokens(); NASM has no 0c prefix, so this also guards direct use.
string X86Generator::intLiteral(const string &text) {
  if ((text.length() > 2) && (text[0] == '0')) {
    int base10;
    switch (text[1]) {
    case 'c':
    case 'C':
      base10 = strtoul(text.substr(2).c_str(), NULL, 8);
      break;
    case 'x':
    case 'X':
      base10 = strtoul(text.c_str(), NULL, 16);
      break;
    case 'b':
    case 'B':
      base10 = strtoul(text.substr(2).c_str(), NULL, 2);
      break;
    default:
      return text;
    }
    stringstream s;
    s << base10;
    return s.str();
  }
  return text;
}

// The ANTLR2 lexer stripped the quotes of T_STRING_LIT/T_CARAC_LIT but kept
// the escape sequences for toNasmString.
string X86Generator::unquote(const string &text) {
  if (text.length() >= 2) {
    return text.substr(1, text.length() - 2);
  }
  return text;
}

// Type of an intermediate result inside a flat expression chain, following
// SemanticEval::evaluateExpr (the analyzer only annotates whole nodes).
int X86Generator::evalType(int e1, int e2, size_t op) {
  switch (op) {
  case PortugolParser::T_KW_OU:
  case PortugolParser::T_KW_E:
  case PortugolParser::T_IGUAL:
  case PortugolParser::T_DIFERENTE:
  case PortugolParser::T_MAIOR:
  case PortugolParser::T_MENOR:
  case PortugolParser::T_MAIOR_EQ:
  case PortugolParser::T_MENOR_EQ:
    return TIPO_LOGICO;
  default:
    break;
  }

  auto isNumeric = [](int t) {
    return (t == TIPO_REAL) || (t == TIPO_INTEIRO) || (t == TIPO_CARACTERE) ||
           (t == TIPO_LOGICO) || (t == TIPO_ALL);
  };
  if (!isNumeric(e1) || !isNumeric(e2)) {
    return TIPO_NULO;
  }
  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    return TIPO_REAL;
  } else if ((e1 == TIPO_INTEIRO) || (e2 == TIPO_INTEIRO)) {
    return TIPO_INTEIRO;
  } else if ((e1 == TIPO_CARACTERE) || (e2 == TIPO_CARACTERE)) {
    return TIPO_CARACTERE;
  } else if ((e1 == TIPO_LOGICO) || (e2 == TIPO_LOGICO)) {
    return TIPO_LOGICO;
  }
  return TIPO_NULO;
}

void X86Generator::writeBinaryExpr(size_t op, int e1, int e2) {
  switch (op) {
  case PortugolParser::T_KW_OU:
    x86.writeOuExpr();
    break;
  case PortugolParser::T_KW_E:
    x86.writeEExpr();
    break;
  case PortugolParser::T_BIT_OU:
    x86.writeBitOuExpr();
    break;
  case PortugolParser::T_BIT_XOU:
    x86.writeBitXouExpr();
    break;
  case PortugolParser::T_BIT_E:
    x86.writeBitEExpr();
    break;
  case PortugolParser::T_IGUAL:
    x86.writeIgualExpr(e1, e2);
    break;
  case PortugolParser::T_DIFERENTE:
    x86.writeDiferenteExpr(e1, e2);
    break;
  case PortugolParser::T_MAIOR:
    x86.writeMaiorExpr(e1, e2);
    break;
  case PortugolParser::T_MENOR:
    x86.writeMenorExpr(e1, e2);
    break;
  case PortugolParser::T_MAIOR_EQ:
    x86.writeMaiorEqExpr(e1, e2);
    break;
  case PortugolParser::T_MENOR_EQ:
    x86.writeMenorEqExpr(e1, e2);
    break;
  case PortugolParser::T_MAIS:
    x86.writeMaisExpr(e1, e2);
    break;
  case PortugolParser::T_MENOS:
    x86.writeMenosExpr(e1, e2);
    break;
  case PortugolParser::T_DIV:
    x86.writeDivExpr(e1, e2);
    break;
  case PortugolParser::T_MULTIP:
    x86.writeMultipExpr(e1, e2);
    break;
  case PortugolParser::T_MOD:
    x86.writeModExpr();
    break;
  }
}

template <typename Ctx, typename Fn>
int X86Generator::binaryChain(Ctx *ctx, int expecting_type, Fn operand) {
  int etype = TIPO_NULO;  // tipo devolvido pela produção (walker: "etype")
  int evtype = TIPO_NULO; // tipo avaliado do nó (walker: getEvalType())
  TerminalNode *op = nullptr;
  bool first = true;

  for (auto child : ctx->children) {
    if (auto t = dynamic_cast<TerminalNode *>(child)) {
      op = t;
    } else if (auto rule = dynamic_cast<ParserRuleContext *>(child)) {
      if (first) {
        etype = operand(rule, expecting_type);
        evtype = stable.getEvalType(rule);
        first = false;
      } else {
        int e2 = operand(rule, expecting_type);
        size_t optype = op->getSymbol()->getType();
        writeBinaryExpr(optype, etype, e2);
        evtype = evalType(evtype, stable.getEvalType(rule), optype);
        etype = evtype;
      }
    }
  }
  return etype;
}

void X86Generator::algoritmo(PortugolParser::AlgoritmoContext *ctx) {
  x86.init(ctx->declaracao_algoritmo()->T_IDENTIFICADOR()->getText());

  if (ctx->var_decl_block()) {
    variaveis(ctx->var_decl_block()->var_decl(), X86::VAR_GLOBAL);
  }

  principal(ctx->stm_block());

  for (auto f : ctx->func_decls()) {
    funcDecls(f);
  }
}

void X86Generator::variaveis(
    const vector<PortugolParser::Var_declContext *> &decls, int decl_type) {
  for (auto decl : decls) {
    declaracao(decl->tipo_decl(), decl->T_IDENTIFICADOR(), decl_type);
  }
}

void X86Generator::declaracao(PortugolParser::Tipo_declContext *tipo,
                              const vector<TerminalNode *> &ids,
                              int decl_type) {
  if (auto prim = dynamic_cast<PortugolParser::TipoPrimitivoContext *>(tipo)) {
    int type = SemanticAnalyzer::tipoPrim(prim->tp_prim());
    for (auto id : ids) {
      x86.declarePrimitive(decl_type, id->getText(), type);
    }
  } else if (auto mat =
                 dynamic_cast<PortugolParser::TipoMatrizContext *>(tipo)) {
    int type = SemanticAnalyzer::tipoPrimPl(mat->tp_matriz()->tp_prim_pl());
    list<string> dims;
    for (auto d : mat->tp_matriz()->dimensoes()->T_INT_LIT()) {
      dims.push_back(intLiteral(d->getText()));
    }
    for (auto id : ids) {
      x86.declareMatrix(decl_type, type, id->getText(), dims);
    }
  }
}

void X86Generator::principal(PortugolParser::Stm_blockContext *ctx) {
  stmBlock(ctx);
  x86.writeTEXT("mov ecx, 0");
  x86.writeExit();
}

void X86Generator::stmBlock(PortugolParser::Stm_blockContext *ctx) {
  stmList(ctx->stm_list());
}

void X86Generator::stmList(PortugolParser::Stm_listContext *ctx) {
  for (auto s : ctx->stm()) {
    stm(s);
  }
}

void X86Generator::stm(PortugolParser::StmContext *ctx) {
  if (auto s = dynamic_cast<PortugolParser::StmAtribuicaoContext *>(ctx)) {
    stmAttr(s->stm_attr());
  } else if (auto s =
                 dynamic_cast<PortugolParser::StmChamadaFuncContext *>(ctx)) {
    fcall(s->fcall(), TIPO_ALL);
    x86.writeTEXT("pop eax");
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

void X86Generator::stmAttr(PortugolParser::Stm_attrContext *ctx) {
  LValue lv = lvalue(ctx->lvalue());

  Symbol symb = stable.getSymbol(x86.currentScope(), lv.second, true);
  int expecting_type = symb.type.primitiveType();
  x86.writeTEXT("push ecx");

  int etype = expr(ctx->expr(), expecting_type);

  x86.writeAttribution(etype, expecting_type, lv);
}

X86Generator::LValue X86Generator::lvalue(PortugolParser::LvalueContext *ctx) {
  stringstream s;
  LValue p;

  string name = ctx->T_IDENTIFICADOR()->getText();
  Symbol symb = stable.getSymbol(x86.currentScope(), name, true);
  p.first.first = symb.type.primitiveType();
  bool isprim = symb.type.isPrimitive();
  p.second = name;

  list<int> dims = symb.type.dimensions();
  int c = dims.size();

  if (!isprim) {
    p.first.second = true;
    x86.writeTEXT("push 0");
  } else {
    p.first.second = false;
  }

  for (auto index : ctx->array_sub()->expr()) {
    expr(index, TIPO_INTEIRO); // index expr type

    p.first.second = false;
    int multiplier = calcMatrixOffset(c, dims);
    x86.writeTEXT("pop eax");

    s << "mov ebx, " << multiplier;
    x86.writeTEXT(s.str());
    x86.writeTEXT("imul ebx");
    x86.writeTEXT("pop ebx");
    x86.writeTEXT("add eax, ebx");
    x86.writeTEXT("push eax");
    s.str("");
    c--;
  }

  if (symb.type.isPrimitive()) {
    x86.writeTEXT("mov ecx, 0");
  } else {
    x86.writeTEXT("pop ecx");
  }

  return p;
}

int X86Generator::fcall(PortugolParser::FcallContext *ctx, int expct_type) {
  int count = 0;
  stringstream s;
  string fname;
  int args = 0;
  int type;
  int ptype = 0;

  string id = ctx->T_IDENTIFICADOR()->getText();
  // so we get the params
  Symbol f = stable.getSymbol(SymbolTable::GlobalScope, id);
  if (f.lexeme == "leia") {
    fname = x86.translateFuncLeia(id, expct_type);
    type = expct_type;
    // ptype doesn't matter: (expr)* is not used
  } else {
    fname = f.lexeme;
    type = f.type.primitiveType();
  }
  ptype = f.param.paramType(count++);

  for (auto arg : ctx->fargs()->expr()) {
    int etype = expr(arg, ptype);
    if (fname == "imprima") {
      switch (etype) {
      case TIPO_INTEIRO:
        x86.writeTEXT("addarg 'i'");
        break;
      case TIPO_REAL:
        x86.writeTEXT("addarg 'r'");
        break;
      case TIPO_CARACTERE:
        x86.writeTEXT("addarg 'c'");
        break;
      case TIPO_LITERAL:
        x86.writeTEXT("addarg 's'");
        break;
      case TIPO_LOGICO:
        x86.writeTEXT("addarg 'l'");
        break;
      }
    } else {
      x86.writeTEXT("pop eax");
      x86.writeCast(etype, ptype);
      x86.writeTEXT("addarg eax");
      ptype = f.param.paramType(count++);
    }
    args++;
  }

  if (fname == "imprima") {
    s << "addarg " << args;
    x86.writeTEXT(s.str());
    x86.writeTEXT("call imprima");
    s.str("");
    s << "clargs " << ((args * 2) + 1);
    x86.writeTEXT(s.str());
    x86.writeTEXT("print_lf"); //\n
  } else if (f.lexeme == "leia") {
    x86.writeTEXT(string("call ") + fname);
  } else {
    x86.writeTEXT(string("call ") + X86::makeID(fname));
    if (args) {
      s << "clargs " << args;
      x86.writeTEXT(s.str());
    }
  }

  x86.writeTEXT("push eax");

  return type;
}

void X86Generator::stmRet(PortugolParser::Stm_retContext *ctx) {
  int expecting_type = TIPO_NULO;
  int etype = TIPO_NULO;
  bool isGlobalEscope = (x86.currentScope() == SymbolTable::GlobalScope);
  if (isGlobalEscope) {
    expecting_type = TIPO_INTEIRO; // o retorno no bloco principal é inteiro
  } else {
    expecting_type =
        stable.getSymbol(SymbolTable::GlobalScope, x86.currentScope(), true)
            .type.primitiveType();
  }

  if (auto r = dynamic_cast<PortugolParser::RetorneComExprContext *>(ctx)) {
    etype = expr(r->expr(), expecting_type);
  }

  if (isGlobalEscope) {
    x86.writeTEXT("pop ecx");
    x86.writeExit();
  } else {
    if (expecting_type != TIPO_NULO) {
      x86.writeTEXT("pop eax");
    }
    if (expecting_type == TIPO_LITERAL) {
      x86.writeTEXT("addarg eax");
      x86.writeTEXT("call clone_literal");
      x86.writeTEXT("clargs 1");
    } else {
      x86.writeCast(etype, expecting_type);
    }

    x86.writeTEXT("return");
  }
}

void X86Generator::stmSe(PortugolParser::Stm_seContext *ctx) {
  stringstream s;
  string lbnext, lbfim;

  lbnext = x86.createLabel(true, "next_se");
  lbfim = x86.createLabel(true, "fim_se");

  bool hasElse = false;

  x86.writeTEXT("; se: expressao");

  expr(ctx->expr(), TIPO_LOGICO);

  x86.writeTEXT("; se: resultado");
  x86.writeTEXT("pop eax");
  x86.writeTEXT("cmp eax, 0");
  s << "je near " << lbnext;
  x86.writeTEXT(s.str());

  x86.writeTEXT("; se: verdadeiro:");

  stmList(ctx->stm_list());

  if (ctx->senao_part()) {
    hasElse = true;

    s.str("");
    s << "jmp " << lbfim;
    x86.writeTEXT(s.str());

    x86.writeTEXT("; se: falso:");

    s.str("");
    s << lbnext << ":";
    x86.writeTEXT(s.str());

    stmList(ctx->senao_part()->stm_list());
  }

  x86.writeTEXT("; se: fim:");

  s.str("");
  if (hasElse) {
    s << lbfim << ":";
  } else {
    s << lbnext << ":";
  }
  x86.writeTEXT(s.str());
}

void X86Generator::stmEnquanto(PortugolParser::Stm_enquantoContext *ctx) {
  stringstream s;
  string lbenq = x86.createLabel(true, "enquanto");
  string lbfim = x86.createLabel(true, "fim_enquanto");

  s << lbenq << ":";
  x86.writeTEXT(s.str());
  s.str("");

  x86.writeTEXT("; while: expressao");

  expr(ctx->expr(), TIPO_LOGICO);

  x86.writeTEXT("; while: resultado");
  x86.writeTEXT("pop eax");
  x86.writeTEXT("cmp eax, 0");
  s << "je near " << lbfim;
  x86.writeTEXT(s.str());

  stmList(ctx->stm_list());

  s.str("");
  s << "jmp " << lbenq;
  x86.writeTEXT(s.str());

  s.str("");
  s << lbfim << ":";
  x86.writeTEXT(s.str());
}

void X86Generator::stmRepita(PortugolParser::Stm_repitaContext *ctx) {
  stringstream s;
  string lbrep = x86.createLabel(true, "repita");
  string lbfim = x86.createLabel(true, "ate");

  s << lbrep << ":";
  x86.writeTEXT(s.str());
  s.str("");

  x86.writeTEXT("; repita");

  stmList(ctx->stm_list());

  x86.writeTEXT("; until expressao");

  expr(ctx->expr(), TIPO_LOGICO);

  x86.writeTEXT("; until resultado");
  x86.writeTEXT("pop eax");
  x86.writeTEXT("cmp eax, 0");
  s << "je near " << lbrep;
  x86.writeTEXT(s.str());
}

void X86Generator::stmPara(PortugolParser::Stm_paraContext *ctx) {
  stringstream s;
  LValue lv;
  pair<int, string> ps;
  int de_type, ate_type;
  bool hasPasso = false;
  string lbpara = x86.createLabel(true, "para");
  string lbfim = x86.createLabel(true, "fim_para");

  x86.writeTEXT("; para: lvalue:");

  lv = lvalue(ctx->lvalue());

  Symbol symb = stable.getSymbol(x86.currentScope(), lv.second, true);
  int expecting_type = symb.type.primitiveType();
  x86.writeTEXT("push ecx"); // lvalue's offset to be used later
  x86.writeTEXT("; para: de:");

  de_type = expr(ctx->inicio, TIPO_INTEIRO);

  x86.writeTEXT("; para: de attr:");
  x86.writeAttribution(de_type, expecting_type, lv);
  x86.writeTEXT("push ecx");
  x86.writeTEXT("; para: ate:");

  ate_type = expr(ctx->fim, TIPO_INTEIRO);

  x86.writeTEXT("pop eax");
  x86.writeCast(ate_type, lv.first.first);
  x86.writeTEXT("push eax"); // top stack tem "ate"

  if (ctx->passo()) {
    ps = passo(ctx->passo());
    hasPasso = true;
  }

  // nao entrar se condicao falsa
  x86.writeTEXT("mov ecx, dword [esp+4]");
  s.str("");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  x86.writeTEXT(s.str());
  x86.writeTEXT("mov eax, dword [edx + ecx * SIZEOF_DWORD]");

  x86.writeTEXT("mov ebx, dword [esp]");
  x86.writeTEXT("cmp eax, ebx");

  s.str("");
  if (hasPasso && ps.first) {
    s << "jl " << lbfim;
  } else {
    s << "jg near " << lbfim;
  }
  x86.writeTEXT(s.str());

  s.str("");
  s << lbpara << ":";
  x86.writeTEXT(s.str());

  stmList(ctx->stm_list());

  // calcular passo [eax]
  x86.writeTEXT("mov ecx, dword [esp+4]");
  s.str("");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  x86.writeTEXT(s.str());
  x86.writeTEXT("mov eax, dword [edx + ecx * SIZEOF_DWORD]");

  s.str("");
  if (!hasPasso) {
    x86.writeTEXT("inc eax");
  } else {
    if (ps.first) { // dec
      s << "sub eax, " << ps.second;
    } else { // cresc
      s << "add eax, " << ps.second;
    }
  }
  x86.writeTEXT(s.str());

  // desviar controle
  x86.writeTEXT("mov ebx, dword [esp]");
  x86.writeTEXT("cmp eax, ebx");

  s.str("");
  if (hasPasso && ps.first) {
    s << "jl near " << lbfim;
  } else {
    s << "jg near " << lbfim;
  }
  x86.writeTEXT(s.str());

  s.str("");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  x86.writeTEXT(s.str());
  x86.writeTEXT("mov ecx, dword [esp+4]");
  x86.writeTEXT("lea edx, [edx + ecx * SIZEOF_DWORD]");
  x86.writeTEXT("mov dword [edx], eax");

  s.str("");
  s << "jmp " << lbpara;
  x86.writeTEXT(s.str());

  s.str("");
  s << lbfim << ":";
  x86.writeTEXT(s.str());

  // lvalue = ate value
  x86.writeTEXT("mov ebx, dword [esp]");
  s.str("");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  x86.writeTEXT(s.str());
  x86.writeTEXT("mov ecx, dword [esp+4]");
  x86.writeTEXT("lea edx, [edx + ecx * SIZEOF_DWORD]");
  x86.writeTEXT("mov dword [edx], ebx");

  // pop ate, pop lvalue offset
  x86.writeTEXT("pop eax");
  x86.writeTEXT("pop ecx");
  x86.writeTEXT("; fimpara");
}

pair<int, string> X86Generator::passo(PortugolParser::PassoContext *ctx) {
  pair<int, string> p;
  p.first = ctx->T_MENOS() ? 1 : 0;
  p.second = intLiteral(ctx->T_INT_LIT()->getText());
  return p;
}

int X86Generator::expr(PortugolParser::ExprContext *ctx, int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprE(static_cast<PortugolParser::Expr_eContext *>(c), et);
  });
}

int X86Generator::exprE(PortugolParser::Expr_eContext *ctx,
                        int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprBitOu(static_cast<PortugolParser::Expr_bit_ouContext *>(c), et);
  });
}

int X86Generator::exprBitOu(PortugolParser::Expr_bit_ouContext *ctx,
                            int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprBitXou(static_cast<PortugolParser::Expr_bit_xouContext *>(c),
                      et);
  });
}

int X86Generator::exprBitXou(PortugolParser::Expr_bit_xouContext *ctx,
                             int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprBitE(static_cast<PortugolParser::Expr_bit_eContext *>(c), et);
  });
}

int X86Generator::exprBitE(PortugolParser::Expr_bit_eContext *ctx,
                           int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprIgual(static_cast<PortugolParser::Expr_igualContext *>(c), et);
  });
}

int X86Generator::exprIgual(PortugolParser::Expr_igualContext *ctx,
                            int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprRelacional(
        static_cast<PortugolParser::Expr_relacionalContext *>(c), et);
  });
}

int X86Generator::exprRelacional(PortugolParser::Expr_relacionalContext *ctx,
                                 int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprAd(static_cast<PortugolParser::Expr_adContext *>(c), et);
  });
}

int X86Generator::exprAd(PortugolParser::Expr_adContext *ctx,
                         int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprMultip(static_cast<PortugolParser::Expr_multipContext *>(c), et);
  });
}

int X86Generator::exprMultip(PortugolParser::Expr_multipContext *ctx,
                             int expecting_type) {
  return binaryChain(ctx, expecting_type, [this](ParserRuleContext *c, int et) {
    return exprUnario(static_cast<PortugolParser::Expr_unarioContext *>(c), et);
  });
}

int X86Generator::exprUnario(PortugolParser::Expr_unarioContext *ctx,
                             int expecting_type) {
  int etype = TIPO_NULO;
  if (auto u = dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
    x86.writeUnaryNeg(etype);
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
    // nothing
  } else if (auto u = dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
    x86.writeUnaryNot();
  } else if (auto u = dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
    x86.writeUnaryBitNotExpr();
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprElementoContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
  }
  return etype;
}

int X86Generator::element(PortugolParser::Expr_elementoContext *ctx,
                          int expecting_type) {
  if (auto e = dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    pair<int, string> lit = literal(e->literal());
    x86.writeLiteralExpr(lit.second);
    return lit.first;
  } else if (auto e = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    return fcall(e->fcall(), expecting_type);
  } else if (auto e = dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    LValue lv = lvalue(e->lvalue());
    x86.writeLValueExpr(lv);
    return lv.first.first;
  } else if (auto e =
                 dynamic_cast<PortugolParser::ElemParentesesContext *>(ctx)) {
    return expr(e->expr(), expecting_type);
  }
  return TIPO_NULO;
}

pair<int, string> X86Generator::literal(PortugolParser::LiteralContext *ctx) {
  pair<int, string> p;
  if (auto s = dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    string text = unquote(s->T_STRING_LIT()->getText());
    if (text.length() > 0) {
      p.second = x86.addGlobalLiteral(text);
    } else {
      p.second = "0";
    }
    p.first = TIPO_LITERAL;
  } else if (auto i = dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    p.second = intLiteral(i->T_INT_LIT()->getText());
    p.first = TIPO_INTEIRO;
  } else if (auto c =
                 dynamic_cast<PortugolParser::LitCaractereContext *>(ctx)) {
    p.second = x86.toChar(unquote(c->T_CARAC_LIT()->getText()));
    p.first = TIPO_CARACTERE;
  } else if (dynamic_cast<PortugolParser::LitVerdadeiroContext *>(ctx)) {
    p.second = "1";
    p.first = TIPO_LOGICO;
  } else if (dynamic_cast<PortugolParser::LitFalsoContext *>(ctx)) {
    p.second = "0";
    p.first = TIPO_LOGICO;
  } else if (auto r = dynamic_cast<PortugolParser::LitRealContext *>(ctx)) {
    p.second = x86.toReal(r->T_REAL_LIT()->getText());
    p.first = TIPO_REAL;
  }
  return p;
}

void X86Generator::funcDecls(PortugolParser::Func_declsContext *ctx) {
  x86.createScope(ctx->T_IDENTIFICADOR()->getText());

  for (auto param : ctx->fparams()->fparam()) {
    vector<TerminalNode *> ids(1, param->T_IDENTIFICADOR());
    declaracao(param->tipo_decl(), ids, X86::VAR_PARAM);
  }

  if (auto vars = dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(
          ctx->fvar_decl())) {
    variaveis(vars->var_decl(), X86::VAR_LOCAL);
  }

  stmBlock(ctx->stm_block());

  x86.writeTEXT("return");
}
