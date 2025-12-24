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
#include <cmath>
#include <iostream>
#include <sstream>

Interpreter::Interpreter(SymbolTable &st, const std::string &host, int port)
    : interpreter(st, host, port), _stable(st), _returning(false),
      _exitCode(0) {}

int Interpreter::run(PortugolParser::AlgoritmoContext *tree) {
  if (tree) {
    visit(tree);
  }
  return _exitCode;
}

void Interpreter::enterScope() {
  _scopeStack.push_back({});
  _arrayStack.push_back({});
}

void Interpreter::exitScope() {
  if (!_scopeStack.empty()) {
    _scopeStack.pop_back();
  }
  if (!_arrayStack.empty()) {
    _arrayStack.pop_back();
  }
}

void Interpreter::setVariable(const std::string &name, const Value &value) {
  // Check in current scope first
  if (!_scopeStack.empty()) {
    // Check if variable exists in any scope
    for (auto it = _scopeStack.rbegin(); it != _scopeStack.rend(); ++it) {
      if (it->find(name) != it->end()) {
        (*it)[name] = value;
        return;
      }
    }
    // Check global scope
    if (_variables.find(name) != _variables.end()) {
      _variables[name] = value;
      return;
    }
    // Create in current scope
    _scopeStack.back()[name] = value;
  } else {
    _variables[name] = value;
  }
}

Value Interpreter::getVariable(const std::string &name) {
  // Check scopes from innermost to outermost
  for (auto it = _scopeStack.rbegin(); it != _scopeStack.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return found->second;
    }
  }
  // Check global scope
  auto found = _variables.find(name);
  if (found != _variables.end()) {
    return found->second;
  }
  // Default value
  return 0;
}

int Interpreter::linearIndex(const std::vector<int> &dimensions,
                             const std::vector<int> &indices) {
  // Calculate linear index from multi-dimensional indices
  // For dimensions [2][3], indices [1][2] -> linear = 1 * 3 + 2 = 5
  if (indices.size() != dimensions.size())
    return -1;

  int linear = 0;
  int multiplier = 1;
  for (int i = static_cast<int>(dimensions.size()) - 1; i >= 0; --i) {
    if (indices[i] < 0 || indices[i] >= dimensions[i])
      return -1; // bounds check
    linear += indices[i] * multiplier;
    multiplier *= dimensions[i];
  }
  return linear;
}

void Interpreter::initArray(const std::string &name,
                            const std::vector<int> &dimensions,
                            const Value &defaultVal) {
  // Calculate total size
  int totalSize = 1;
  for (int dim : dimensions) {
    totalSize *= dim;
  }

  ArrayInfo info;
  info.dimensions = dimensions;
  info.data.assign(totalSize, defaultVal);

  // Check scope stack first (for local arrays), then global
  if (!_arrayStack.empty()) {
    _arrayStack.back()[name] = info;
  } else {
    _arrays[name] = info;
  }
}

void Interpreter::setArrayElement(const std::string &name,
                                  const std::vector<int> &indices,
                                  const Value &value) {
  // Search in scope stack from top to bottom
  for (auto it = _arrayStack.rbegin(); it != _arrayStack.rend(); ++it) {
    auto ait = it->find(name);
    if (ait != it->end()) {
      int idx = linearIndex(ait->second.dimensions, indices);
      if (idx >= 0 && idx < static_cast<int>(ait->second.data.size())) {
        ait->second.data[idx] = value;
        return;
      }
    }
  }
  // Then check global arrays
  auto ait = _arrays.find(name);
  if (ait != _arrays.end()) {
    int idx = linearIndex(ait->second.dimensions, indices);
    if (idx >= 0 && idx < static_cast<int>(ait->second.data.size())) {
      ait->second.data[idx] = value;
    }
  }
}

