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

#ifndef SEMANTICEVAL_HPP
#define SEMANTICEVAL_HPP

#include "Symbol.hpp"
#include "SymbolTable.hpp"

#include <list>
#include <map>
#include <string>

using namespace std;

//---------- helpers ------------

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
  list<int> _dimensions;
  string _id;
};

//---------- SemanticEval ------------

class SemanticEval {
public:
  SemanticEval(SymbolTable &st);

  void registrarLeia();
  void registrarImprima();

  void declararVariavel(const string &nome, int tipo, int linha);
  void declararVariavel(const string &nome, int tipo,
                        const list<int> &dimensoes, int linha);
  void declararFuncao(const string &nome, int tipoRetorno,
                      const list<pair<string, int>> &params, int linha);
  void verificarVariavel(const string &nome, int linha);
  void verificarFuncao(const string &nome, int linha);

  void entrarEscopo(const string &nome);
  void sairEscopo();

private:
  SymbolTable &stable;
  string _currentScope;
  list<string> _scopeStack;
};

#endif
