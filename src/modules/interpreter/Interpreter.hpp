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

#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "InterpreterEval.hpp"
#include "PortugolBaseVisitor.h"
#include "SymbolTable.hpp"
#include <map>
#include <string>
#include <variant>
#include <vector>

// Value type for interpreter
using Value = std::variant<int, double, std::string, bool>;

class Interpreter : public PortugolBaseVisitor {
public:
  Interpreter(SymbolTable &st, const std::string &host = "", int port = 0);

  int run(PortugolParser::AlgoritmoContext *tree);

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

  // Expression evaluation
  Value evaluateExpr(PortugolParser::ExprContext *ctx);
  Value evaluateExprE(PortugolParser::Expr_eContext *ctx);
  Value evaluateExprBitOu(PortugolParser::Expr_bit_ouContext *ctx);
  Value evaluateExprBitXou(PortugolParser::Expr_bit_xouContext *ctx);
  Value evaluateExprBitE(PortugolParser::Expr_bit_eContext *ctx);
  Value evaluateExprIgual(PortugolParser::Expr_igualContext *ctx);
  Value evaluateExprRelacional(PortugolParser::Expr_relacionalContext *ctx);
  Value evaluateExprAd(PortugolParser::Expr_adContext *ctx);
  Value evaluateExprMultip(PortugolParser::Expr_multipContext *ctx);
  Value evaluateExprUnario(PortugolParser::Expr_unarioContext *ctx);
  Value evaluateExprElemento(PortugolParser::Expr_elementoContext *ctx);
  Value evaluateFcall(PortugolParser::FcallContext *ctx);
  Value evaluateLvalue(PortugolParser::LvalueContext *ctx);
  Value evaluateLiteral(PortugolParser::LiteralContext *ctx);

private:
  InterpreterEval interpreter;
  SymbolTable &_stable;

  // Variable storage
  std::map<std::string, Value> _variables;

  // Array storage with dimensions for multi-dimensional support
  struct ArrayInfo {
    std::vector<Value> data;
    std::vector<int> dimensions;
  };
  std::map<std::string, ArrayInfo> _arrays;

  // Function storage
  std::map<std::string, PortugolParser::Func_declsContext *> _functions;

  // Execution state
  bool _returning;
  Value _returnValue;
  int _exitCode;

  // Scope management
  std::vector<std::map<std::string, Value>> _scopeStack;
  std::vector<std::map<std::string, ArrayInfo>> _arrayStack;

  void enterScope();
  void exitScope();
  void setVariable(const std::string &name, const Value &value);
  Value getVariable(const std::string &name);

  // Array support (multi-dimensional)
  void setArrayElement(const std::string &name, const std::vector<int> &indices,
                       const Value &value);
  Value getArrayElement(const std::string &name,
                        const std::vector<int> &indices);
  void initArray(const std::string &name, const std::vector<int> &dimensions,
                 const Value &defaultVal);
  int linearIndex(const std::vector<int> &dimensions,
                  const std::vector<int> &indices);

  // Helper functions
  bool toBool(const Value &v);
  int toInt(const Value &v);
  double toDouble(const Value &v);
  std::string toString(const Value &v);
};

#endif // INTERPRETER_HPP
