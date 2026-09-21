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

#ifndef SEMANTICEVAL_HPP
#define SEMANTICEVAL_HPP

#include "Symbol.hpp"
#include "SymbolTable.hpp"

#include <list>
#include <map>
#include <stdlib.h>
#include <string>

using namespace std;

//---------- helpers ------------

// Replaces the RefPortugolAST the ANTLR2 evaluator received.
class TokenRef {
public:
  TokenRef() : line(0), type(0) {}
  TokenRef(const string &text_, int line_, int type_ = 0)
      : text(text_), line(line_), type(type_) {}

  const string &getText() const { return text; }
  int getLine() const { return line; }
  int getType() const { return type; }

  string text;
  int line;
  int type; // tipo do token (PortugolParser::T_*), usado para operadores
};

class ExpressionValue {
public:
  ExpressionValue();
  ExpressionValue(int type);

  void setPrimitiveType(int type);
  int primitiveType() const;

  void setPrimitive(bool);
  bool isPrimitive() const;

  void setDimensions(const list<int> &);
  list<int> &dimensions();

  bool isNumeric(bool integerOnly = false) const;
  bool isCompatibleWidth(ExpressionValue &other) const;
  bool isCompatibleWidth(SymbolType &other) const;

  string toString() const;

  void set(SymbolType &);

  void setID(const string &);
  string id();

protected:
  bool matchesType(bool other_isprimitive) const;
  bool matchesDimensions(list<int> &other_dimensions) const;
  bool matchesPrimitiveType(int other_type) const;

  bool _isPrimitive;
  int _primitiveType;
  list<int> _dimensions; // conjunto/matriz
  string _id;
};

class Funcao {
public:
  void setId(const TokenRef &t) { id = t; }

  void addParam(const TokenRef &name, int type) {
    SymbolType t(type);
    params.push_back(pair<TokenRef, SymbolType>(name, t));
  }

  void addParam(const TokenRef &name, int type, const list<int> &dims) {
    SymbolType t(type);
    t.setPrimitive(false);
    t.setDimensions(dims);
    params.push_back(pair<TokenRef, SymbolType>(name, t));
  }

  void setReturnType(int type) { return_type.setPrimitiveType(type); }

  TokenRef id;
  SymbolType return_type;

  list<pair<TokenRef, SymbolType>> params; // pair<lexeme, type>
};

//---------------------------------------------------------------------//

class SemanticEval {
public:
  enum UnaryOp { UN_POS, UN_NEG, UN_NOT, UN_BNOT };

  SemanticEval(SymbolTable &st);

  SymbolTable &getSymbolTable();

  void setCurrentScope(const string &);
  const string &currentScopeName() const { return currentScope; }

  void declareVar(int type, const TokenRef &prim);
  void declareVar(int type, list<int> dims, const TokenRef &mt);

  void declareVars(int type, const list<TokenRef> &prims);
  void declareVars(int type, const list<int> &dims, const list<TokenRef> &ms);

  void evaluateAttribution(ExpressionValue &lv, ExpressionValue &rv, int line);

  ExpressionValue evaluateLValue(const TokenRef &id,
                                 list<ExpressionValue> &dim);

  void evaluateBooleanExpr(ExpressionValue &ev, int line);
  void evaluateParaExpr(ExpressionValue &ev, int line, const string &term);
  ExpressionValue evaluateExpr(ExpressionValue &left, ExpressionValue &right,
                               const TokenRef &op);
  ExpressionValue evaluateExpr(ExpressionValue &ev, UnaryOp op,
                               const TokenRef &unary_op);

  void evaluateReturnCmd(ExpressionValue &ev, int line);

  void declareFunction(Funcao &f);
  ExpressionValue evaluateFCall(const TokenRef &f, list<ExpressionValue> &args);

  void evaluatePasso(int line, const string &str);

protected:
  bool evalVariableRedeclaration(const string &scope, const TokenRef &id);
  ExpressionValue evaluateNumTypes(ExpressionValue &left,
                                   ExpressionValue &right);

  SymbolTable &stable;
  string currentScope;
};

#endif
