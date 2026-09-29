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

#include "Interpreter.hpp"

#include "GPTDisplay.hpp"
#include "config.h" // após os cabeçalhos do ANTLR4 (VERSION conflita)

#include <sstream>
#include <stdlib.h>

using antlr4::ParserRuleContext;
using antlr4::tree::TerminalNode;

Interpreter::Interpreter(SymbolTable &st, const std::string &host, int port)
    : interpreter(st, host, port), _returning(false) {}

std::string Interpreter::parseLiteral(std::string str) {
  std::string::size_type idx = 0;
  char c;
  while ((idx = str.find('\\', idx)) != std::string::npos) {
    switch (str[idx + 1]) {
    case 'n':
      c = '\n';
      break;
    case 't':
      c = '\t';
      break;
    case 'r':
      c = '\r';
      break;
    case '\\':
      c = '\\';
      break;
    case '\'':
      c = '\'';
      break;
    case '"':
      c = '"';
      break;
    default:
      c = str[idx + 1];
    }
    str.replace(idx, 2, 1, c);
    idx++;
  }
  return str;
}

// Numeric code of a character literal:
//   ''    => 0
//   'a'   => a
//   '\t'  => \t
//   '\n'  => \n
//   '\r'  => \r
//   '\i'  => i
std::string Interpreter::parseChar(std::string str) {
  std::stringstream ret;
  if (str.empty()) {
    return "0";
  }
  if (str[0] == '\\') {
    switch (str[1]) {
    case 't':
      ret << (int)'\t';
      break;
    case 'n':
      ret << (int)'\n';
      break;
    case 'r':
      ret << (int)'\r';
      break;
    default:
      ret << (int)str[1];
    }
    return ret.str();
  } else {
    ret << (int)str[0];
    return ret.str();
  }
}

// Integer literals reach the tree with the decimal text produced by
// validateTokens(); strtoul keeps the ANTLR2 overflow behaviour.
std::string Interpreter::intLiteral(const std::string &text) {
  if ((text.length() > 2) && (text[0] == '0')) {
    int base10;
    switch (text[1]) {
    case 'x':
    case 'X':
      base10 = strtoul(text.c_str(), NULL, 16);
      break;
    case 'c':
    case 'C':
      base10 = strtoul(text.substr(2).c_str(), NULL, 8);
      break;
    case 'b':
    case 'B':
      base10 = strtoul(text.substr(2).c_str(), NULL, 2);
      break;
    default:
      return text;
    }
    std::stringstream s;
    s << base10;
    return s.str();
  }
  return text;
}

// The ANTLR2 lexer stripped the quotes of string and character literals.
std::string Interpreter::stripQuotes(const std::string &text) {
  if (text.length() >= 2) {
    return text.substr(1, text.length() - 2);
  }
  return text;
}

void Interpreter::mapLine(int line, std::string &file, int &localLine) {
  file = GPTDisplay::self()->getCurrentFile();
  GPTDisplay::self()->mapLine(line, file, localLine);
}

void Interpreter::nextCmd(int line) {
  std::string file;
  int localLine;
  mapLine(line, file, localLine);
  interpreter.nextCmd(file, localLine);
}

// Line of the statement's root node in the ANTLR2 AST (T_ATTR, TI_FCALL,
// T_KW_*), which is what the debugger receives.
int Interpreter::stmLine(PortugolParser::StmContext *ctx) {
  if (auto a = dynamic_cast<PortugolParser::StmAtribuicaoContext *>(ctx)) {
    return a->stm_attr()->T_ATTR()->getSymbol()->getLine();
  }
  if (auto f = dynamic_cast<PortugolParser::StmChamadaFuncContext *>(ctx)) {
    return f->fcall()->T_IDENTIFICADOR()->getSymbol()->getLine();
  }
  return ctx->getStart()->getLine();
}

int Interpreter::run(PortugolParser::AlgoritmoContext *tree) {
  int ret = 0;
  if (!tree) {
    return ret;
  }

  _returning = false;
  _functions.clear();
  for (auto f : tree->func_decls()) {
    _functions[f->T_IDENTIFICADOR()->getText()] = f;
  }

  std::string file;
  int localLine;
  mapLine(tree->getStart()->getLine(), file, localLine);
  interpreter.init(file);

  inicio(tree->stm_block());

  if (_returning) {
    ret = interpreter.getReturning();
  }
  return ret;
}

