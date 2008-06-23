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

#include "SymbolTable.hpp"
#include "PortugolAST.hpp"

#include <string>
#include <map>
#include <list>
#include <stdlib.h>

//---------- helpers ------------

class ExpressionValue {
public:
  ExpressionValue();
  ExpressionValue(int type);

  void setPrimitiveType(int type);
  int primitiveType() const;
  
  void setPrimitive(bool);
  bool isPrimitive() const;

  void setDimensions(const list<int>&);
  list<int>& dimensions();
  
  bool isNumeric(bool integerOnly = false) const;
  bool isCompatibleWidth(ExpressionValue& other) const;
  bool isCompatibleWidth(SymbolType& other) const;

  string toString() const;

  void set(SymbolType&);

  void setID(const string&);
  string id();
  
protected:
  bool matchesType(bool other_isprimitive) const;
  bool matchesDimensions(list<int>& other_dimensions) const;
  bool matchesPrimitiveType(int other_type) const;

  bool _isPrimitive;
  int _primitiveType;
  list<int> _dimensions; //conjunto/matriz
  string _id;
};


class Funcao {
public:
  void setId(RefPortugolAST t) {
    id = t;
  }
  void addParams(pair<int, list<RefPortugolAST> >& p) {		
		for(list<RefPortugolAST>::iterator it = p.second.begin(); it != p.second.end(); ++it) {
			SymbolType t(p.first);
			params.push_back(pair<RefPortugolAST, SymbolType>(*it, t));
		}
  }

  void addParams(pair< pair<int, list<int> >, list<RefPortugolAST> >& m) {
		for(list<RefPortugolAST>::iterator it = m.second.begin(); it != m.second.end(); ++it) {
			SymbolType t(m.first.first);
			t.setPrimitive(false);
			t.setDimensions(m.first.second);
			params.push_back(pair<RefPortugolAST, SymbolType>(*it, t));
		}
  }

//   void setReturnType(pair<int, list<int> > type) {
//     return_type.setPrimitive(false);
//     return_type.setPrimitiveType(type.first);
//     return_type.setDimensions(type.second);
//   }

  void setReturnType(int type) {
    return_type.setPrimitiveType(type);
  }


  RefPortugolAST id;
  SymbolType return_type;

	list<pair<RefPortugolAST, SymbolType> > params; //pair<lexeme, type>
//   list< pair<int, list<RefPortugolAST> > > prim_params;
//   list< pair< pair<int, list<int> >, list<RefPortugolAST> > > mt_params;

};

//---------------------------------------------------------------------//

class SemanticEval {
public:
  SemanticEval(SymbolTable& st);

  SymbolTable& getSymbolTable();

  void setCurrentScope(const string&);

	void declareVar(int type, RefPortugolAST prim);
	void declareVar(int type, list<int> dims, RefPortugolAST mt);

  void declareVars(pair<int, list<RefPortugolAST> >& prims);
  void declareVars(pair< pair<int, list<int> >, list<RefPortugolAST> >& ms);
 

  void evaluateAttribution(ExpressionValue&  lv, ExpressionValue& rv, int line);

  ExpressionValue  evaluateLValue(RefPortugolAST id, list<ExpressionValue>& dim);

  void evaluateBooleanExpr(ExpressionValue& ev, int line);
  void evaluateParaExpr(ExpressionValue& ev, int line, const string& term);  
  ExpressionValue evaluateExpr(ExpressionValue& left, ExpressionValue& right, RefPortugolAST op);
  ExpressionValue evaluateExpr(ExpressionValue& ev, RefPortugolAST unary_op);  

  void evaluateReturnCmd(ExpressionValue& ev, int line);

  void declareFunction(Funcao& f);
  ExpressionValue  evaluateFCall(RefPortugolAST f, list<ExpressionValue>& args);  
//   void evaluateAllFCalls();
  
  void evaluatePasso(int line, const string& str);
protected:

  bool evalVariableRedeclaration(const string& scope, RefPortugolAST id);

  ExpressionValue evaluateNumTypes(ExpressionValue& left, ExpressionValue& right);  

  SymbolTable& stable;
  string currentScope;
  list<pair<RefPortugolAST,list<ExpressionValue> > > fcallsList;
};

#endif