Value Interpreter::getArrayElement(const std::string &name,
                                   const std::vector<int> &indices) {
  // Search in scope stack from top to bottom
  for (auto it = _arrayStack.rbegin(); it != _arrayStack.rend(); ++it) {
    auto ait = it->find(name);
    if (ait != it->end()) {
      int idx = linearIndex(ait->second.dimensions, indices);
      if (idx >= 0 && idx < static_cast<int>(ait->second.data.size())) {
        return ait->second.data[idx];
      }
    }
  }
  // Then check global arrays
  auto ait = _arrays.find(name);
  if (ait != _arrays.end()) {
    int idx = linearIndex(ait->second.dimensions, indices);
    if (idx >= 0 && idx < static_cast<int>(ait->second.data.size())) {
      return ait->second.data[idx];
    }
  }
  return 0;
}

bool Interpreter::toBool(const Value &v) {
  return std::visit(
      [](auto &&arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>)
          return arg;
        else if constexpr (std::is_same_v<T, int>)
          return arg != 0;
        else if constexpr (std::is_same_v<T, double>)
          return arg != 0.0;
        else if constexpr (std::is_same_v<T, std::string>)
          return !arg.empty();
        else
          return false;
      },
      v);
}

int Interpreter::toInt(const Value &v) {
  return std::visit(
      [](auto &&arg) -> int {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>)
          return arg ? 1 : 0;
        else if constexpr (std::is_same_v<T, int>)
          return arg;
        else if constexpr (std::is_same_v<T, double>)
          return static_cast<int>(arg);
        else if constexpr (std::is_same_v<T, std::string>) {
          // If single character, return ASCII value
          if (arg.size() == 1)
            return static_cast<int>(static_cast<unsigned char>(arg[0]));
          try {
            return std::stoi(arg);
          } catch (...) {
            return 0;
          }
        } else
          return 0;
      },
      v);
}

double Interpreter::toDouble(const Value &v) {
  return std::visit(
      [](auto &&arg) -> double {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>)
          return arg ? 1.0 : 0.0;
        else if constexpr (std::is_same_v<T, int>)
          return static_cast<double>(arg);
        else if constexpr (std::is_same_v<T, double>)
          return arg;
        else if constexpr (std::is_same_v<T, std::string>) {
          // If single character, return ASCII value
          if (arg.size() == 1)
            return static_cast<double>(static_cast<unsigned char>(arg[0]));
          try {
            return std::stod(arg);
          } catch (...) {
            return 0.0;
          }
        } else
          return 0.0;
      },
      v);
}

std::string Interpreter::toString(const Value &v) {
  return std::visit(
      [](auto &&arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, bool>)
          return arg ? "verdadeiro" : "falso";
        else if constexpr (std::is_same_v<T, int>)
          return std::to_string(arg);
        else if constexpr (std::is_same_v<T, double>)
          return std::to_string(arg);
        else if constexpr (std::is_same_v<T, std::string>)
          return arg;
        else
          return "";
      },
      v);
}

antlrcpp::Any
Interpreter::visitAlgoritmo(PortugolParser::AlgoritmoContext *ctx) {
  // Initialize variables
  if (ctx->var_decl_block()) {
    visit(ctx->var_decl_block());
  }

  // Register functions
  for (auto func : ctx->func_decls()) {
    std::string funcName = func->T_IDENTIFICADOR()->getText();
    _functions[funcName] = func;
  }

  // Execute main block
  if (ctx->stm_block()) {
    visit(ctx->stm_block());
  }

  return nullptr;
}

antlrcpp::Any
Interpreter::visitVar_decl_block(PortugolParser::Var_decl_blockContext *ctx) {
  for (auto varDecl : ctx->var_decl()) {
    visit(varDecl);
  }
  return nullptr;
}

