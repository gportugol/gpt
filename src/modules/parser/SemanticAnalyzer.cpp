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

#include "SemanticAnalyzer.hpp"
#include "GPTDisplay.hpp"
#include "Symbol.hpp"

#include <sstream>

using namespace std;
using antlr4::ParserRuleContext;
using antlr4::tree::TerminalNode;

SemanticAnalyzer::SemanticAnalyzer(SymbolTable &st)
    : evaluator(st), _stable(st) {}

TokenRef SemanticAnalyzer::ref(TerminalNode *node) {
  antlr4::Token *t = node->getSymbol();
  return TokenRef(t->getText(), (int)t->getLine(), (int)t->getType());
}

int SemanticAnalyzer::tipoPrim(PortugolParser::Tp_primContext *ctx) {
  if (!ctx)
    return TIPO_NULO;
  if (ctx->T_KW_INTEIRO())
    return TIPO_INTEIRO;
  if (ctx->T_KW_REAL())
    return TIPO_REAL;
  if (ctx->T_KW_CARACTERE())
    return TIPO_CARACTERE;
  if (ctx->T_KW_LITERAL())
    return TIPO_LITERAL;
  if (ctx->T_KW_LOGICO())
    return TIPO_LOGICO;
  return TIPO_NULO;
}

int SemanticAnalyzer::tipoPrimPl(PortugolParser::Tp_prim_plContext *ctx) {
  if (!ctx)
    return TIPO_NULO;
  if (ctx->T_KW_INTEIROS())
    return TIPO_INTEIRO;
  if (ctx->T_KW_REAIS())
    return TIPO_REAL;
  if (ctx->T_KW_CARACTERES())
    return TIPO_CARACTERE;
  if (ctx->T_KW_LITERAIS())
    return TIPO_LITERAL;
  if (ctx->T_KW_LOGICOS())
    return TIPO_LOGICO;
  return TIPO_NULO;
}

int SemanticAnalyzer::tipoDecl(PortugolParser::Tipo_declContext *ctx) {
  if (auto p = dynamic_cast<PortugolParser::TipoPrimitivoContext *>(ctx)) {
    return tipoPrim(p->tp_prim());
  }
  if (auto m = dynamic_cast<PortugolParser::TipoMatrizContext *>(ctx)) {
    return tipoPrimPl(m->tp_matriz()->tp_prim_pl());
  }
  return TIPO_NULO;
}

list<int> SemanticAnalyzer::dimensoes(PortugolParser::Tipo_declContext *ctx) {
  list<int> dims;
  if (auto m = dynamic_cast<PortugolParser::TipoMatrizContext *>(ctx)) {
    for (auto d : m->tp_matriz()->dimensoes()->T_INT_LIT()) {
      dims.push_back(atoi(d->getText().c_str()));
    }
  }
  return dims;
}

ExpressionValue SemanticAnalyzer::annotate(ParserRuleContext *ctx,
                                           ExpressionValue v) {
  _stable.setEvalType(ctx, v.primitiveType());
  return v;
}

void SemanticAnalyzer::analyze(PortugolParser::AlgoritmoContext *ctx) {
  if (!ctx) {
    return;
  }

  evaluator.setCurrentScope(SymbolTable::GlobalScope);

  if (ctx->var_decl_block()) {
    variaveis(ctx->var_decl_block()->var_decl());
  }

  // Function prototypes must be known before the main block is checked.
  for (auto f : ctx->func_decls()) {
    funcProto(f);
  }

  evaluator.setCurrentScope(SymbolTable::GlobalScope);
  if (ctx->stm_block()) {
    stmList(ctx->stm_block()->stm_list());
  }

  for (auto f : ctx->func_decls()) {
    funcDecl(f);
  }

  evaluator.setCurrentScope(SymbolTable::GlobalScope);
}

void SemanticAnalyzer::variaveis(
    const vector<PortugolParser::Var_declContext *> &decls) {
  for (auto decl : decls) {
    list<TokenRef> ids;
    for (auto id : decl->T_IDENTIFICADOR()) {
      ids.push_back(ref(id));
    }
    int type = tipoDecl(decl->tipo_decl());
    if (dynamic_cast<PortugolParser::TipoMatrizContext *>(decl->tipo_decl())) {
      evaluator.declareVars(type, dimensoes(decl->tipo_decl()), ids);
    } else {
      evaluator.declareVars(type, ids);
    }
  }
}

void SemanticAnalyzer::funcProto(PortugolParser::Func_declsContext *ctx) {
  Funcao f;
  f.setId(ref(ctx->T_IDENTIFICADOR()));

  if (ctx->fparams()) {
    for (auto p : ctx->fparams()->fparam()) {
      int type = tipoDecl(p->tipo_decl());
      if (dynamic_cast<PortugolParser::TipoMatrizContext *>(p->tipo_decl())) {
        f.addParam(ref(p->T_IDENTIFICADOR()), type, dimensoes(p->tipo_decl()));
      } else {
        f.addParam(ref(p->T_IDENTIFICADOR()), type);
      }
    }
  }

  if (auto rt = dynamic_cast<PortugolParser::RettypeComTipoContext *>(
          ctx->rettype())) {
    f.setReturnType(tipoPrim(rt->tp_prim()));
  } else {
    f.setReturnType(TIPO_NULO); /*void*/
  }

  // Parameters live in the function's own scope.
  evaluator.setCurrentScope(ctx->T_IDENTIFICADOR()->getText());
  evaluator.declareFunction(f);
}

