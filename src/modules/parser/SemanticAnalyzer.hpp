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

#include "PortugolBaseVisitor.h"
#include "SemanticEval.hpp"
#include "SymbolTable.hpp"
#include <string>

class SemanticAnalyzer : public PortugolBaseVisitor {
public:
  SemanticAnalyzer(SymbolTable &st, const std::string &sourceContent = "");

  void analyze(PortugolParser::AlgoritmoContext *tree);

  // Visitor overrides
  virtual antlrcpp::Any
  visitAlgoritmo(PortugolParser::AlgoritmoContext *ctx) override;
  virtual antlrcpp::Any visitDeclaracao_algoritmo(
      PortugolParser::Declaracao_algoritmoContext *ctx) override;
  virtual antlrcpp::Any
  visitVar_decl_block(PortugolParser::Var_decl_blockContext *ctx) override;
  virtual antlrcpp::Any
  visitVar_decl(PortugolParser::Var_declContext *ctx) override;
  virtual antlrcpp::Any
  visitTipoPrimitivo(PortugolParser::TipoPrimitivoContext *ctx) override;
  virtual antlrcpp::Any
  visitTipoMatriz(PortugolParser::TipoMatrizContext *ctx) override;
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
  virtual antlrcpp::Any visitFcall(PortugolParser::FcallContext *ctx) override;
  virtual antlrcpp::Any
  visitFunc_decls(PortugolParser::Func_declsContext *ctx) override;
  virtual antlrcpp::Any visitExpr(PortugolParser::ExprContext *ctx) override;
  virtual antlrcpp::Any
  visitLvalue(PortugolParser::LvalueContext *ctx) override;

private:
  SemanticEval evaluator;
  std::string _sourceContent;

  // Helper to get type from tipo_decl
  int getTypeFromTipoDecl(PortugolParser::Tipo_declContext *ctx);
  int getTypeFromTpPrim(PortugolParser::Tp_primContext *ctx);

  // Current context tracking
  std::string _currentFunction;
  bool _inFunction;
};

#endif // SEMANTICANALYZER_HPP