antlrcpp::Any Interpreter::visitVar_decl(PortugolParser::Var_declContext *ctx) {
  // Initialize variables with default values based on type
  Value defaultVal = 0; // Default for int
  bool isArray = false;
  std::vector<int> dimensions;

  auto tipoDecl = ctx->tipo_decl();
  if (auto tipoPrim =
          dynamic_cast<PortugolParser::TipoPrimitivoContext *>(tipoDecl)) {
    auto tp = tipoPrim->tp_prim();
    if (tp->T_KW_INTEIRO())
      defaultVal = 0;
    else if (tp->T_KW_REAL())
      defaultVal = 0.0;
    else if (tp->T_KW_CARACTERE())
      defaultVal = std::string("");
    else if (tp->T_KW_LITERAL())
      defaultVal = std::string("");
    else if (tp->T_KW_LOGICO())
      defaultVal = false;
  } else if (auto tipoMatriz =
                 dynamic_cast<PortugolParser::TipoMatrizContext *>(tipoDecl)) {
    isArray = true;
    // Get ALL dimensions for multi-dimensional arrays
    auto dims = tipoMatriz->tp_matriz()->dimensoes();
    if (dims) {
      for (auto dimToken : dims->T_INT_LIT()) {
        dimensions.push_back(std::stoi(dimToken->getText()));
      }
    }
    // Get base type
    auto tpPrimPl = tipoMatriz->tp_matriz()->tp_prim_pl();
    if (tpPrimPl->T_KW_INTEIROS())
      defaultVal = 0;
    else if (tpPrimPl->T_KW_REAIS())
      defaultVal = 0.0;
    else if (tpPrimPl->T_KW_CARACTERES())
      defaultVal = std::string("");
    else if (tpPrimPl->T_KW_LITERAIS())
      defaultVal = std::string("");
    else if (tpPrimPl->T_KW_LOGICOS())
      defaultVal = false;
  }

  for (auto id : ctx->T_IDENTIFICADOR()) {
    std::string varName = id->getText();
    if (isArray) {
      initArray(varName, dimensions, defaultVal);
    } else {
      _variables[varName] = defaultVal;
    }
  }

  return nullptr;
}

antlrcpp::Any
Interpreter::visitStm_block(PortugolParser::Stm_blockContext *ctx) {
  if (ctx->stm_list()) {
    for (auto stm : ctx->stm_list()->stm()) {
      if (_returning)
        break;
      visit(stm);
    }
  }
  return nullptr;
}

antlrcpp::Any
Interpreter::visitStmAtribuicao(PortugolParser::StmAtribuicaoContext *ctx) {
  auto stmAttr = ctx->stm_attr();
  if (stmAttr) {
    std::string varName = stmAttr->lvalue()->T_IDENTIFICADOR()->getText();
    Value val = evaluateExpr(stmAttr->expr());

    // Check for array subscript
    auto arraySub = stmAttr->lvalue()->array_sub();
    if (arraySub && !arraySub->expr().empty()) {
      // Array assignment - collect ALL indices for multi-dimensional arrays
      std::vector<int> indices;
      for (auto indexExpr : arraySub->expr()) {
        indices.push_back(toInt(evaluateExpr(indexExpr)));
      }

      // Convert type based on existing array element type
      Value existing = getArrayElement(varName, indices);
      if (std::holds_alternative<int>(existing) &&
          !std::holds_alternative<int>(val)) {
        val = toInt(val);
      }

      setArrayElement(varName, indices, val);
    } else {
      // Simple variable assignment - convert type based on existing variable
      // type
      Value existing = getVariable(varName);
      if (std::holds_alternative<int>(existing) &&
          !std::holds_alternative<int>(val)) {
        val = toInt(val);
      }

      setVariable(varName, val);
    }
  }
  return nullptr;
}

antlrcpp::Any
Interpreter::visitStmChamadaFunc(PortugolParser::StmChamadaFuncContext *ctx) {
  evaluateFcall(ctx->fcall());
  return nullptr;
}