void Interpreter::inicio(PortugolParser::Stm_blockContext *ctx) {
  stmList(ctx->stm_list());
  nextCmd(ctx->T_KW_FIM()->getSymbol()->getLine());
}

void Interpreter::stm(PortugolParser::StmContext *ctx) {
  nextCmd(stmLine(ctx));

  if (auto a = dynamic_cast<PortugolParser::StmAtribuicaoContext *>(ctx)) {
    stmAttr(a->stm_attr());
  } else if (auto f =
                 dynamic_cast<PortugolParser::StmChamadaFuncContext *>(ctx)) {
    fcall(f->fcall()); // retToDevNull
  } else if (auto r = dynamic_cast<PortugolParser::StmRetornoContext *>(ctx)) {
    stmRet(r->stm_ret());
  } else if (auto s =
                 dynamic_cast<PortugolParser::StmCondicionalContext *>(ctx)) {
    stmSe(s->stm_se());
  } else if (auto e = dynamic_cast<PortugolParser::StmEnquantoContext *>(ctx)) {
    stmEnquanto(e->stm_enquanto());
  } else if (auto rp = dynamic_cast<PortugolParser::StmRepitaContext *>(ctx)) {
    stmRepita(rp->stm_repita());
  } else if (auto p = dynamic_cast<PortugolParser::StmParaContext *>(ctx)) {
    stmPara(p->stm_para());
  }
}

void Interpreter::stmAttr(PortugolParser::Stm_attrContext *ctx) {
  LValue l = lvalue(ctx->lvalue());
  ExprValue v = expr(ctx->expr());
  interpreter.execAttribution(l, v);
}

void Interpreter::stmRet(PortugolParser::Stm_retContext *ctx) {
  ExprValue eval;
  eval.type = TIPO_NULO;
  if (auto r = dynamic_cast<PortugolParser::RetorneComExprContext *>(ctx)) {
    eval = expr(r->expr());
  }
  interpreter.setReturnExprValue(eval);
  _returning = true;
}

// Runs the statements of a block until a "retorne" is executed, so that
// the interpreter leaves nested blocks and loops like the native and C
// code do.
void Interpreter::stmList(PortugolParser::Stm_listContext *ctx) {
  for (auto s : ctx->stm()) {
    if (_returning) {
      return;
    }
    stm(s);
  }
}

void Interpreter::stmSe(PortugolParser::Stm_seContext *ctx) {
  ExprValue e = expr(ctx->expr());
  bool exec = e.ifTrue();

  if (exec) {
    stmList(ctx->stm_list());
  } else if (ctx->senao_part()) {
    stmList(ctx->senao_part()->stm_list());
  }
}

void Interpreter::stmEnquanto(PortugolParser::Stm_enquantoContext *ctx) {
  bool exec = expr(ctx->expr()).ifTrue();

  while (exec && !_returning) {
    stmList(ctx->stm_list());
    if (_returning) {
      return;
    }
    exec = expr(ctx->expr()).ifTrue();
  }
}

void Interpreter::stmRepita(PortugolParser::Stm_repitaContext *ctx) {
  bool exec;
  do {
    stmList(ctx->stm_list());
    if (_returning) {
      return;
    }
    exec = expr(ctx->expr()).ifTrue();
  } while (!exec);
}

void Interpreter::stmPara(PortugolParser::Stm_paraContext *ctx) {
  LValue lv = lvalue(ctx->lvalue());
  ExprValue de = expr(ctx->inicio);
  interpreter.execAttribution(lv, de);
  ExprValue ate = expr(ctx->fim);

  int ps = ctx->passo() ? passo(ctx->passo()) : 1;

  while (true) {
    if (ps > 0) {
      if (!interpreter.execLowerEq(lv, ate))
        break;
    } else {
      if (!interpreter.execBiggerEq(lv, ate))
        break;
    }
    stmList(ctx->stm_list());
    if (_returning) {
      return;
    }
    interpreter.execPasso(lv, ps);
    ate = expr(ctx->fim);
  }

  // lv deve ter um valor a mais do que até (ou a menos, se loop decrescente).
  // setar o valor de lv para valor de ate
  interpreter.execAttribution(lv, ate);
}

