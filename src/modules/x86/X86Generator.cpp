/***************************************************************************
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *   Copyright (C) 2026 by Marcelo Jorge Vieira                            *
 *   metal@alucinados.com                                                  *
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
#include "GPTDisplay.hpp"
#include "PortugolErrorStrategy.hpp"
#include "SemanticAnalyzer.hpp"

#include <stdlib.h>

using namespace std;
using antlr4::ParserRuleContext;
using antlr4::tree::TerminalNode;

X86Generator::X86Generator(SymbolTable &st, NativeBackend &backend)
    : stable(st), x86(backend) {}

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

// Se, enquanto, repita, não, e and ou test the low 32 bits of a value, so a
// real operand becomes a lógico first.
void X86Generator::logicalOperand(int etype) {
  if (etype == TIPO_REAL) {
    x86.writeRealToLogical();
  }
}

void X86Generator::writeBinaryExpr(antlr4::Token *op, int e1, int e2) {
  switch (op->getType()) {
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
    x86.writeDivExpr(e1, e2,
                     GPTDisplay::self()->sourceLine((int)op->getLine()));
    break;
  case PortugolParser::T_MULTIP:
    x86.writeMultipExpr(e1, e2);
    break;
  case PortugolParser::T_MOD:
    x86.writeModExpr(GPTDisplay::self()->sourceLine((int)op->getLine()));
    break;
  }
}

template <typename Ctx, typename Fn>
int X86Generator::binaryChain(Ctx *ctx, int expecting_type, Fn operand) {
  const vector<antlr4::tree::ParseTree *> &children = ctx->children;
  ParserRuleContext *first = static_cast<ParserRuleContext *>(children[0]);
  // type the production returns (walker: "etype")
  int etype = operand(first, expecting_type);
  if (children.size() == 1) {
    return etype;
  }
  // evaluated type of the node (walker: getEvalType())
  int evtype = SemanticAnalyzer::chainEvalType(stable, first);
  for (size_t i = 1; i + 1 < children.size(); i += 2) {
    antlr4::Token *optoken =
        static_cast<TerminalNode *>(children[i])->getSymbol();
    ParserRuleContext *rule = static_cast<ParserRuleContext *>(children[i + 1]);
    bool logical = (optoken->getType() == PortugolParser::T_KW_E) ||
                   (optoken->getType() == PortugolParser::T_KW_OU);
    if (logical) {
      logicalOperand(etype);
    }
    int e2 = operand(rule, expecting_type);
    if (logical) {
      logicalOperand(e2);
    }
    writeBinaryExpr(optoken, etype, e2);
    evtype = evalType(evtype, SemanticAnalyzer::chainEvalType(stable, rule),
                      optoken->getType());
    etype = evtype;
  }
  return etype;
}

void X86Generator::algoritmo(PortugolParser::AlgoritmoContext *ctx) {
  x86.init(ctx->declaracao_algoritmo()->T_IDENTIFICADOR()->getText());

  if (ctx->var_decl_block()) {
    variaveis(ctx->var_decl_block()->var_decl(), NativeBackend::VAR_GLOBAL);
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
      dims.push_back(d->getText());
    }
    for (auto id : ids) {
      x86.declareMatrix(decl_type, type, id->getText(), dims);
    }
  }
}

void X86Generator::principal(PortugolParser::Stm_blockContext *ctx) {
  stmBlock(ctx);
  x86.writeMainExit();
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
    x86.writeDiscardResult();
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
  x86.writePushIndex();

  int etype = expr(ctx->expr(), expecting_type);

  x86.writeAttribution(etype, expecting_type, lv);
}

X86Generator::LValue X86Generator::lvalue(PortugolParser::LvalueContext *ctx) {
  LValue p;

  antlr4::Token *id = ctx->T_IDENTIFICADOR()->getSymbol();
  string name = id->getText();
  Symbol symb = stable.getSymbol(x86.currentScope(), name, true);
  p.first.first = symb.type.primitiveType();
  bool isprim = symb.type.isPrimitive();
  p.second = name;

  list<int> dims = symb.type.dimensions();
  list<int>::iterator dim = dims.begin();
  int c = dims.size();
  int line = GPTDisplay::self()->sourceLine((int)id->getLine());

  p.first.second = !isprim;
  x86.writeLValueBegin(isprim);

  for (auto index : ctx->array_sub()->expr()) {
    expr(index, TIPO_INTEIRO); // index expr type

    p.first.second = false;
    int multiplier = calcMatrixOffset(c, dims);
    x86.writeLValueIndex(name, line, *dim, multiplier);
    ++dim;
    c--;
  }

  x86.writeLValueEnd(isprim);

  return p;
}

int X86Generator::fcall(PortugolParser::FcallContext *ctx, int expct_type) {
  int count = 0;
  string fname;
  int args = 0;
  int type;
  int ptype = 0;

  string id = ctx->T_IDENTIFICADOR()->getText();
  // so we get the params
  Symbol f = stable.getSymbol(SymbolTable::GlobalScope, id);
  if (f.lexeme == "leia") {
    // No type is expected in imprima(leia()) or a bare leia(); read the
    // whole line as text, like the interpreter.
    type = (expct_type == TIPO_ALL) ? TIPO_LITERAL : expct_type;
    fname = x86.translateFuncLeia(type);
    // ptype doesn't matter: (expr)* is not used
  } else {
    fname = f.lexeme;
    type = f.type.primitiveType();
  }
  ptype = f.param.paramType(count++);

  for (auto arg : ctx->fargs()->expr()) {
    int etype = expr(arg, ptype);
    if (fname == "imprima") {
      x86.writeImprimaTag(etype);
    } else {
      x86.writeCallArg(etype, ptype);
      ptype = f.param.paramType(count++);
    }
    args++;
  }

  if (fname == "imprima") {
    x86.writeImprimaCall(args);
  } else if (f.lexeme == "leia") {
    x86.writeLeiaCall(fname);
  } else {
    x86.writeCall(fname, args);
  }

  x86.writePushResult();

  return type;
}

void X86Generator::stmRet(PortugolParser::Stm_retContext *ctx) {
  int expecting_type = TIPO_NULO;
  int etype = TIPO_NULO;
  bool hasRetExpr = false;
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
    hasRetExpr = true;
  }

  if (isGlobalEscope) {
    x86.writeMainReturn();
  } else {
    x86.writeFuncReturn(expecting_type, etype, hasRetExpr);
  }
}

void X86Generator::stmSe(PortugolParser::Stm_seContext *ctx) {
  string lbnext, lbfim;

  lbnext = x86.createLabel(true, "next_se");
  lbfim = x86.createLabel(true, "fim_se");

  bool hasElse = false;

  x86.writeComment("se: expressao");

  logicalOperand(expr(ctx->expr(), TIPO_LOGICO));

  x86.writeComment("se: resultado");
  x86.writeJumpIfFalse(lbnext);
  x86.writeComment("se: verdadeiro:");

  stmList(ctx->stm_list());

  if (ctx->senao_part()) {
    hasElse = true;
    x86.writeJump(lbfim);
    x86.writeComment("se: falso:");
    x86.writeLabel(lbnext);

    stmList(ctx->senao_part()->stm_list());
  }

  x86.writeComment("se: fim:");
  x86.writeLabel(hasElse ? lbfim : lbnext);
}

void X86Generator::stmEnquanto(PortugolParser::Stm_enquantoContext *ctx) {
  string lbenq = x86.createLabel(true, "enquanto");
  string lbfim = x86.createLabel(true, "fim_enquanto");

  x86.writeLabel(lbenq);
  x86.writeComment("while: expressao");

  logicalOperand(expr(ctx->expr(), TIPO_LOGICO));

  x86.writeComment("while: resultado");
  x86.writeJumpIfFalse(lbfim);

  stmList(ctx->stm_list());

  x86.writeJump(lbenq);
  x86.writeLabel(lbfim);
}

void X86Generator::stmRepita(PortugolParser::Stm_repitaContext *ctx) {
  string lbrep = x86.createLabel(true, "repita");
  string lbfim = x86.createLabel(true, "ate");

  x86.writeLabel(lbrep);
  x86.writeComment("repita");

  stmList(ctx->stm_list());

  x86.writeComment("until expressao");

  logicalOperand(expr(ctx->expr(), TIPO_LOGICO));

  x86.writeComment("until resultado");
  x86.writeJumpIfFalse(lbrep);
}

void X86Generator::stmPara(PortugolParser::Stm_paraContext *ctx) {
  LValue lv;
  pair<int, string> ps;
  int de_type, ate_type;
  bool hasPasso = false;
  string lbpara = x86.createLabel(true, "para");
  string lbfim = x86.createLabel(true, "fim_para");

  x86.writeComment("para: lvalue:");

  lv = lvalue(ctx->lvalue());

  Symbol symb = stable.getSymbol(x86.currentScope(), lv.second, true);
  int expecting_type = symb.type.primitiveType();
  x86.writeParaDe();

  de_type = expr(ctx->inicio, TIPO_INTEIRO);

  x86.writeParaDeAttr(de_type, expecting_type, lv);

  ate_type = expr(ctx->fim, TIPO_INTEIRO);

  x86.writeParaAte(ate_type, lv.first.first);

  if (ctx->passo()) {
    ps = passo(ctx->passo());
    hasPasso = true;
  }

  x86.writeParaEntry(lv.second, hasPasso && ps.first, lbpara, lbfim);

  stmList(ctx->stm_list());

  x86.writeParaNext(lv.second, hasPasso, ps.first, ps.second, lbpara, lbfim);
}

pair<int, string> X86Generator::passo(PortugolParser::PassoContext *ctx) {
  pair<int, string> p;
  p.first = ctx->T_MENOS() ? 1 : 0;
  p.second = ctx->T_INT_LIT()->getText();
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
  if (auto u = dynamic_cast<PortugolParser::ExprElementoContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
    x86.writeUnaryNeg(etype);
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
    // nothing
  } else if (auto u = dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    logicalOperand(element(u->expr_elemento(), expecting_type));
    x86.writeUnaryNot();
    etype = TIPO_LOGICO;
  } else if (auto u = dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    etype = element(u->expr_elemento(), expecting_type);
    x86.writeUnaryBitNotExpr();
  }
  return etype;
}

int X86Generator::element(PortugolParser::Expr_elementoContext *ctx,
                          int expecting_type) {
  if (auto e = dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    LValue lv = lvalue(e->lvalue());
    x86.writeLValueExpr(lv);
    return lv.first.first;
  } else if (auto e = dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    pair<int, string> lit = literal(e->literal());
    x86.writeLiteralExpr(lit.first, lit.second);
    return lit.first;
  } else if (auto e = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    return fcall(e->fcall(), expecting_type);
  } else if (auto e =
                 dynamic_cast<PortugolParser::ElemParentesesContext *>(ctx)) {
    return expr(e->expr(), expecting_type);
  }
  return TIPO_NULO;
}

pair<int, string> X86Generator::literal(PortugolParser::LiteralContext *ctx) {
  pair<int, string> p;
  if (auto i = dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    p.second = i->T_INT_LIT()->getText();
    p.first = TIPO_INTEIRO;
  } else if (auto s = dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    string text = unquote(s->T_STRING_LIT()->getText());
    if (text.length() > 0) {
      p.second = x86.addGlobalLiteral(text);
    } else {
      p.second = "0";
    }
    p.first = TIPO_LITERAL;
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
    declaracao(param->tipo_decl(), ids, NativeBackend::VAR_PARAM);
  }

  if (auto vars = dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(
          ctx->fvar_decl())) {
    variaveis(vars->var_decl(), NativeBackend::VAR_LOCAL);
  }

  stmBlock(ctx->stm_block());

  x86.writeFuncEnd();
}