antlrcpp::Any
Interpreter::visitStmRetorno(PortugolParser::StmRetornoContext *ctx) {
  auto stmRet = ctx->stm_ret();
  if (auto retComExpr =
          dynamic_cast<PortugolParser::RetorneComExprContext *>(stmRet)) {
    _returnValue = evaluateExpr(retComExpr->expr());
    // If in main block (not in a function), set exit code
    if (_scopeStack.empty()) {
      _exitCode = toInt(_returnValue);
    }
  }
  _returning = true;
  return nullptr;
}

antlrcpp::Any
Interpreter::visitStmCondicional(PortugolParser::StmCondicionalContext *ctx) {
  auto stmSe = ctx->stm_se();
  if (stmSe) {
    Value cond = evaluateExpr(stmSe->expr());
    if (toBool(cond)) {
      // Execute then block
      if (stmSe->stm_list()) {
        for (auto stm : stmSe->stm_list()->stm()) {
          if (_returning)
            break;
          visit(stm);
        }
      }
    } else if (stmSe->senao_part()) {
      // Execute else block
      for (auto stm : stmSe->senao_part()->stm_list()->stm()) {
        if (_returning)
          break;
        visit(stm);
      }
    }
  }
  return nullptr;
}

antlrcpp::Any
Interpreter::visitStmEnquanto(PortugolParser::StmEnquantoContext *ctx) {
  auto stmEnq = ctx->stm_enquanto();
  if (stmEnq) {
    while (!_returning && toBool(evaluateExpr(stmEnq->expr()))) {
      if (stmEnq->stm_list()) {
        for (auto stm : stmEnq->stm_list()->stm()) {
          if (_returning)
            break;
          visit(stm);
        }
      }
    }
  }
  return nullptr;
}

antlrcpp::Any
Interpreter::visitStmRepita(PortugolParser::StmRepitaContext *ctx) {
  auto stmRep = ctx->stm_repita();
  if (stmRep) {
    do {
      if (stmRep->stm_list()) {
        for (auto stm : stmRep->stm_list()->stm()) {
          if (_returning)
            break;
          visit(stm);
        }
      }
    } while (!_returning && !toBool(evaluateExpr(stmRep->expr())));
  }
  return nullptr;
}

antlrcpp::Any Interpreter::visitStmPara(PortugolParser::StmParaContext *ctx) {
  auto stmPara = ctx->stm_para();
  if (stmPara) {
    std::string varName = stmPara->lvalue()->T_IDENTIFICADOR()->getText();
    int inicio = toInt(evaluateExpr(stmPara->inicio));
    int fim = toInt(evaluateExpr(stmPara->fim));

    int step = 1;
    if (stmPara->passo()) {
      step = std::stoi(stmPara->passo()->T_INT_LIT()->getText());
      if (stmPara->passo()->T_MENOS()) {
        step = -step;
      }
    }

    // Loop from inicio to fim (inclusive)
    for (int current = inicio; !_returning && ((step > 0 && current <= fim) ||
                                               (step < 0 && current >= fim));
         current += step) {

      setVariable(varName, current);

      if (stmPara->stm_list()) {
        for (auto stm : stmPara->stm_list()->stm()) {
          if (_returning)
            break;
          visit(stm);
        }
      }
    }
  }
  return nullptr;
}

antlrcpp::Any
Interpreter::visitFunc_decls(PortugolParser::Func_declsContext *ctx) {
  // Functions are registered during algoritmo visit, not executed here
  return nullptr;
}

Value Interpreter::evaluateExpr(PortugolParser::ExprContext *ctx) {
  if (!ctx)
    return 0;

  auto exprEList = ctx->expr_e();
  if (exprEList.empty())
    return 0;

  Value result = evaluateExprE(exprEList[0]);

  // Handle OU operations
  for (size_t i = 1; i < exprEList.size(); i++) {
    Value right = evaluateExprE(exprEList[i]);
    result = toBool(result) || toBool(right);
  }

  return result;
}