int Interpreter::passo(PortugolParser::PassoContext *ctx) {
  int p = atoi(intLiteral(ctx->T_INT_LIT()->getText()).c_str());
  if (ctx->T_MENOS()) {
    p = -p;
  }
  return p;
}

void Interpreter::funcDecls(PortugolParser::Func_declsContext *ctx,
                            std::list<ExprValue> &args, int line) {
  std::string name = ctx->T_IDENTIFICADOR()->getText();
  std::string file;
  int localLine;
  mapLine(ctx->T_IDENTIFICADOR()->getSymbol()->getLine(), file, localLine);

  interpreter.beginFunctionCall(file, name, args, line);
  inicio(ctx->stm_block());
  interpreter.endFunctionCall();
}

template <typename Ctx, typename Fn>
ExprValue Interpreter::binaryChain(Ctx *ctx, Fn operand) {
  ExprValue left;
  left.type = TIPO_NULO;
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
        ExprValue right = operand(rule);
        left = binaryOp(op->getSymbol()->getType(), left, right);
      }
    }
  }
  return left;
}

ExprValue Interpreter::binaryOp(size_t tokenType, ExprValue &left,
                                ExprValue &right) {
  switch (tokenType) {
  case PortugolParser::T_KW_OU:
    return interpreter.evaluateOu(left, right);
  case PortugolParser::T_KW_E:
    return interpreter.evaluateE(left, right);
  case PortugolParser::T_BIT_OU:
    return interpreter.evaluateBitOu(left, right);
  case PortugolParser::T_BIT_XOU:
    return interpreter.evaluateBitXou(left, right);
  case PortugolParser::T_BIT_E:
    return interpreter.evaluateBitE(left, right);
  case PortugolParser::T_IGUAL:
    return interpreter.evaluateIgual(left, right);
  case PortugolParser::T_DIFERENTE:
    return interpreter.evaluateDif(left, right);
  case PortugolParser::T_MAIOR:
    return interpreter.evaluateMaior(left, right);
  case PortugolParser::T_MENOR:
    return interpreter.evaluateMenor(left, right);
  case PortugolParser::T_MAIOR_EQ:
    return interpreter.evaluateMaiorEq(left, right);
  case PortugolParser::T_MENOR_EQ:
    return interpreter.evaluateMenorEq(left, right);
  case PortugolParser::T_MAIS:
    return interpreter.evaluateMais(left, right);
  case PortugolParser::T_MENOS:
    return interpreter.evaluateMenos(left, right);
  case PortugolParser::T_DIV:
    return interpreter.evaluateDiv(left, right);
  case PortugolParser::T_MULTIP:
    return interpreter.evaluateMultip(left, right);
  case PortugolParser::T_MOD:
    return interpreter.evaluateMod(left, right);
  default: {
    std::stringstream s;
    s << PACKAGE << ": BUG: operador desconhecido na expressão." << std::endl;
    GPTDisplay::self()->showError(s);
    return left;
  }
  }
}

ExprValue Interpreter::expr(PortugolParser::ExprContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprE(static_cast<PortugolParser::Expr_eContext *>(c));
  });
}

ExprValue Interpreter::exprE(PortugolParser::Expr_eContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprBitOu(static_cast<PortugolParser::Expr_bit_ouContext *>(c));
  });
}

ExprValue Interpreter::exprBitOu(PortugolParser::Expr_bit_ouContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprBitXou(static_cast<PortugolParser::Expr_bit_xouContext *>(c));
  });
}

ExprValue Interpreter::exprBitXou(PortugolParser::Expr_bit_xouContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprBitE(static_cast<PortugolParser::Expr_bit_eContext *>(c));
  });
}

ExprValue Interpreter::exprBitE(PortugolParser::Expr_bit_eContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprIgual(static_cast<PortugolParser::Expr_igualContext *>(c));
  });
}

ExprValue Interpreter::exprIgual(PortugolParser::Expr_igualContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprRelacional(
        static_cast<PortugolParser::Expr_relacionalContext *>(c));
  });
}

ExprValue
Interpreter::exprRelacional(PortugolParser::Expr_relacionalContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprAd(static_cast<PortugolParser::Expr_adContext *>(c));
  });
}

