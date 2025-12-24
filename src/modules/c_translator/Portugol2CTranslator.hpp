/*
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 */

#ifndef PORTUGOL2CTRANSLATOR_HPP
#define PORTUGOL2CTRANSLATOR_HPP

#include "PortugolBaseVisitor.h"
#include "SymbolTable.hpp"
#include <sstream>
#include <string>

class Portugol2CTranslator : public PortugolBaseVisitor {
public:
  Portugol2CTranslator(SymbolTable &st);

  std::string translate(PortugolParser::AlgoritmoContext *tree);

  // Visitor overrides
  virtual antlrcpp::Any
  visitAlgoritmo(PortugolParser::AlgoritmoContext *ctx) override;
  virtual antlrcpp::Any
  visitVar_decl_block(PortugolParser::Var_decl_blockContext *ctx) override;
  virtual antlrcpp::Any
  visitVar_decl(PortugolParser::Var_declContext *ctx) override;
  virtual antlrcpp::Any
  visitStm_block(PortugolParser::Stm_blockContext *ctx) override;
  virtual antlrcpp::Any
  visitStmAtribuicao(PortugolParser::StmAtribuicaoContext *ctx) override;
  virtual antlrcpp::Any
  visitStmChamadaFunc(PortugolParser::StmChamadaFuncContext *ctx) override;
  virtual antlrcpp::Any
  visitStmRetorno(PortugolParser::StmRetornoContext *ctx) override;
  virtual antlrcpp::Any
  visitStmCondicional(PortugolParser::StmCondicionalContext *ctx) override;
  virtual antlrcpp::Any
  visitStmEnquanto(PortugolParser::StmEnquantoContext *ctx) override;
  virtual antlrcpp::Any
  visitStmRepita(PortugolParser::StmRepitaContext *ctx) override;
  virtual antlrcpp::Any
  visitStmPara(PortugolParser::StmParaContext *ctx) override;
  virtual antlrcpp::Any
  visitFunc_decls(PortugolParser::Func_declsContext *ctx) override;

private:
  SymbolTable &_stable;
  std::stringstream _output;
  std::stringstream _funcDecls;
  int _indentLevel;

  void indent();
  std::string getIndent();
  void dedent();

  // Type helpers
  std::string translateType(int type);
  int getTypeFromTpPrim(PortugolParser::Tp_primContext *ctx);
  int getTypeFromTpPrimPl(PortugolParser::Tp_prim_plContext *ctx);

  // Expression translators
  std::string translateExpr(PortugolParser::ExprContext *ctx);
  std::string translateExprE(PortugolParser::Expr_eContext *ctx);
  std::string translateExprBitOu(PortugolParser::Expr_bit_ouContext *ctx);
  std::string translateExprBitXou(PortugolParser::Expr_bit_xouContext *ctx);
  std::string translateExprBitE(PortugolParser::Expr_bit_eContext *ctx);
  std::string translateExprIgual(PortugolParser::Expr_igualContext *ctx);
  std::string
  translateExprRelacional(PortugolParser::Expr_relacionalContext *ctx);
  std::string translateExprAd(PortugolParser::Expr_adContext *ctx);
  std::string translateExprMultip(PortugolParser::Expr_multipContext *ctx);
  std::string translateExprUnario(PortugolParser::Expr_unarioContext *ctx);
  std::string translateExprElemento(PortugolParser::Expr_elementoContext *ctx);
  std::string translateFcall(PortugolParser::FcallContext *ctx);
  std::string translateLvalue(PortugolParser::LvalueContext *ctx);
  std::string translateLiteral(PortugolParser::LiteralContext *ctx);
  std::string convertIntLiteral(const std::string &lit);
};

#endif // PORTUGOL2CTRANSLATOR_HPP
