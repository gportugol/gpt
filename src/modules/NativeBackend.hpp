/***************************************************************************
 *   Copyright (C) 2026 by Marcelo Jorge Vieira                            *
 *   metal@alucinados.com                                                  *
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

#ifndef NATIVEBACKEND_HPP
#define NATIVEBACKEND_HPP

#include <list>
#include <string>
#include <utility>

using namespace std;

// What the native code walker asks of a code generator. The generated
// program is a stack machine: every expression leaves its value on the
// stack and the operators pop their operands from it. An lvalue leaves the
// element index in the backend's index register (0 for a variable that is
// not a matrix), which writePushIndex saves on the stack for an assignment.
// While the body of a para loop runs, the stack holds the index of the loop
// variable under its final value.
class NativeBackend {
public:
  enum { VAR_GLOBAL, VAR_PARAM, VAR_LOCAL };

  virtual ~NativeBackend() {}

  virtual void init(const string &) = 0;
  virtual string source() = 0;

  virtual void createScope(const string &) = 0;
  virtual string currentScope() = 0;

  virtual void declarePrimitive(int decl_type, const string &name,
                                int type) = 0;
  virtual void declareMatrix(int decl_type, int type, string name,
                             list<string> dims) = 0;

  virtual string addGlobalLiteral(string str) = 0;
  virtual string createLabel(bool local, string tmpl) = 0;

  virtual void writeMainExit() = 0;
  virtual void writeMainReturn() = 0;
  virtual void writeFuncReturn(int expecting_type, int etype,
                               bool hasRetExpr) = 0;
  virtual void writeFuncEnd() = 0;

  virtual void writeComment(const string &) = 0;
  virtual void writeLabel(const string &) = 0;
  virtual void writeJump(const string &) = 0;
  virtual void writeJumpIfFalse(const string &) = 0;

  virtual void writeDiscardResult() = 0;
  virtual void writePushResult() = 0;
  virtual void writePushIndex() = 0;
  virtual void writeLValueBegin(bool isprim) = 0;
  virtual void writeLValueIndex(const string &name, int line, int size,
                                int multiplier) = 0;
  virtual void writeLValueEnd(bool isprim) = 0;

  virtual void writeImprimaTag(int etype) = 0;
  virtual void writeImprimaCall(int args) = 0;
  virtual void writeCallArg(int etype, int ptype) = 0;
  virtual void writeLeiaCall(const string &fname) = 0;
  virtual void writeCall(const string &fname, int args) = 0;

  virtual void writeParaDe() = 0;
  virtual void writeParaDeAttr(int de_type, int expecting_type,
                               pair<pair<int, bool>, string> &) = 0;
  virtual void writeParaAte(int ate_type, int lvtype) = 0;
  virtual void writeParaEntry(const string &lvname, bool decreasing,
                              const string &lbpara, const string &lbfim) = 0;
  virtual void writeParaNext(const string &lvname, bool hasPasso,
                             bool decreasing, const string &step,
                             const string &lbpara, const string &lbfim) = 0;

  virtual void writeAttribution(int e1, int e2,
                                pair<pair<int, bool>, string> &) = 0;
  virtual void writeOuExpr() = 0;
  virtual void writeEExpr() = 0;
  virtual void writeBitOuExpr() = 0;
  virtual void writeBitXouExpr() = 0;
  virtual void writeBitEExpr() = 0;
  virtual void writeIgualExpr(int e1, int e2) = 0;
  virtual void writeDiferenteExpr(int e1, int e2) = 0;
  virtual void writeMaiorExpr(int e1, int e2) = 0;
  virtual void writeMenorExpr(int e1, int e2) = 0;
  virtual void writeMaiorEqExpr(int e1, int e2) = 0;
  virtual void writeMenorEqExpr(int e1, int e2) = 0;
  virtual void writeMaisExpr(int e1, int e2) = 0;
  virtual void writeMenosExpr(int e1, int e2) = 0;
  virtual void writeDivExpr(int e1, int e2, int line) = 0;
  virtual void writeMultipExpr(int e1, int e2) = 0;
  virtual void writeModExpr(int line) = 0;
  virtual void writeUnaryNeg(int etype) = 0;
  virtual void writeUnaryNot() = 0;
  virtual void writeUnaryBitNotExpr() = 0;
  virtual void writeLiteralExpr(int type, const string &src) = 0;
  virtual void writeLValueExpr(pair<pair<int, bool>, string> &) = 0;

  string translateFuncLeia(int type);
  string toChar(const string &);
  string toReal(const string &);
};

#endif