void SemanticAnalyzer::funcDecl(PortugolParser::Func_declsContext *ctx) {
  evaluator.setCurrentScope(ctx->T_IDENTIFICADOR()->getText());

  if (auto vars = dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(
          ctx->fvar_decl())) {
    variaveis(vars->var_decl());
  }

  if (ctx->stm_block()) {
    stmList(ctx->stm_block()->stm_list());
  }

  evaluator.setCurrentScope(SymbolTable::GlobalScope);
}

void SemanticAnalyzer::stmList(PortugolParser::Stm_listContext *ctx) {
  if (!ctx) {
    return;
  }
  for (auto s : ctx->stm()) {
    stm(s);
  }
}

void SemanticAnalyzer::stm(PortugolParser::StmContext *ctx) {
  if (auto s = dynamic_cast<PortugolParser::StmAtribuicaoContext *>(ctx)) {
    auto attr = s->stm_attr();
    ExpressionValue ltype = lvalue(attr->lvalue());
    ExpressionValue etype = expr(attr->expr());
    evaluator.evaluateAttribution(ltype, etype,
                                  (int)attr->T_ATTR()->getSymbol()->getLine());

  } else if (auto s =
                 dynamic_cast<PortugolParser::StmChamadaFuncContext *>(ctx)) {
    fcall(s->fcall());

  } else if (auto s = dynamic_cast<PortugolParser::StmRetornoContext *>(ctx)) {
    ExpressionValue etype; // TIPO_NULO
    int line;
    if (auto r = dynamic_cast<PortugolParser::RetorneComExprContext *>(
            s->stm_ret())) {
      etype = expr(r->expr());
      line = (int)r->T_KW_RETORNE()->getSymbol()->getLine();
    } else {
      auto r0 =
          static_cast<PortugolParser::RetorneSemExprContext *>(s->stm_ret());
      line = (int)r0->T_KW_RETORNE()->getSymbol()->getLine();
    }
    evaluator.evaluateReturnCmd(etype, line);

  } else if (auto s =
                 dynamic_cast<PortugolParser::StmCondicionalContext *>(ctx)) {
    auto se = s->stm_se();
    ExpressionValue etype = expr(se->expr());
    evaluator.evaluateBooleanExpr(etype,
                                  (int)se->T_KW_SE()->getSymbol()->getLine());
    stmList(se->stm_list());
    if (se->senao_part()) {
      stmList(se->senao_part()->stm_list());
    }

  } else if (auto s = dynamic_cast<PortugolParser::StmEnquantoContext *>(ctx)) {
    auto enq = s->stm_enquanto();
    ExpressionValue etype = expr(enq->expr());
    evaluator.evaluateBooleanExpr(
        etype, (int)enq->T_KW_ENQUANTO()->getSymbol()->getLine());
    stmList(enq->stm_list());

  } else if (auto s = dynamic_cast<PortugolParser::StmRepitaContext *>(ctx)) {
    auto rep = s->stm_repita();
    stmList(rep->stm_list());
    ExpressionValue etype = expr(rep->expr());
    evaluator.evaluateBooleanExpr(
        etype, (int)rep->T_KW_REPITA()->getSymbol()->getLine());

  } else if (auto s = dynamic_cast<PortugolParser::StmParaContext *>(ctx)) {
    auto para = s->stm_para();
    int line = (int)para->T_KW_PARA()->getSymbol()->getLine();

    ExpressionValue lv = lvalue(para->lvalue());
    evaluator.evaluateParaExpr(lv, line, lv.id());

    ExpressionValue de = expr(para->inicio);
    evaluator.evaluateParaExpr(de, line, "de");

    ExpressionValue ate = expr(para->fim);
    evaluator.evaluateParaExpr(ate, line, "até");

    if (para->passo()) {
      TerminalNode *i = para->passo()->T_INT_LIT();
      evaluator.evaluatePasso((int)i->getSymbol()->getLine(), i->getText());
    }

    stmList(para->stm_list());
  }
}

template <typename Ctx, typename Fn>
ExpressionValue SemanticAnalyzer::binaryChain(Ctx *ctx, Fn operand) {
  ExpressionValue left;
  TerminalNode *op = nullptr;
  bool first = true;

  for (auto child : ctx->children) {
    if (auto t = dynamic_cast<TerminalNode *>(child)) {
      op = t;
    } else if (auto rule = dynamic_cast<ParserRuleContext *>(child)) {
      if (first) {
        left = operand(rule);
        first = false;
      } else {
        ExpressionValue right = operand(rule);
        left = evaluator.evaluateExpr(left, right, ref(op));
      }
    }
  }
  return annotate(ctx, left);
}