ExprValue Interpreter::exprAd(PortugolParser::Expr_adContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprMultip(static_cast<PortugolParser::Expr_multipContext *>(c));
  });
}

ExprValue Interpreter::exprMultip(PortugolParser::Expr_multipContext *ctx) {
  return binaryChain(ctx, [this](ParserRuleContext *c) {
    return exprUnario(static_cast<PortugolParser::Expr_unarioContext *>(c));
  });
}

ExprValue Interpreter::exprUnario(PortugolParser::Expr_unarioContext *ctx) {
  if (auto n = dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    ExprValue right = element(n->expr_elemento());
    return interpreter.evaluateUnNeg(right);
  }
  if (auto p = dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    ExprValue right = element(p->expr_elemento());
    return interpreter.evaluateUnPos(right);
  }
  if (auto nt = dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    ExprValue right = element(nt->expr_elemento());
    return interpreter.evaluateUnNot(right);
  }
  if (auto bn = dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    ExprValue right = element(bn->expr_elemento());
    return interpreter.evaluateUnBNot(right);
  }
  return element(
      static_cast<PortugolParser::ExprElementoContext *>(ctx)->expr_elemento());
}

ExprValue Interpreter::element(PortugolParser::Expr_elementoContext *ctx) {
  if (auto l = dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    return literal(l->literal());
  }
  if (auto f = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    return fcall(f->fcall());
  }
  if (auto lv = dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    LValue l = lvalue(lv->lvalue());
    return interpreter.getLValueValue(l);
  }
  return expr(
      static_cast<PortugolParser::ElemParentesesContext *>(ctx)->expr());
}

ExprValue Interpreter::literal(PortugolParser::LiteralContext *ctx) {
  ExprValue v;
  if (auto l = dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    v.setValue(parseLiteral(stripQuotes(l->T_STRING_LIT()->getText())));
    v.type = TIPO_LITERAL;
  } else if (auto i = dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    v.setValue(intLiteral(i->T_INT_LIT()->getText()));
    v.type = TIPO_INTEIRO;
  } else if (auto r = dynamic_cast<PortugolParser::LitRealContext *>(ctx)) {
    v.setValue(r->T_REAL_LIT()->getText());
    v.type = TIPO_REAL;
  } else if (auto c =
                 dynamic_cast<PortugolParser::LitCaractereContext *>(ctx)) {
    v.setValue(parseChar(stripQuotes(c->T_CARAC_LIT()->getText())));
    v.type = TIPO_CARACTERE;
  } else if (dynamic_cast<PortugolParser::LitVerdadeiroContext *>(ctx)) {
    v.setValue("1");
    v.type = TIPO_LOGICO;
  } else { // falso
    v.setValue("0");
    v.type = TIPO_LOGICO;
  }
  return v;
}

LValue Interpreter::lvalue(PortugolParser::LvalueContext *ctx) {
  LValue l;
  l.name = ctx->T_IDENTIFICADOR()->getText();
  if (ctx->array_sub()) {
    for (auto e : ctx->array_sub()->expr()) {
      ExprValue v = expr(e);
      l.addMatrixIndex(v);
    }
  }
  return l;
}

ExprValue Interpreter::fcall(PortugolParser::FcallContext *ctx) {
  ExprValue v;
  v.type = TIPO_NULO;
  std::list<ExprValue> args;

  if (ctx->fargs()) {
    for (auto e : ctx->fargs()->expr()) {
      args.push_back(expr(e));
    }
  }

  std::string name = ctx->T_IDENTIFICADOR()->getText();
  if (interpreter.isBuiltInFunction(name)) {
    v = interpreter.execBuiltInFunction(name, args);
  } else {
    std::map<std::string, PortugolParser::Func_declsContext *>::iterator it =
        _functions.find(name);
    if (it == _functions.end()) {
      std::stringstream s;
      s << PACKAGE << ": BUG: função \"" << name << "\" não encontrada."
        << std::endl;
      GPTDisplay::self()->showError(s);
      exit(1);
    }

    std::string file;
    int line;
    mapLine(ctx->T_IDENTIFICADOR()->getSymbol()->getLine(), file, line);

    funcDecls(it->second, args, line); // executes
    _returning = false;
    v = interpreter.getReturnExprValue(name);
  }
  return v;
}
