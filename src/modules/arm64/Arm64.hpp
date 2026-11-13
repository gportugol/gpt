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

#ifndef ARM64_HPP
#define ARM64_HPP

#include "NativeBackend.hpp"
#include "SymbolTable.hpp"
#include <list>
#include <map>
#include <sstream>
#include <string>

using namespace std;

// Locals and parameters have no names in the assembly: each one is an
// offset from x29, kept here and resolved when the code is written.
class Arm64SubProgram {
public:
  Arm64SubProgram();

  void declareLocal(const string &, int = 0, bool minit = true);
  void declareParam(const string &, int type, int = 0);
  bool offsetOf(const string &, int &);

  void writeTEXT(const string &);

  void init(const string &, int = 0);
  string name();

  string source();

private:
  void writeMatrixInitCode(int offset, int size);
  void writeMatrixCopyCode(int paramOffset, int offset, int type, int msize);

  int _param_offset;
  int _local_offset;
  string _name;
  map<string, int> _offsets;
  list<string> _declared;

  stringstream _init; // init commands
  stringstream _txt;  // body
};

class Arm64 : public NativeBackend {
public:
  static string EntryPoint;
  static string makeID(const string &);

  static string loadImm(const string &reg, long long value);
  static string frameAddress(const string &reg, int offset);
  static string frameStore(const string &reg, int offset);
  static string frameLoad(const string &reg, int offset);
  static string dropArgs(int count);

  Arm64(SymbolTable &);
  ~Arm64();

  void init(const string &);
  string source();

  void writeTEXT(const string &);

  void createScope(const string &);
  string currentScope();

  void declarePrimitive(int decl_type, const string &name, int type);
  void declareMatrix(int decl_type, int type, string name, list<string> dims);

  string addGlobalLiteral(string str);
  string createLabel(bool local, string tmpl);

  void writeExit();
  void writeMainExit();
  void writeMainReturn();
  void writeFuncReturn(int expecting_type, int etype, bool hasRetExpr);
  void writeFuncEnd();

  void writeComment(const string &);
  void writeLabel(const string &);
  void writeJump(const string &);
  void writeJumpIfFalse(const string &);

  void writeDiscardResult();
  void writePushResult();
  void writePushIndex();
  void writeLValueBegin(bool isprim);
  void writeLValueIndex(const string &name, int line, int size, int multiplier);
  void writeLValueEnd(bool isprim);

  void writeImprimaTag(int etype);
  void writeImprimaCall(int args);
  void writeCallArg(int etype, int ptype);
  void writeLeiaCall(const string &fname);
  void writeCall(const string &fname, int args);

  void writeParaDe();
  void writeParaDeAttr(int de_type, int expecting_type,
                       pair<pair<int, bool>, string> &);
  void writeParaAte(int ate_type, int lvtype);
  void writeParaEntry(const string &lvname, bool decreasing,
                      const string &lbpara, const string &lbfim);
  void writeParaNext(const string &lvname, bool hasPasso, bool decreasing,
                     const string &step, const string &lbpara,
                     const string &lbfim);

  void writeAttribution(int e1, int e2, pair<pair<int, bool>, string> &);
  void writeOuExpr();
  void writeEExpr();
  void writeBitOuExpr();
  void writeBitXouExpr();
  void writeBitEExpr();
  void writeIgualExpr(int e1, int e2);
  void writeDiferenteExpr(int e1, int e2);
  void writeMaiorExpr(int e1, int e2);
  void writeMenorExpr(int e1, int e2);
  void writeMaiorEqExpr(int e1, int e2);
  void writeMenorEqExpr(int e1, int e2);
  void writeMaisExpr(int e1, int e2);
  void writeMenosExpr(int e1, int e2);
  void writeDivExpr(int e1, int e2, int line);
  void writeMultipExpr(int e1, int e2);
  void writeModExpr(int line);
  void writeUnaryNeg(int etype);
  void writeUnaryNot();
  void writeRealToLogical();
  void writeUnaryBitNotExpr();
  void writeLiteralExpr(int type, const string &src);
  void writeIndexCheck(const string &name, int line, int size);
  void writeLValueExpr(pair<pair<int, bool>, string> &);

  void writeCast(int e1, int e2);
  void writeSignExtend(int type);

private:
  string toGasString(const string &str);
  void writeAddress(const string &reg, const string &name);
  void writeElementAddress(const string &name);
  void writeLogicalOf(int type);
  void writeLoadReals(int e1, int e2);
  void writeOrderExpr(int e1, int e2, const string &intCond,
                      const string &realCond);
  void writeRealArith(int e1, int e2, const string &op);
  void writeDivisorCheck(int line, bool realDivisor);
  void writeSignedDivide(int line, bool remainder);

  SymbolTable &_stable;

  string _currentScope;

  stringstream _head;
  stringstream _bss;
  stringstream _data;
  stringstream _lib;

  map<string, Arm64SubProgram> _subprograms;
  long long _globalBytes;
  map<string, string> _nameLiterals;
};

#endif
