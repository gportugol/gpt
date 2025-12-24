/***************************************************************************
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
 ***************************************************************************/

#include "SemanticEval.hpp"
#include "GPTDisplay.hpp"
#include <sstream>

// ExpressionValue implementation

ExpressionValue::ExpressionValue()
    : _isPrimitive(true), _primitiveType(TIPO_NULO) {}

ExpressionValue::ExpressionValue(int type)
    : _isPrimitive(true), _primitiveType(type) {}

void ExpressionValue::setPrimitiveType(int type) { _primitiveType = type; }
int ExpressionValue::primitiveType() const { return _primitiveType; }

void ExpressionValue::setPrimitive(bool p) { _isPrimitive = p; }
bool ExpressionValue::isPrimitive() const { return _isPrimitive; }

void ExpressionValue::setDimensions(const list<int> &dims) {
  _dimensions = dims;
}
list<int> &ExpressionValue::dimensions() { return _dimensions; }

bool ExpressionValue::isNumeric(bool integerOnly) const {
  if (integerOnly) {
    return _primitiveType == TIPO_INTEIRO;
  }
  return _primitiveType == TIPO_INTEIRO || _primitiveType == TIPO_REAL;
}

bool ExpressionValue::isCompatibleWidth(ExpressionValue &other) const {
  if (!matchesType(other._isPrimitive))
    return false;
  if (!matchesPrimitiveType(other._primitiveType))
    return false;
  if (!_isPrimitive && !matchesDimensions(other._dimensions))
    return false;
  return true;
}

bool ExpressionValue::isCompatibleWidth(SymbolType &other) const {
  if (!matchesType(other.isPrimitive()))
    return false;
  if (!matchesPrimitiveType(other.primitiveType()))
    return false;
  if (!_isPrimitive && !matchesDimensions(other.dimensions()))
    return false;
  return true;
}

string ExpressionValue::toString() const {
  switch (_primitiveType) {
  case TIPO_INTEIRO:
    return "inteiro";
  case TIPO_REAL:
    return "real";
  case TIPO_CARACTERE:
    return "caractere";
  case TIPO_LITERAL:
    return "literal";
  case TIPO_LOGICO:
    return "logico";
  default:
    return "nulo";
  }
}

void ExpressionValue::set(SymbolType &s) {
  _isPrimitive = s.isPrimitive();
  _primitiveType = s.primitiveType();
  _dimensions = s.dimensions();
}

void ExpressionValue::setID(const string &id) { _id = id; }
string ExpressionValue::id() { return _id; }

bool ExpressionValue::matchesType(bool other_isprimitive) const {
  return _isPrimitive == other_isprimitive;
}

bool ExpressionValue::matchesDimensions(list<int> &other_dimensions) const {
  if (_dimensions.size() != other_dimensions.size())
    return false;
  auto it1 = _dimensions.begin();
  auto it2 = other_dimensions.begin();
  while (it1 != _dimensions.end()) {
    if (*it1 != *it2)
      return false;
    ++it1;
    ++it2;
  }
  return true;
}

bool ExpressionValue::matchesPrimitiveType(int other_type) const {
  if (_primitiveType == other_type)
    return true;
  // Allow numeric compatibility
  if ((_primitiveType == TIPO_INTEIRO || _primitiveType == TIPO_REAL) &&
      (other_type == TIPO_INTEIRO || other_type == TIPO_REAL)) {
    return true;
  }
  return false;
}

// SemanticEval implementation

SemanticEval::SemanticEval(SymbolTable &st) : stable(st) {}

void SemanticEval::registrarLeia() {
  Symbol s;
  s.setLexeme("leia");
  s.setType(SymbolType(TIPO_LITERAL));
  s.setScope("@global");
  s.setIsFunction(true);
  stable.insertSymbol(s, "@global");
}

void SemanticEval::registrarImprima() {
  Symbol s;
  s.setLexeme("imprima");
  s.setType(SymbolType(TIPO_NULO));
  s.setScope("@global");
  s.setIsFunction(true);
  stable.insertSymbol(s, "@global");
}

void SemanticEval::declararVariavel(const string &nome, int tipo, int linha) {
  list<int> empty;
  declararVariavel(nome, tipo, empty, linha);
}

void SemanticEval::declararVariavel(const string &nome, int tipo,
                                    const list<int> &dimensoes, int linha) {
  string scope = _currentScope.empty() ? "@global" : _currentScope;

  // Check if variable already exists in current scope
  try {
    Symbol &existing = stable.getSymbol(scope, nome, false);
    // If we got here, symbol already exists
    stringstream ss;
    ss << "Variável '" << nome << "' já declarada neste escopo";
    throw SymbolTableException(ss.str());
  } catch (SymbolTableException &) {
    // Symbol doesn't exist, good - create it
    Symbol s;
    s.setLexeme(nome);
    SymbolType st(tipo);
    if (!dimensoes.empty()) {
      st.setPrimitive(false);
      st.setDimensions(dimensoes);
    }
    s.setType(st);
    s.setScope(scope);
    s.setLine(linha);
    stable.insertSymbol(s, scope);
  }
}

void SemanticEval::declararFuncao(const string &nome, int tipoRetorno,
                                  const list<pair<string, int>> &params,
                                  int linha) {
  // Check if function already exists
  try {
    Symbol &existing = stable.getSymbol("@global", nome, false);
    if (existing.isFunction_()) {
      stringstream ss;
      ss << "Função '" << nome << "' já declarada";
      throw SymbolTableException(ss.str());
    }
  } catch (SymbolTableException &) {
    // Function doesn't exist, good
  }

  Symbol s;
  s.setLexeme(nome);
  s.setType(SymbolType(tipoRetorno));
  s.setScope("@global");
  s.setLine(linha);
  s.setIsFunction(true);

  // Store parameter info
  list<SymbolType> paramTypes;
  for (const auto &p : params) {
    paramTypes.push_back(SymbolType(p.second));
  }
  s.setParams(paramTypes);

  stable.insertSymbol(s, "@global");
}

void SemanticEval::verificarVariavel(const string &nome, int linha) {
  string scope = _currentScope.empty() ? "@global" : _currentScope;

  // Try current scope first, then global
  try {
    stable.getSymbol(scope, nome, true);
  } catch (SymbolTableException &e) {
    stringstream ss;
    ss << "Variável '" << nome << "' não declarada";
    throw SymbolTableException(ss.str());
  }
}

void SemanticEval::verificarFuncao(const string &nome, int linha) {
  try {
    Symbol &s = stable.getSymbol("@global", nome, false);
    if (!s.isFunction_()) {
      stringstream ss;
      ss << "'" << nome << "' não é uma função";
      throw SymbolTableException(ss.str());
    }
  } catch (SymbolTableException &) {
    stringstream ss;
    ss << "Função '" << nome << "' não declarada";
    throw SymbolTableException(ss.str());
  }
}

void SemanticEval::entrarEscopo(const string &nome) {
  _scopeStack.push_back(_currentScope);
  _currentScope = nome;
}

void SemanticEval::sairEscopo() {
  if (!_scopeStack.empty()) {
    _currentScope = _scopeStack.back();
    _scopeStack.pop_back();
  } else {
    _currentScope.clear();
  }
}