Value Interpreter::evaluateExprE(PortugolParser::Expr_eContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_bit_ou();
  if (list.empty())
    return 0;

  Value result = evaluateExprBitOu(list[0]);

  // Handle E operations
  for (size_t i = 1; i < list.size(); i++) {
    Value right = evaluateExprBitOu(list[i]);
    result = toBool(result) && toBool(right);
  }

  return result;
}

Value Interpreter::evaluateExprBitOu(PortugolParser::Expr_bit_ouContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_bit_xou();
  if (list.empty())
    return 0;

  Value result = evaluateExprBitXou(list[0]);

  for (size_t i = 1; i < list.size(); i++) {
    Value right = evaluateExprBitXou(list[i]);
    result = toInt(result) | toInt(right);
  }

  return result;
}

Value Interpreter::evaluateExprBitXou(
    PortugolParser::Expr_bit_xouContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_bit_e();
  if (list.empty())
    return 0;

  Value result = evaluateExprBitE(list[0]);

  for (size_t i = 1; i < list.size(); i++) {
    Value right = evaluateExprBitE(list[i]);
    result = toInt(result) ^ toInt(right);
  }

  return result;
}

Value Interpreter::evaluateExprBitE(PortugolParser::Expr_bit_eContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_igual();
  if (list.empty())
    return 0;

  Value result = evaluateExprIgual(list[0]);

  for (size_t i = 1; i < list.size(); i++) {
    Value right = evaluateExprIgual(list[i]);
    result = toInt(result) & toInt(right);
  }

  return result;
}

Value Interpreter::evaluateExprIgual(PortugolParser::Expr_igualContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_relacional();
  if (list.empty())
    return 0;

  Value result = evaluateExprRelacional(list[0]);

  // Get operators
  auto igualOps = ctx->T_IGUAL();
  auto difOps = ctx->T_DIFERENTE();

  for (size_t i = 1; i < list.size(); i++) {
    Value right = evaluateExprRelacional(list[i]);

    // ANTLR2-style comparison using stringstream for float precision
    bool equal;
    if (std::holds_alternative<double>(result) ||
        std::holds_alternative<double>(right)) {
      // At least one is real - compare via stringstream
      std::stringstream ssL, ssR;
      ssL << toDouble(result);
      ssR << toDouble(right);
      equal = (ssL.str() == ssR.str());
    } else {
      // Compare as integers
      equal = (toInt(result) == toInt(right));
    }

    if (i - 1 < igualOps.size()) {
      result = equal;
    } else {
      result = !equal;
    }
  }

  return result;
}

Value Interpreter::evaluateExprRelacional(
    PortugolParser::Expr_relacionalContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_ad();
  if (list.empty())
    return 0;

  Value result = evaluateExprAd(list[0]);

  auto maiorOps = ctx->T_MAIOR();
  auto menorOps = ctx->T_MENOR();
  auto maiorEqOps = ctx->T_MAIOR_EQ();
  auto menorEqOps = ctx->T_MENOR_EQ();

  for (size_t i = 1; i < list.size(); i++) {
    Value right = evaluateExprAd(list[i]);
    double l = toDouble(result);
    double r = toDouble(right);

    // Determine which operator was used (simplified)
    if (!maiorOps.empty())
      result = l > r;
    else if (!menorOps.empty())
      result = l < r;
    else if (!maiorEqOps.empty())
      result = l >= r;
    else if (!menorEqOps.empty())
      result = l <= r;
    else
      result = l > r;
  }

  return result;
}

