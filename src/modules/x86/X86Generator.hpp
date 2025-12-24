/*
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 */

#ifndef X86GENERATOR_HPP
#define X86GENERATOR_HPP

#include "PortugolBaseVisitor.h"
#include "SymbolTable.hpp"
#include "X86.hpp"
#include <list>
#include <string>
#include <utility>

class X86Generator : public PortugolBaseVisitor {
public:
  X86Generator(SymbolTable &st);

  std::string generate(PortugolParser::AlgoritmoContext *tree);

  // Visitor methods
  antlrcpp::Any visitAlgoritmo(PortugolParser::AlgoritmoContext *ctx) override;
  antlrcpp::Any
  visitVar_decl_block(PortugolParser::Var_decl_blockContext *ctx) override;
  antlrcpp::Any visitVar_decl(PortugolParser::Var_declContext *ctx) override;
  antlrcpp::Any visitStm_block(PortugolParser::Stm_blockContext *ctx) override;
  antlrcpp::Any
  visitStmAtribuicao(PortugolParser::StmAtribuicaoContext *ctx) override;
  antlrcpp::Any
  visitStmChamadaFunc(PortugolParser::StmChamadaFuncContext *ctx) override;
  antlrcpp::Any
  visitStmRetorno(PortugolParser::StmRetornoContext *ctx) override;
  antlrcpp::Any
  visitStmCondicional(PortugolParser::StmCondicionalContext *ctx) override;
  antlrcpp::Any
  visitStmEnquanto(PortugolParser::StmEnquantoContext *ctx) override;
  antlrcpp::Any visitStmRepita(PortugolParser::StmRepitaContext *ctx) override;
  antlrcpp::Any visitStmPara(PortugolParser::StmParaContext *ctx) override;
  antlrcpp::Any
  visitFunc_decls(PortugolParser::Func_declsContext *ctx) override;

private:
  SymbolTable &_stable;
  X86 _x86;
  int _declType; // VAR_GLOBAL, VAR_PARAM, VAR_LOCAL

  // Helper methods
  int getTypeFromTpPrim(PortugolParser::Tp_primContext *ctx);
  int getTypeFromTpPrimPl(PortugolParser::Tp_prim_plContext *ctx);
  std::list<std::string> getDimensions(PortugolParser::DimensoesContext *ctx);

  // Expression evaluation - returns type
  int evaluateExpr(PortugolParser::ExprContext *ctx, int expectingType);
  int evaluateExprE(PortugolParser::Expr_eContext *ctx, int expectingType);
  int evaluateExprBitOu(PortugolParser::Expr_bit_ouContext *ctx,
                        int expectingType);
  int evaluateExprBitXou(PortugolParser::Expr_bit_xouContext *ctx,
                         int expectingType);
  int evaluateExprBitE(PortugolParser::Expr_bit_eContext *ctx,
                       int expectingType);
  int evaluateExprIgual(PortugolParser::Expr_igualContext *ctx,
                        int expectingType);
  int evaluateExprRelacional(PortugolParser::Expr_relacionalContext *ctx,
                             int expectingType);
  int evaluateExprAd(PortugolParser::Expr_adContext *ctx, int expectingType);
  int evaluateExprMultip(PortugolParser::Expr_multipContext *ctx,
                         int expectingType);
  int evaluateExprUnario(PortugolParser::Expr_unarioContext *ctx,
                         int expectingType);
  int evaluateExprElemento(PortugolParser::Expr_elementoContext *ctx,
                           int expectingType);
  int evaluateFcall(PortugolParser::FcallContext *ctx, int expectingType);
  std::pair<int, std::string>
  evaluateLiteral(PortugolParser::LiteralContext *ctx);

  // lvalue returns pair<pair<type, usingAddr>, name>
  std::pair<std::pair<int, bool>, std::string>
  evaluateLvalue(PortugolParser::LvalueContext *ctx);

  int calcMatrixOffset(int c, std::list<int> &dims);
};

#endif // X86GENERATOR_HPP