ExpressionValue SemanticAnalyzer::expr(PortugolParser::ExprContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprE(static_cast<PortugolParser::Expr_eContext *>(c));
  });
}

ExpressionValue SemanticAnalyzer::exprE(PortugolParser::Expr_eContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprBitOu(static_cast<PortugolParser::Expr_bit_ouContext *>(c));
  });
}

ExpressionValue
SemanticAnalyzer::exprBitOu(PortugolParser::Expr_bit_ouContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprBitXou(static_cast<PortugolParser::Expr_bit_xouContext *>(c));
  });
}

ExpressionValue
SemanticAnalyzer::exprBitXou(PortugolParser::Expr_bit_xouContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprBitE(static_cast<PortugolParser::Expr_bit_eContext *>(c));
  });
}

ExpressionValue
SemanticAnalyzer::exprBitE(PortugolParser::Expr_bit_eContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprIgual(static_cast<PortugolParser::Expr_igualContext *>(c));
  });
}

ExpressionValue
SemanticAnalyzer::exprIgual(PortugolParser::Expr_igualContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprRelacional(
        static_cast<PortugolParser::Expr_relacionalContext *>(c));
  });
}

ExpressionValue
SemanticAnalyzer::exprRelacional(PortugolParser::Expr_relacionalContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprAd(static_cast<PortugolParser::Expr_adContext *>(c));
  });
}

ExpressionValue SemanticAnalyzer::exprAd(PortugolParser::Expr_adContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprMultip(static_cast<PortugolParser::Expr_multipContext *>(c));
  });
}

ExpressionValue
SemanticAnalyzer::exprMultip(PortugolParser::Expr_multipContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprUnario(static_cast<PortugolParser::Expr_unarioContext *>(c));
  });
}

ExpressionValue
SemanticAnalyzer::exprUnario(PortugolParser::Expr_unarioContext *ctx) {
  ExpressionValue v;
  if (auto u = dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    ExpressionValue e = exprElemento(u->expr_elemento());
    v = evaluator.evaluateExpr(e, SemanticEval::UN_NEG, ref(u->T_MENOS()));
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    ExpressionValue e = exprElemento(u->expr_elemento());
    v = evaluator.evaluateExpr(e, SemanticEval::UN_POS, ref(u->T_MAIS()));
  } else if (auto u = dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    ExpressionValue e = exprElemento(u->expr_elemento());
    v = evaluator.evaluateExpr(e, SemanticEval::UN_NOT, ref(u->T_KW_NOT()));
  } else if (auto u = dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    ExpressionValue e = exprElemento(u->expr_elemento());
    v = evaluator.evaluateExpr(e, SemanticEval::UN_BNOT, ref(u->T_BIT_NOT()));
  } else if (auto u =
                 dynamic_cast<PortugolParser::ExprElementoContext *>(ctx)) {
    v = exprElemento(u->expr_elemento());
  }
  return annotate(ctx, v);
}

ExpressionValue
SemanticAnalyzer::exprElemento(PortugolParser::Expr_elementoContext *ctx) {
  ExpressionValue v;
  if (auto e = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    v = fcall(e->fcall());
  } else if (auto e = dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    v = lvalue(e->lvalue());
  } else if (auto e = dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    v = literal(e->literal());
  } else if (auto e =
                 dynamic_cast<PortugolParser::ElemParentesesContext *>(ctx)) {
    v = expr(e->expr());
  }
  return annotate(ctx, v);
}

ExpressionValue SemanticAnalyzer::literal(PortugolParser::LiteralContext *ctx) {
  ExpressionValue type;
  type.setPrimitive(true);
  if (dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    type.setPrimitiveType(TIPO_LITERAL);
  } else if (dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    type.setPrimitiveType(TIPO_INTEIRO);
  } else if (dynamic_cast<PortugolParser::LitRealContext *>(ctx)) {
    type.setPrimitiveType(TIPO_REAL);
  } else if (dynamic_cast<PortugolParser::LitCaractereContext *>(ctx)) {
    type.setPrimitiveType(TIPO_CARACTERE);
  } else { // verdadeiro | falso
    type.setPrimitiveType(TIPO_LOGICO);
  }
  return annotate(ctx, type);
}

ExpressionValue SemanticAnalyzer::fcall(PortugolParser::FcallContext *ctx) {
  list<ExpressionValue> args;
  if (ctx->fargs()) {
    for (auto e : ctx->fargs()->expr()) {
      args.push_back(expr(e));
    }
  }
  return annotate(ctx,
                  evaluator.evaluateFCall(ref(ctx->T_IDENTIFICADOR()), args));
}

ExpressionValue SemanticAnalyzer::lvalue(PortugolParser::LvalueContext *ctx) {
  list<ExpressionValue> dimensions;
  if (ctx->array_sub()) {
    for (auto e : ctx->array_sub()->expr()) {
      dimensions.push_back(expr(e));
    }
  }
  return annotate(
      ctx, evaluator.evaluateLValue(ref(ctx->T_IDENTIFICADOR()), dimensions));
}
