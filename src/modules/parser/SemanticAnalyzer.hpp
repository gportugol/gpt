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

#ifndef SEMANTICANALYZER_HPP
#define SEMANTICANALYZER_HPP

#include "PortugolParser.h"
#include "SemanticEval.hpp"
#include "SymbolTable.hpp"

#include <list>
#include <string>

/*
 * Semantic analysis over the ANTLR4 parse tree.
 *
 * Port of the ANTLR2 SemanticWalker (semantic.g): declares the symbols,
 * validates types, arguments and return values through SemanticEval and
 * records the evaluated type of every expression node with
 * SymbolTable::setEvalType() for the code generators.
 */
class SemanticAnalyzer {
public:
  SemanticAnalyzer(SymbolTable &st);

  void analyze(PortugolParser::AlgoritmoContext *tree);

  static int tipoPrim(PortugolParser::Tp_primContext *ctx);
  static int tipoPrimPl(PortugolParser::Tp_prim_plContext *ctx);
  static int tipoDecl(PortugolParser::Tipo_declContext *ctx);
  static std::list<int> dimensoes(PortugolParser::Tipo_declContext *ctx);

private:
  void variaveis(const std::vector<PortugolParser::Var_declContext *> &decls);
  void funcProto(PortugolParser::Func_declsContext *ctx);
  void funcDecl(PortugolParser::Func_declsContext *ctx);

  void stmList(PortugolParser::Stm_listContext *ctx);
  void stm(PortugolParser::StmContext *ctx);

  ExpressionValue expr(PortugolParser::ExprContext *ctx);
  ExpressionValue exprE(PortugolParser::Expr_eContext *ctx);
  ExpressionValue exprBitOu(PortugolParser::Expr_bit_ouContext *ctx);
  ExpressionValue exprBitXou(PortugolParser::Expr_bit_xouContext *ctx);
  ExpressionValue exprBitE(PortugolParser::Expr_bit_eContext *ctx);
  ExpressionValue exprIgual(PortugolParser::Expr_igualContext *ctx);
  ExpressionValue exprRelacional(PortugolParser::Expr_relacionalContext *ctx);
  ExpressionValue exprAd(PortugolParser::Expr_adContext *ctx);
  ExpressionValue exprMultip(PortugolParser::Expr_multipContext *ctx);
  ExpressionValue exprUnario(PortugolParser::Expr_unarioContext *ctx);
  ExpressionValue exprElemento(PortugolParser::Expr_elementoContext *ctx);
  ExpressionValue literal(PortugolParser::LiteralContext *ctx);
  ExpressionValue fcall(PortugolParser::FcallContext *ctx);
  ExpressionValue lvalue(PortugolParser::LvalueContext *ctx);

  // Folds a flat "operand (op operand)*" chain left to right, like the
  // left-associative nested nodes of the ANTLR2 AST.
  template <typename Ctx, typename Fn>
  ExpressionValue binaryChain(Ctx *ctx, Fn operand);

  ExpressionValue annotate(antlr4::ParserRuleContext *ctx, ExpressionValue v);

  static TokenRef ref(antlr4::tree::TerminalNode *node);

  SemanticEval evaluator;
  SymbolTable &_stable;
};

#endif // SEMANTICANALYZER_HPP