Value Interpreter::evaluateExprAd(PortugolParser::Expr_adContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_multip();
  if (list.empty())
    return 0;

  Value result = evaluateExprMultip(list[0]);

  // Get all children and iterate to find operators in order
  auto children = ctx->children;
  size_t operandIdx = 1;

  for (auto child : children) {
    if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
      int tokenType = terminal->getSymbol()->getType();
      if (operandIdx < list.size()) {
        Value right = evaluateExprMultip(list[operandIdx]);
        bool bothInt = std::holds_alternative<int>(result) &&
                       std::holds_alternative<int>(right);

        if (tokenType == PortugolParser::T_MAIS) {
          result = bothInt ? Value(toInt(result) + toInt(right))
                           : Value(toDouble(result) + toDouble(right));
          operandIdx++;
        } else if (tokenType == PortugolParser::T_MENOS) {
          result = bothInt ? Value(toInt(result) - toInt(right))
                           : Value(toDouble(result) - toDouble(right));
          operandIdx++;
        }
      }
    }
  }

  return result;
}

Value Interpreter::evaluateExprMultip(PortugolParser::Expr_multipContext *ctx) {
  if (!ctx)
    return 0;

  auto list = ctx->expr_unario();
  if (list.empty())
    return 0;

  Value result = evaluateExprUnario(list[0]);

  // Get all children and iterate to find operators in order
  auto children = ctx->children;
  size_t operandIdx = 1;

  for (auto child : children) {
    if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
      int tokenType = terminal->getSymbol()->getType();
      if (operandIdx < list.size()) {
        Value right = evaluateExprUnario(list[operandIdx]);
        bool bothInt = std::holds_alternative<int>(result) &&
                       std::holds_alternative<int>(right);

        if (tokenType == PortugolParser::T_MULTIP) {
          result = bothInt ? Value(toInt(result) * toInt(right))
                           : Value(toDouble(result) * toDouble(right));
          operandIdx++;
        } else if (tokenType == PortugolParser::T_DIV) {
          // Integer division if both are integers
          if (bothInt) {
            result = toInt(result) / toInt(right);
          } else {
            result = toDouble(result) / toDouble(right);
          }
          operandIdx++;
        } else if (tokenType == PortugolParser::T_MOD) {
          result = toInt(result) % toInt(right);
          operandIdx++;
        }
      }
    }
  }

  return result;
}

Value Interpreter::evaluateExprUnario(PortugolParser::Expr_unarioContext *ctx) {
  if (!ctx)
    return 0;

  if (auto neg = dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    Value v = evaluateExprElemento(neg->expr_elemento());
    return -toDouble(v);
  } else if (auto pos =
                 dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    return evaluateExprElemento(pos->expr_elemento());
  } else if (auto notExpr =
                 dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    Value v = evaluateExprElemento(notExpr->expr_elemento());
    return !toBool(v);
  } else if (auto bitNot =
                 dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    Value v = evaluateExprElemento(bitNot->expr_elemento());
    return ~toInt(v);
  } else if (auto elem =
                 dynamic_cast<PortugolParser::ExprElementoContext *>(ctx)) {
    return evaluateExprElemento(elem->expr_elemento());
  }

  return 0;
}

Value Interpreter::evaluateExprElemento(
    PortugolParser::Expr_elementoContext *ctx) {
  if (!ctx)
    return 0;

  if (auto fc = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    return evaluateFcall(fc->fcall());
  } else if (auto lv = dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    return evaluateLvalue(lv->lvalue());
  } else if (auto lit =
                 dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    return evaluateLiteral(lit->literal());
  } else if (auto paren =
                 dynamic_cast<PortugolParser::ElemParentesesContext *>(ctx)) {
    return evaluateExpr(paren->expr());
  }

  return 0;
}

