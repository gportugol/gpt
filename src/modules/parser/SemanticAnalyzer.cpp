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
#include <iostream>
#include <sstream>

SemanticAnalyzer::SemanticAnalyzer(SymbolTable &st,
                                   const std::string &sourceContent)
    : evaluator(st), _sourceContent(sourceContent), _inFunction(false) {
  // Register built-in functions
  evaluator.registrarLeia();
  evaluator.registrarImprima();
}

void SemanticAnalyzer::analyze(PortugolParser::AlgoritmoContext *tree) {
  if (tree) {
    visit(tree);
  }
}

antlrcpp::Any
SemanticAnalyzer::visitAlgoritmo(PortugolParser::AlgoritmoContext *ctx) {
  // Visit algorithm declaration
  if (ctx->declaracao_algoritmo()) {
    visit(ctx->declaracao_algoritmo());
  }

  // Visit variable declarations
  if (ctx->var_decl_block()) {
    visit(ctx->var_decl_block());
  }

  // First pass: register all function prototypes
  for (auto func : ctx->func_decls()) {
    // Register function before visiting body
    std::string funcName = func->T_IDENTIFICADOR()->getText();

    // Get return type
    int retType = TIPO_NULO;
    auto retCtx = func->rettype();
    if (auto rettypeComTipo =
            dynamic_cast<PortugolParser::RettypeComTipoContext *>(retCtx)) {
      retType = getTypeFromTpPrim(rettypeComTipo->tp_prim());
    }

    // Get parameters
    std::list<std::pair<std::string, int>> params;
    auto fparamsCtx = func->fparams();
    if (fparamsCtx) {
      for (auto fp : fparamsCtx->fparam()) {
        std::string paramName = fp->T_IDENTIFICADOR()->getText();
        int paramType = getTypeFromTipoDecl(fp->tipo_decl());
        params.push_back({paramName, paramType});
      }
    }

    try {
      evaluator.declararFuncao(funcName, retType, params,
                               func->getStart()->getLine());
    } catch (SymbolTableException &e) {
      std::stringstream ss;
      ss << func->getStart()->getLine() << ": " << e.getMessage();
      GPTDisplay::self()->showError(ss);
    }
  }

  // Visit main block
  if (ctx->stm_block()) {
    visit(ctx->stm_block());
  }

  // Second pass: visit function bodies
  for (auto func : ctx->func_decls()) {
    visit(func);
  }

  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitDeclaracao_algoritmo(
    PortugolParser::Declaracao_algoritmoContext *ctx) {
  // Algorithm name - just validate it exists
  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitVar_decl_block(
    PortugolParser::Var_decl_blockContext *ctx) {
  for (auto varDecl : ctx->var_decl()) {
    visit(varDecl);
  }
  return nullptr;
}

antlrcpp::Any
SemanticAnalyzer::visitVar_decl(PortugolParser::Var_declContext *ctx) {
  // Get type and dimensions
  int type = getTypeFromTipoDecl(ctx->tipo_decl());
  std::list<int> dimensions;

  // Check if it's a matrix type
  if (auto tipoMatriz =
          dynamic_cast<PortugolParser::TipoMatrizContext *>(ctx->tipo_decl())) {
    auto dims = tipoMatriz->tp_matriz()->dimensoes();
    if (dims) {
      for (auto dimToken : dims->T_INT_LIT()) {
        dimensions.push_back(std::stoi(dimToken->getText()));
      }
    }
  }

  // Get all identifiers
  for (auto id : ctx->T_IDENTIFICADOR()) {
    std::string varName = id->getText();
    try {
      evaluator.declararVariavel(varName, type, dimensions,
                                 id->getSymbol()->getLine());
    } catch (SymbolTableException &e) {
      std::stringstream ss;
      ss << id->getSymbol()->getLine() << ": " << e.getMessage();
      GPTDisplay::self()->showError(ss);
    }
  }

  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitTipoPrimitivo(
    PortugolParser::TipoPrimitivoContext *ctx) {
  return getTypeFromTpPrim(ctx->tp_prim());
}

antlrcpp::Any
SemanticAnalyzer::visitTipoMatriz(PortugolParser::TipoMatrizContext *ctx) {
  // Matrix type - get base type and dimensions
  // For now, just return the base type
  auto tpPrimPl = ctx->tp_matriz()->tp_prim_pl();
  if (tpPrimPl->T_KW_INTEIROS())
    return TIPO_INTEIRO;
  if (tpPrimPl->T_KW_REAIS())
    return TIPO_REAL;
  if (tpPrimPl->T_KW_CARACTERES())
    return TIPO_CARACTERE;
  if (tpPrimPl->T_KW_LITERAIS())
    return TIPO_LITERAL;
  if (tpPrimPl->T_KW_LOGICOS())
    return TIPO_LOGICO;
  return TIPO_NULO;
}

antlrcpp::Any
SemanticAnalyzer::visitStm_block(PortugolParser::Stm_blockContext *ctx) {
  if (ctx->stm_list()) {
    for (auto stm : ctx->stm_list()->stm()) {
      visit(stm);
    }
  }
  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitStmAtribuicao(
    PortugolParser::StmAtribuicaoContext *ctx) {
  auto stmAttr = ctx->stm_attr();
  if (stmAttr) {
    // Validate lvalue
    visit(stmAttr->lvalue());
    // Validate expression
    visit(stmAttr->expr());
  }
  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitStmChamadaFunc(
    PortugolParser::StmChamadaFuncContext *ctx) {
  visit(ctx->fcall());
  return nullptr;
}

antlrcpp::Any
SemanticAnalyzer::visitStmRetorno(PortugolParser::StmRetornoContext *ctx) {
  auto stmRet = ctx->stm_ret();
  if (auto retComExpr =
          dynamic_cast<PortugolParser::RetorneComExprContext *>(stmRet)) {
    visit(retComExpr->expr());
  }
  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitStmCondicional(
    PortugolParser::StmCondicionalContext *ctx) {
  auto stmSe = ctx->stm_se();
  if (stmSe) {
    // Validate condition expression
    visit(stmSe->expr());

    // Validate then block
    if (stmSe->stm_list()) {
      for (auto stm : stmSe->stm_list()->stm()) {
        visit(stm);
      }
    }

    // Validate else block
    if (stmSe->senao_part()) {
      for (auto stm : stmSe->senao_part()->stm_list()->stm()) {
        visit(stm);
      }
    }
  }
  return nullptr;
}

antlrcpp::Any
SemanticAnalyzer::visitStmEnquanto(PortugolParser::StmEnquantoContext *ctx) {
  auto stmEnq = ctx->stm_enquanto();
  if (stmEnq) {
    visit(stmEnq->expr());
    if (stmEnq->stm_list()) {
      for (auto stm : stmEnq->stm_list()->stm()) {
        visit(stm);
      }
    }
  }
  return nullptr;
}

antlrcpp::Any
SemanticAnalyzer::visitStmRepita(PortugolParser::StmRepitaContext *ctx) {
  auto stmRep = ctx->stm_repita();
  if (stmRep) {
    if (stmRep->stm_list()) {
      for (auto stm : stmRep->stm_list()->stm()) {
        visit(stm);
      }
    }
    visit(stmRep->expr());
  }
  return nullptr;
}

antlrcpp::Any
SemanticAnalyzer::visitStmPara(PortugolParser::StmParaContext *ctx) {
  auto stmPara = ctx->stm_para();
  if (stmPara) {
    visit(stmPara->lvalue());
    visit(stmPara->inicio);
    visit(stmPara->fim);
    if (stmPara->stm_list()) {
      for (auto stm : stmPara->stm_list()->stm()) {
        visit(stm);
      }
    }
  }
  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitFcall(PortugolParser::FcallContext *ctx) {
  std::string funcName = ctx->T_IDENTIFICADOR()->getText();

  try {
    // Check if function exists
    evaluator.verificarFuncao(funcName, ctx->getStart()->getLine());
  } catch (SymbolTableException &e) {
    std::stringstream ss;
    ss << ctx->getStart()->getLine() << ": " << e.getMessage();
    GPTDisplay::self()->showError(ss);
  }

  // Validate arguments
  if (ctx->fargs()) {
    for (auto expr : ctx->fargs()->expr()) {
      visit(expr);
    }
  }

  return nullptr;
}

antlrcpp::Any
SemanticAnalyzer::visitFunc_decls(PortugolParser::Func_declsContext *ctx) {
  _currentFunction = ctx->T_IDENTIFICADOR()->getText();
  _inFunction = true;

  // Enter function scope
  evaluator.entrarEscopo(_currentFunction);

  // Register parameters as local variables
  if (ctx->fparams()) {
    for (auto fp : ctx->fparams()->fparam()) {
      std::string paramName = fp->T_IDENTIFICADOR()->getText();
      int paramType = getTypeFromTipoDecl(fp->tipo_decl());
      std::list<int> paramDims;

      // Check if parameter is a matrix
      if (auto tipoMatriz = dynamic_cast<PortugolParser::TipoMatrizContext *>(
              fp->tipo_decl())) {
        auto dims = tipoMatriz->tp_matriz()->dimensoes();
        if (dims) {
          for (auto dimToken : dims->T_INT_LIT()) {
            paramDims.push_back(std::stoi(dimToken->getText()));
          }
        }
      }

      try {
        evaluator.declararVariavel(paramName, paramType, paramDims,
                                   fp->getStart()->getLine());
      } catch (SymbolTableException &e) {
        std::stringstream ss;
        ss << fp->getStart()->getLine() << ": " << e.getMessage();
        GPTDisplay::self()->showError(ss);
      }
    }
  }

  // Visit local variable declarations
  if (auto fvarDecl = dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(
          ctx->fvar_decl())) {
    for (auto varDecl : fvarDecl->var_decl()) {
      visit(varDecl);
    }
  }

  // Visit function body
  visit(ctx->stm_block());

  // Exit function scope
  evaluator.sairEscopo();

  _inFunction = false;
  _currentFunction.clear();

  return nullptr;
}

antlrcpp::Any SemanticAnalyzer::visitExpr(PortugolParser::ExprContext *ctx) {
  // Visit all sub-expressions
  for (auto exprE : ctx->expr_e()) {
    visit(exprE);
  }
  return nullptr;
}

antlrcpp::Any
SemanticAnalyzer::visitLvalue(PortugolParser::LvalueContext *ctx) {
  std::string varName = ctx->T_IDENTIFICADOR()->getText();

  try {
    evaluator.verificarVariavel(varName, ctx->getStart()->getLine());
  } catch (SymbolTableException &e) {
    std::stringstream ss;
    ss << ctx->getStart()->getLine() << ": " << e.getMessage();
    GPTDisplay::self()->showError(ss);
  }

  // Validate array subscripts
  if (ctx->array_sub()) {
    for (auto expr : ctx->array_sub()->expr()) {
      visit(expr);
    }
  }

  return nullptr;
}

int SemanticAnalyzer::getTypeFromTipoDecl(
    PortugolParser::Tipo_declContext *ctx) {
  if (auto tipoPrim =
          dynamic_cast<PortugolParser::TipoPrimitivoContext *>(ctx)) {
    return getTypeFromTpPrim(tipoPrim->tp_prim());
  } else if (auto tipoMatriz =
                 dynamic_cast<PortugolParser::TipoMatrizContext *>(ctx)) {
    // Return base type for matrix
    auto tpPrimPl = tipoMatriz->tp_matriz()->tp_prim_pl();
    if (tpPrimPl->T_KW_INTEIROS())
      return TIPO_INTEIRO;
    if (tpPrimPl->T_KW_REAIS())
      return TIPO_REAL;
    if (tpPrimPl->T_KW_CARACTERES())
      return TIPO_CARACTERE;
    if (tpPrimPl->T_KW_LITERAIS())
      return TIPO_LITERAL;
    if (tpPrimPl->T_KW_LOGICOS())
      return TIPO_LOGICO;
  }
  return TIPO_NULO;
}

int SemanticAnalyzer::getTypeFromTpPrim(PortugolParser::Tp_primContext *ctx) {
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