Value Interpreter::evaluateFcall(PortugolParser::FcallContext *ctx) {
  std::string funcName = ctx->T_IDENTIFICADOR()->getText();

  // Built-in functions
  if (funcName == "imprima") {
    if (ctx->fargs()) {
      for (auto expr : ctx->fargs()->expr()) {
        Value v = evaluateExpr(expr);
        std::cout << toString(v);
      }
    }
    std::cout << std::endl;
    return 0;
  }

  if (funcName == "leia") {
    std::string input;
    std::getline(std::cin, input);
    return input;
  }

  // User-defined functions
  auto it = _functions.find(funcName);
  if (it != _functions.end()) {
    auto funcCtx = it->second;

    enterScope();

    // Bind parameters
    auto fparams = funcCtx->fparams();
    auto fargs = ctx->fargs();
    if (fparams && fargs) {
      auto params = fparams->fparam();
      auto args = fargs->expr();
      for (size_t i = 0; i < params.size() && i < args.size(); i++) {
        std::string paramName = params[i]->T_IDENTIFICADOR()->getText();
        auto tipoDecl = params[i]->tipo_decl();

        // Check if parameter is a matrix type
        if (auto tipoMatriz =
                dynamic_cast<PortugolParser::TipoMatrizContext *>(tipoDecl)) {
          // Navigate through expression tree to find the lvalue
          // expr -> expr_e -> ... -> expr_elemento -> lvalue
          std::string argName;
          auto expr = args[i];
          if (expr && !expr->expr_e().empty()) {
            auto exprE = expr->expr_e(0);
            if (exprE && !exprE->expr_bit_ou().empty()) {
              auto exprBitOu = exprE->expr_bit_ou(0);
              if (exprBitOu && !exprBitOu->expr_bit_xou().empty()) {
                auto exprBitXou = exprBitOu->expr_bit_xou(0);
                if (exprBitXou && !exprBitXou->expr_bit_e().empty()) {
                  auto exprBitE = exprBitXou->expr_bit_e(0);
                  if (exprBitE && !exprBitE->expr_igual().empty()) {
                    auto exprIgual = exprBitE->expr_igual(0);
                    if (exprIgual && !exprIgual->expr_relacional().empty()) {
                      auto exprRel = exprIgual->expr_relacional(0);
                      if (exprRel && !exprRel->expr_ad().empty()) {
                        auto exprAd = exprRel->expr_ad(0);
                        if (exprAd && !exprAd->expr_multip().empty()) {
                          auto exprMult = exprAd->expr_multip(0);
                          if (exprMult && !exprMult->expr_unario().empty()) {
                            auto exprUn = exprMult->expr_unario(0);
                            if (auto elemCtx = dynamic_cast<
                                    PortugolParser::ExprElementoContext *>(
                                    exprUn)) {
                              if (auto elemLv = dynamic_cast<
                                      PortugolParser::ElemLvalueContext *>(
                                      elemCtx->expr_elemento())) {
                                argName = elemLv->lvalue()
                                              ->T_IDENTIFICADOR()
                                              ->getText();
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }

          if (!argName.empty()) {
            // Copy array to function scope (local stack)
            // First look in array stack
            bool found = false;
            for (auto it = _arrayStack.rbegin(); it != _arrayStack.rend();
                 ++it) {
              auto ait = it->find(argName);
              if (ait != it->end()) {
                _arrayStack.back()[paramName] = ait->second;
                found = true;
                break;
              }
            }
            // Then look in global arrays
            if (!found && _arrays.count(argName)) {
              _arrayStack.back()[paramName] = _arrays[argName];
            }
          }
        } else {
          // Scalar parameter - convert type if necessary
          Value argVal = evaluateExpr(args[i]);

          // Get expected type and convert
          if (auto tipoPrim =
                  dynamic_cast<PortugolParser::TipoPrimitivoContext *>(
                      tipoDecl)) {
            auto tp = tipoPrim->tp_prim();
            if (tp->T_KW_INTEIRO()) {
              argVal = toInt(argVal);
            } else if (tp->T_KW_REAL()) {
              argVal = toDouble(argVal);
            } else if (tp->T_KW_LOGICO()) {
              argVal = toBool(argVal);
            }
            // LITERAL and CARACTERE remain as strings
          }

          // Create parameter DIRECTLY in current scope (not setVariable, which
          // looks up)
          _scopeStack.back()[paramName] = argVal;
        }
      }
    }

    // Declare and initialize local variables (fvar_decl)
    if (auto fvarDecl = dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(
            funcCtx->fvar_decl())) {
      for (auto varDecl : fvarDecl->var_decl()) {
        visit(varDecl);
      }
    }

    // Execute function body
    _returning = false;
    visit(funcCtx->stm_block());

    Value result = _returnValue;
    _returning = false;
    _returnValue = 0;

    exitScope();

    return result;
  }

  return 0;
}

Value Interpreter::evaluateLvalue(PortugolParser::LvalueContext *ctx) {
  std::string varName = ctx->T_IDENTIFICADOR()->getText();

  // Check for array subscript
  auto arraySub = ctx->array_sub();
  if (arraySub && !arraySub->expr().empty()) {
    // Collect ALL indices for multi-dimensional arrays
    std::vector<int> indices;
    for (auto indexExpr : arraySub->expr()) {
      indices.push_back(toInt(evaluateExpr(indexExpr)));
    }
    return getArrayElement(varName, indices);
  }

  return getVariable(varName);
}

Value Interpreter::evaluateLiteral(PortugolParser::LiteralContext *ctx) {
  if (auto litInt = dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    std::string text = litInt->T_INT_LIT()->getText();
    // Parse binary (0b), hex (0x), octal (0c) literals
    if (text.size() > 2) {
      if (text[0] == '0' && (text[1] == 'b' || text[1] == 'B')) {
        return static_cast<int>(std::stol(text.substr(2), nullptr, 2));
      } else if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        return static_cast<int>(std::stol(text.substr(2), nullptr, 16));
      } else if (text[0] == '0' && (text[1] == 'c' || text[1] == 'C')) {
        return static_cast<int>(std::stol(text.substr(2), nullptr, 8));
      }
    }
    return std::stoi(text);
  } else if (auto litReal =
                 dynamic_cast<PortugolParser::LitRealContext *>(ctx)) {
    return std::stod(litReal->T_REAL_LIT()->getText());
  } else if (auto litStr =
                 dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    std::string s = litStr->T_STRING_LIT()->getText();
    // Remove quotes
    if (s.size() >= 2)
      s = s.substr(1, s.size() - 2);
    // Process escape sequences
    std::string result;
    for (size_t i = 0; i < s.size(); i++) {
      if (s[i] == '\\' && i + 1 < s.size()) {
        switch (s[i + 1]) {
        case 'n':
          result += '\n';
          i++;
          break;
        case 't':
          result += '\t';
          i++;
          break;
        case 'r':
          result += '\r';
          i++;
          break;
        case '\\':
          result += '\\';
          i++;
          break;
        case '"':
          result += '"';
          i++;
          break;
        default:
          result += s[i];
          break;
        }
      } else {
        result += s[i];
      }
    }
    return result;
  } else if (auto litCar =
                 dynamic_cast<PortugolParser::LitCaractereContext *>(ctx)) {
    std::string s = litCar->T_CARAC_LIT()->getText();
    if (s.size() >= 2)
      s = s.substr(1, s.size() - 2);
    // Handle escape sequences
    if (s.size() == 2 && s[0] == '\\') {
      switch (s[1]) {
      case 'n':
        return std::string(1, '\n');
      case 't':
        return std::string(1, '\t');
      case 'r':
        return std::string(1, '\r');
      case '0':
        return std::string(1, '\0');
      case '\\':
        return std::string("\\");
      case '\'':
        return std::string("'");
      default:
        return s;
      }
    }
    return s;
  } else if (dynamic_cast<PortugolParser::LitVerdadeiroContext *>(ctx)) {
    return true;
  } else if (dynamic_cast<PortugolParser::LitFalsoContext *>(ctx)) {
    return false;
  }

  return 0;
}
