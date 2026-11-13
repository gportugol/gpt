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

#include "Arm64.hpp"
#include "GPTDisplay.hpp"
#include "config.h"

#include <stdlib.h>

// The registers follow the x86-64 backend's roles: x0 holds the value at
// hand and the result of a call, x1 the right operand, x2 the element
// index and x3 an address. x9 is scratch for addresses and constants.

Arm64SubProgram::Arm64SubProgram()
    : _param_offset(16), // the saved x29 and x30 come first
      _local_offset(8) { // starts at -8
}

void Arm64SubProgram::writeTEXT(const string &str) { _txt << str << endl; }

// Each argument takes a 16-byte slot and the first one is pushed first,
// so it ends up farthest from x29.
void Arm64SubProgram::init(const string &name, int totalParams) {
  _name = name;
  _param_offset = 16 * totalParams;
}

string Arm64SubProgram::name() { return _name; }

bool Arm64SubProgram::offsetOf(const string &var, int &offset) {
  map<string, int>::iterator it = _offsets.find(var);
  if (it == _offsets.end()) {
    return false;
  }
  offset = it->second;
  return true;
}

// Locals take 8-byte slots below x29; a matrix keeps its first element at
// the lowest address.
void Arm64SubProgram::declareLocal(const string &local_var, int msize,
                                   bool minit) {
  long long bytes = (long long)(msize == 0 ? 1 : msize) * 8;
  NativeBackend::checkVariableBytes(_local_offset - 8 + bytes, _name);
  _declared.push_back(local_var);
  if (msize == 0) {
    _offsets[local_var] = -_local_offset;
    if (minit) {
      _init << Arm64::frameStore("xzr", -_local_offset) << endl;
    }
    _local_offset += 8;
  } else {
    int offset = -(_local_offset + (msize * 8) - 8);
    _offsets[local_var] = offset;
    if (minit) {
      writeMatrixInitCode(offset, msize);
    }
    _local_offset += msize * 8;
  }
}

// A matrix parameter arrives as the address of the caller's matrix and is
// copied into a local of the same size.
void Arm64SubProgram::declareParam(const string &param, int type, int msize) {
  if (msize == 0) {
    _declared.push_back(param);
    _offsets[param] = _param_offset;
  } else {
    int paramOffset = _param_offset;
    declareLocal(param, msize, false);
    writeMatrixCopyCode(paramOffset, _offsets[param], type, msize);
  }
  _param_offset -= 16;
}

void Arm64SubProgram::writeMatrixCopyCode(int paramOffset, int offset, int type,
                                          int msize) {
  _init << Arm64::frameLoad("x0", paramOffset) << endl;
  _init << "push x0" << endl;
  _init << Arm64::frameAddress("x0", offset) << endl;
  _init << "push x0" << endl;
  _init << "mov x0, #" << (type == TIPO_LITERAL) << endl;
  _init << "push x0" << endl;
  _init << Arm64::loadImm("x0", (long long)msize * 8) << endl;
  _init << "push x0" << endl;
  _init << "bl matrix_cpy" << endl;
  _init << "clargs 4" << endl;
}

void Arm64SubProgram::writeMatrixInitCode(int offset, int size) {
  _init << Arm64::frameAddress("x0", offset) << endl;
  _init << "push x0" << endl;
  _init << Arm64::loadImm("x0", (long long)size * 8) << endl;
  _init << "push x0" << endl;
  _init << "bl matrix_init" << endl;
  _init << "clargs 2" << endl;
}

string Arm64SubProgram::source() {
  stringstream s;

  if (_name == SymbolTable::GlobalScope) {
    s << Arm64::EntryPoint << ":" << endl;
  } else {
    s << Arm64::makeID(_name) << ":" << endl;
    for (list<string>::iterator it = _declared.begin(); it != _declared.end();
         ++it) {
      int offset = _offsets[*it];
      s << "// " << *it << ": x29" << (offset < 0 ? "" : "+") << offset << endl;
    }
    // sp stays aligned to 16 bytes. A frame of a page or more grows one
    // page at a time, touching each one, so that it cannot skip the guard
    // page below the stack and land in other memory.
    int frame = (_local_offset - 8 + 15) & ~15;
    if (frame <= 4095) {
      s << "begin " << frame << endl;
    } else {
      s << "begin 0" << endl;
      s << Arm64::loadImm("x9", frame / 4096) << endl;
      s << "1:" << endl;
      s << "sub sp, sp, #4096" << endl;
      s << "str xzr, [sp]" << endl;
      s << "subs x9, x9, #1" << endl;
      s << "b.ne 1b" << endl;
      if (frame % 4096) {
        s << "sub sp, sp, #" << frame % 4096 << endl;
      }
    }
  }

  s << _init.str();
  s << _txt.str();

  return s.str();
}

////////--------------------------------------------------------

string Arm64::EntryPoint = "start";

// Every identifier x of the program becomes _x, so the string labels carry
// a $, which an identifier cannot contain.
string Arm64::makeID(const string &str) { return string("_") + str; }

// mov takes what a single movz or movn builds; any other value is built 16
// bits at a time. A w register gets the low 32 bits of value.
string Arm64::loadImm(const string &reg, long long value) {
  bool wide = reg[0] == 'x';
  unsigned long long bits =
      wide ? (unsigned long long)value : (unsigned int)value;
  long long sval = wide ? value : (int)(unsigned int)value;

  stringstream s;
  if (sval >= -65536 && sval <= 65535) {
    s << "mov " << reg << ", #" << sval;
    return s.str();
  }

  bool first = true;
  for (int shift = 0; shift < (wide ? 64 : 32); shift += 16) {
    unsigned int part = (bits >> shift) & 0xffff;
    if (part == 0) {
      continue;
    }
    if (!first) {
      s << "\n";
    }
    s << (first ? "movz " : "movk ") << reg << ", #0x" << hex << part << dec;
    if (shift) {
      s << ", lsl #" << shift;
    }
    first = false;
  }
  return s.str();
}

// add and sub take a 12-bit immediate; larger offsets go through x9.
string Arm64::frameAddress(const string &reg, int offset) {
  stringstream s;
  int magnitude = offset < 0 ? -offset : offset;
  const char *op = offset < 0 ? "sub " : "add ";
  if (magnitude <= 4095) {
    s << op << reg << ", x29, #" << magnitude;
  } else {
    s << loadImm("x9", magnitude) << "\n" << op << reg << ", x29, x9";
  }
  return s.str();
}

// A 64-bit load or store reaches x29-256 to x29+255 unscaled and x29+32760
// scaled by 8; anything else goes through an address in x9.
static bool inSlotReach(int offset) {
  return (offset >= -256 && offset <= 255) ||
         (offset >= 0 && offset <= 32760 && offset % 8 == 0);
}

string Arm64::frameStore(const string &reg, int offset) {
  stringstream s;
  if (inSlotReach(offset)) {
    s << "str " << reg << ", [x29, #" << offset << "]";
  } else {
    s << frameAddress("x9", offset) << "\nstr " << reg << ", [x9]";
  }
  return s.str();
}

string Arm64::frameLoad(const string &reg, int offset) {
  stringstream s;
  if (inSlotReach(offset)) {
    s << "ldr " << reg << ", [x29, #" << offset << "]";
  } else {
    s << frameAddress("x9", offset) << "\nldr " << reg << ", [x9]";
  }
  return s.str();
}

string Arm64::dropArgs(int count) {
  stringstream s;
  if (count * 16 <= 4095) {
    s << "clargs " << count;
  } else {
    s << loadImm("x9", (long long)count * 16) << "\nadd sp, sp, x9";
  }
  return s.str();
}

Arm64::Arm64(SymbolTable &st) : _stable(st), _globalBytes(0) {}

Arm64::~Arm64() {}

void Arm64::init(const string &name) {

  _head << "// algoritmo: " << name << "\n\n";

#include "arm64_linux.h"

  _data << "    .data\n"
           "    .balign 8\n"
           "mem_ptr: .quad 0\n"
           "str_true: .asciz \"verdadeiro\"\n"
           "str_false: .asciz \"falso\"\n"
           "str_null: .asciz \"(nulo)\"\n"
           "str_empty: .byte 0\n"
           "str_zero: .asciz \"0\"\n"
           "str_no_mem_left: .asciz \"Não foi possível alocar memória.\"\n";

  // Global variables start zeroed, so they live in .bss with the arena.
  _bss << "    .bss\n"
          "    .balign 16\n"
          "mem: .zero MEMORY_SIZE\n";

  createScope(SymbolTable::GlobalScope);
}

void Arm64::writeTEXT(const string &str) {
  _subprograms[_currentScope].writeTEXT(str);
}

void Arm64::createScope(const string &scope) {
  _currentScope = scope;
  if (scope == SymbolTable::GlobalScope) {
    _subprograms[scope].init(scope);
  } else {
    Symbol symb = _stable.getSymbol(SymbolTable::GlobalScope, scope, true);
    _subprograms[scope].init(scope, symb.param.symbolList().size());
  }
}

string Arm64::currentScope() { return _currentScope; }

void Arm64::writeExit() { writeTEXT("exit w2"); }

void Arm64::writeMainExit() {
  writeTEXT("mov w2, #0");
  writeExit();
}

void Arm64::writeMainReturn() {
  writeTEXT("pop x2");
  writeExit();
}

void Arm64::writeFuncReturn(int expecting_type, int etype, bool hasRetExpr) {
  if (expecting_type != TIPO_NULO) {
    writeTEXT("pop x0");
  }
  if (expecting_type == TIPO_LITERAL) {
    writeTEXT("push x0");
    writeTEXT("bl clone_literal");
    writeTEXT("clargs 1");
  } else if (hasRetExpr) {
    writeCast(etype, expecting_type);
  }
  writeTEXT("return");
}

// A function that ends without retorne returns all bits zero, which is 0,
// 0.0, falso or the null literal, as in the interpreter.
void Arm64::writeFuncEnd() {
  writeTEXT("mov x0, #0");
  writeTEXT("return");
}

void Arm64::writeComment(const string &text) { writeTEXT("// " + text); }

void Arm64::writeLabel(const string &label) { writeTEXT(label + ":"); }

void Arm64::writeJump(const string &label) { writeTEXT("b " + label); }

void Arm64::writeJumpIfFalse(const string &label) {
  writeTEXT("pop x0");
  writeTEXT("cbz w0, " + label);
}

void Arm64::writeDiscardResult() { writeTEXT("pop x0"); }

void Arm64::writePushResult() { writeTEXT("push x0"); }

void Arm64::writePushIndex() { writeTEXT("push x2"); }

void Arm64::writeLValueBegin(bool isprim) {
  if (!isprim) {
    writeTEXT("push xzr");
  }
}

void Arm64::writeLValueIndex(const string &name, int line, int size,
                             int multiplier) {
  writeTEXT("pop x0");
  writeIndexCheck(name, line, size);
  writeTEXT(loadImm("w1", multiplier));
  writeTEXT("mul w0, w0, w1");
  writeTEXT("pop x1");
  writeTEXT("add w0, w0, w1");
  writeTEXT("push x0");
}

void Arm64::writeLValueEnd(bool isprim) {
  writeTEXT(isprim ? "mov x2, #0" : "pop x2");
}

void Arm64::writeImprimaTag(int etype) {
  switch (etype) {
  case TIPO_INTEIRO:
    writeTEXT("mov w0, #105 // 'i'");
    break;
  case TIPO_REAL:
    writeTEXT("mov w0, #114 // 'r'");
    break;
  case TIPO_CARACTERE:
    writeTEXT("mov w0, #99 // 'c'");
    break;
  case TIPO_LITERAL:
    writeTEXT("mov w0, #115 // 's'");
    break;
  case TIPO_LOGICO:
    writeTEXT("mov w0, #108 // 'l'");
    break;
  default:
    GPTDisplay::self()->showError(
        "Erro interno: tipo nao suportado (Arm64::writeImprimaTag).");
    exit(1);
  }
  writeTEXT("push x0");
}

void Arm64::writeImprimaCall(int args) {
  writeTEXT(loadImm("w0", args));
  writeTEXT("push x0");
  writeTEXT("bl imprima");
  writeTEXT(dropArgs((args * 2) + 1));
  writeTEXT("print_lf");
}

void Arm64::writeCallArg(int etype, int ptype) {
  writeTEXT("pop x0");
  writeCast(etype, ptype);
  writeTEXT("push x0");
}

void Arm64::writeLeiaCall(const string &fname) { writeTEXT("bl " + fname); }

void Arm64::writeCall(const string &fname, int args) {
  writeTEXT("bl " + Arm64::makeID(fname));
  if (args) {
    writeTEXT(dropArgs(args));
  }
}

void Arm64::writeParaDe() {
  writeTEXT("push x2");
  writeTEXT("// para: de:");
}

void Arm64::writeParaDeAttr(int de_type, int expecting_type,
                            pair<pair<int, bool>, string> &lv) {
  writeTEXT("// para: de attr:");
  writeAttribution(de_type, expecting_type, lv);
  writeTEXT("push x2");
  writeTEXT("// para: ate:");
}

void Arm64::writeParaAte(int ate_type, int lvtype) {
  writeTEXT("pop x0");
  writeCast(ate_type, lvtype);
  writeTEXT("push x0");
}

// The loop variable's slot index sits at [sp, #16] and the final value at
// [sp] while the body runs.
void Arm64::writeParaEntry(const string &lvname, bool decreasing,
                           const string &lbpara, const string &lbfim) {
  writeTEXT("ldr x2, [sp, #16]");
  writeElementAddress(lvname);
  writeTEXT("ldr w0, [x3]");
  writeTEXT("ldr w1, [sp]");
  writeTEXT("cmp w0, w1");
  writeTEXT((decreasing ? "b.lt " : "b.gt ") + lbfim);
  writeTEXT(lbpara + ":");
}

void Arm64::writeParaNext(const string &lvname, bool hasPasso, bool decreasing,
                          const string &step, const string &lbpara,
                          const string &lbfim) {
  writeTEXT("ldr x2, [sp, #16]");
  writeElementAddress(lvname);
  writeTEXT("ldr w0, [x3]");
  if (!hasPasso) {
    writeTEXT("add w0, w0, #1");
  } else {
    writeTEXT(loadImm("w1", (int)(unsigned)strtoll(step.c_str(), NULL, 10)));
    writeTEXT(decreasing ? "sub w0, w0, w1" : "add w0, w0, w1");
  }
  writeTEXT("ldr w1, [sp]");
  writeTEXT("cmp w0, w1");
  writeTEXT(((hasPasso && decreasing) ? "b.lt " : "b.gt ") + lbfim);
  writeTEXT("sxtw x0, w0");
  writeTEXT("str x0, [x3]");
  writeTEXT("b " + lbpara);
  writeTEXT(lbfim + ":");
  writeTEXT("ldrsw x1, [sp]");
  writeTEXT("ldr x2, [sp, #16]");
  writeElementAddress(lvname);
  writeTEXT("str x1, [x3]");
  writeTEXT("pop x0");
  writeTEXT("pop x2");
  writeTEXT("// fimpara");
}

void Arm64::declarePrimitive(int decl_type, const string &name, int type) {
  if (decl_type == VAR_GLOBAL) {
    // every value takes one 8-byte slot
    switch (type) {
    case TIPO_INTEIRO:
    case TIPO_REAL:
    case TIPO_CARACTERE:
    case TIPO_LITERAL:
    case TIPO_LOGICO:
      _globalBytes += 8;
      checkVariableBytes(_globalBytes, SymbolTable::GlobalScope);
      _bss << Arm64::makeID(name) << ": .zero 8" << endl;
      break;
    default:
      GPTDisplay::self()->showError(
          "Erro interno: tipo nao suportado (Arm64::declarePrimitive).");
      exit(1);
    }
  } else if (decl_type == VAR_PARAM) {
    _subprograms[currentScope()].declareParam(name, type);
  } else if (decl_type == VAR_LOCAL) {
    _subprograms[currentScope()].declareLocal(name);
  } else {
    GPTDisplay::self()->showError("Erro interno: Arm64::declarePrimitive).");
    exit(1);
  }
}

void Arm64::declareMatrix(int decl_type, int type, string name,
                          list<string> dims) {
  int size = (int)matrixElements(dims);

  if (decl_type == VAR_GLOBAL) {
    switch (type) {
    case TIPO_INTEIRO:
    case TIPO_REAL:
    case TIPO_CARACTERE:
    case TIPO_LOGICO:
    case TIPO_LITERAL:
      _globalBytes += (long long)size * 8;
      checkVariableBytes(_globalBytes, SymbolTable::GlobalScope);
      _bss << Arm64::makeID(name) << ": .zero " << (long long)size * 8 << endl;
      break;
    default:
      GPTDisplay::self()->showError(
          "Erro interno: tipo nao suportado (Arm64::declareMatrix).");
      exit(1);
    }
  } else if (decl_type == VAR_PARAM) {
    _subprograms[currentScope()].declareParam(name, type, size);
  } else if (decl_type == VAR_LOCAL) {
    _subprograms[currentScope()].declareLocal(name, size);
  } else {
    GPTDisplay::self()->showError("Erro interno: Arm64::declareMatrix).");
    exit(1);
  }
}

// The escapes of a G-Portugol literal, then the bytes the assembler string
// cannot hold as they are (quote, backslash and control characters) as
// octal escapes. Bytes from 128 up, the UTF-8 text, stay as they are.
string Arm64::toGasString(const string &str) {
  string bytes;
  for (unsigned int i = 0; i < str.length(); i++) {
    if (str[i] != '\\') {
      bytes += str[i];
      continue;
    }
    i++;
    switch (str[i]) {
    case '0':
      bytes += '\0';
      break;
    case 'n':
      bytes += '\n';
      break;
    case 'r':
      bytes += '\r';
      break;
    case 't':
      bytes += '\t';
      break;
    default:
      bytes += str[i];
    }
  }

  stringstream s;
  for (unsigned int i = 0; i < bytes.length(); i++) {
    unsigned char c = bytes[i];
    if (c == '"' || c == '\\') {
      s << '\\' << c;
    } else if (c < 32 || c == 127) {
      s << '\\' << (char)('0' + (c >> 6)) << (char)('0' + ((c >> 3) & 7))
        << (char)('0' + (c & 7));
    } else {
      s << c;
    }
  }
  return s.str();
}

string Arm64::addGlobalLiteral(string str) {
  string lb = createLabel(false, "str");
  _data << lb << ": .asciz \"" << toGasString(str) << "\"" << endl;
  return lb;
}

string Arm64::createLabel(bool local, string tmpl) {
  static int c = 0;
  stringstream s;
  if (local) {
    s << ".L__" << tmpl << "_" << c;
  } else {
    s << "__" << tmpl << "$" << c;
  }
  c++;
  return s.str();
}

string Arm64::source() {
  stringstream str;

  str << _head.str();
  str << "    .text\n"
         "    .global "
      << EntryPoint << "\n";
  for (map<string, Arm64SubProgram>::iterator it = _subprograms.begin();
       it != _subprograms.end(); ++it) {
    str << it->second.source();
  }
  str << _lib.str();
  str << "\n" << _data.str() << "\n" << _bss.str();
  // Without this note the linker marks the stack executable.
  str << "\n    .section .note.GNU-stack,\"\",%progbits\n";

  return str.str();
}

void Arm64::writeAddress(const string &reg, const string &name) {
  int offset;
  if (_subprograms[_currentScope].offsetOf(name, offset)) {
    writeTEXT(frameAddress(reg, offset));
  } else {
    writeTEXT("la " + reg + ", " + Arm64::makeID(name));
  }
}

void Arm64::writeElementAddress(const string &name) {
  writeAddress("x3", name);
  writeTEXT("add x3, x3, x2, lsl #3");
}

// Reals travel as the 64 bits of a double in the integer registers and
// stack slots; the FP registers are only used while operating on them.
void Arm64::writeCast(int e1, int e2) {
  if ((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) {
    writeTEXT("scvtf d0, w0");
    writeTEXT("fmov x0, d0");
  } else if ((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) {
    // truncates, like the C cast
    writeTEXT("fmov d0, x0");
    writeTEXT("fcvtzs w0, d0");
  }
}

// Integer values are computed in 32 bits, so the slot gets the sign
// extension before being stored; literals and reals already fill 64 bits.
void Arm64::writeSignExtend(int type) {
  if ((type != TIPO_REAL) && (type != TIPO_LITERAL)) {
    writeTEXT("sxtw x0, w0");
  }
}

// Left operand from x0 into d0 and right from x1 into d1, converting
// whichever side is an integer.
void Arm64::writeLoadReals(int e1, int e2) {
  writeTEXT(e1 == TIPO_REAL ? "fmov d0, x0" : "scvtf d0, w0");
  writeTEXT(e2 == TIPO_REAL ? "fmov d1, x1" : "scvtf d1, w1");
}

void Arm64::writeAttribution(int e1, int e2,
                             pair<pair<int, bool>, string> &lv) {
  writeTEXT("pop x0");
  writeTEXT("pop x2");
  writeCast(e1, e2);
  writeSignExtend(e2);
  writeElementAddress(lv.second);
  writeTEXT("str x0, [x3]");
}

void Arm64::writeOuExpr() {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  writeTEXT("cmp w0, #0");
  writeTEXT("cset w0, ne");
  writeTEXT("cmp w1, #0");
  writeTEXT("cset w1, ne");
  writeTEXT("orr w0, w0, w1");
  writeTEXT("push x0");
}

void Arm64::writeEExpr() {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  writeTEXT("cmp w0, #0");
  writeTEXT("cset w0, ne");
  writeTEXT("cmp w1, #0");
  writeTEXT("cset w1, ne");
  writeTEXT("and w0, w0, w1");
  writeTEXT("push x0");
}

void Arm64::writeBitOuExpr() {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  writeTEXT("orr w0, w0, w1");
  writeTEXT("push x0");
}

void Arm64::writeBitXouExpr() {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  writeTEXT("eor w0, w0, w1");
  writeTEXT("push x0");
}

void Arm64::writeBitEExpr() {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  writeTEXT("and w0, w0, w1");
  writeTEXT("push x0");
}

// A literal compared with anything goes through strcmp, which returns 1
// when the strings are equal. Reals compare as doubles, so 3.0 = 3 holds
// and NaN is different from everything, including itself.
void Arm64::writeIgualExpr(int e1, int e2) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  if ((e1 == TIPO_LITERAL) || (e2 == TIPO_LITERAL)) {
    writeTEXT("push x0");
    writeTEXT("push x1");
    writeTEXT("bl strcmp");
    writeTEXT("clargs 2");
  } else if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeLoadReals(e1, e2);
    writeTEXT("fcmp d0, d1");
    writeTEXT("cset w0, eq");
  } else {
    writeTEXT("cmp w0, w1");
    writeTEXT("cset w0, eq");
  }
  writeTEXT("push x0");
}

void Arm64::writeDiferenteExpr(int e1, int e2) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  if ((e1 == TIPO_LITERAL) || (e2 == TIPO_LITERAL)) {
    writeTEXT("push x0");
    writeTEXT("push x1");
    writeTEXT("bl strcmp");
    writeTEXT("clargs 2");
    writeTEXT("cmp w0, #1");
    writeTEXT("cset w0, ne");
  } else if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeLoadReals(e1, e2);
    writeTEXT("fcmp d0, d1");
    writeTEXT("cset w0, ne");
  } else {
    writeTEXT("cmp w0, w1");
    writeTEXT("cset w0, ne");
  }
  writeTEXT("push x0");
}

// Ordering of two literals compares their lengths, as the interpreter
// does. After fcmp, lt and le also hold for an unordered result while mi
// and ls do not, so the real conditions give falso for a NaN, as in C.
void Arm64::writeOrderExpr(int e1, int e2, const string &intCond,
                           const string &realCond) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");

  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeLoadReals(e1, e2);
    writeTEXT("fcmp d0, d1");
    writeTEXT("cset w0, " + realCond);
  } else if ((e1 == TIPO_LITERAL) && (e2 == TIPO_LITERAL)) {
    writeTEXT("push x1");

    writeTEXT("push x0");
    writeTEXT("bl strlen");
    writeTEXT("clargs 1");

    writeTEXT("pop x1");
    writeTEXT("push x0");

    writeTEXT("push x1");
    writeTEXT("bl strlen");
    writeTEXT("clargs 1");

    writeTEXT("mov w1, w0");
    writeTEXT("pop x0");

    writeTEXT("cmp w0, w1");
    writeTEXT("cset w0, " + intCond);
  } else {
    writeTEXT("cmp w0, w1");
    writeTEXT("cset w0, " + intCond);
  }
  writeTEXT("push x0");
}

void Arm64::writeMaiorExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "gt", "gt");
}

void Arm64::writeMenorExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "lt", "mi");
}

void Arm64::writeMaiorEqExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "ge", "ge");
}

void Arm64::writeMenorEqExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "le", "ls");
}

void Arm64::writeRealArith(int e1, int e2, const string &op) {
  writeLoadReals(e1, e2);
  writeTEXT(op + " d0, d0, d1");
  writeTEXT("fmov x0, d0");
}

void Arm64::writeMaisExpr(int e1, int e2) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeRealArith(e1, e2, "fadd");
  } else {
    writeTEXT("add w0, w0, w1");
  }
  writeTEXT("push x0");
}

void Arm64::writeMenosExpr(int e1, int e2) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeRealArith(e1, e2, "fsub");
  } else {
    writeTEXT("sub w0, w0, w1");
  }
  writeTEXT("push x0");
}

// With the divisor in x1, a zero ends the program with the interpreter's
// message. A real divisor is tested without its sign bit so that -0.0 also
// counts as zero.
void Arm64::writeDivisorCheck(int line, bool realDivisor) {
  string ok = createLabel(true, "div");
  if (realDivisor) {
    writeTEXT("lsl x9, x1, #1");
    writeTEXT("cbnz x9, " + ok);
  } else {
    writeTEXT("cbnz w1, " + ok);
  }
  writeTEXT(loadImm("w0", line));
  writeTEXT("push x0");
  writeTEXT("bl div_zero_error");
  writeTEXT(ok + ":");
}

// sdiv does not trap: INT_MIN / -1 wraps to INT_MIN, and msub then leaves
// a remainder of 0, as the x86-64 backend's special case does.
void Arm64::writeSignedDivide(int line, bool remainder) {
  writeDivisorCheck(line, false);
  writeTEXT("sdiv w9, w0, w1");
  writeTEXT(remainder ? "msub w0, w9, w1, w0" : "mov w0, w9");
}

void Arm64::writeDivExpr(int e1, int e2, int line) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeDivisorCheck(line, e2 == TIPO_REAL);
    writeRealArith(e1, e2, "fdiv");
  } else {
    writeSignedDivide(line, false);
  }
  writeTEXT("push x0");
}

void Arm64::writeMultipExpr(int e1, int e2) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeRealArith(e1, e2, "fmul");
  } else {
    writeTEXT("mul w0, w0, w1");
  }
  writeTEXT("push x0");
}

void Arm64::writeModExpr(int line) {
  writeTEXT("pop x1");
  writeTEXT("pop x0");
  writeSignedDivide(line, true);
  writeTEXT("push x0");
}

void Arm64::writeUnaryNeg(int etype) {
  writeTEXT("pop x0");
  if (etype == TIPO_REAL) {
    writeTEXT("eor x0, x0, #0x8000000000000000");
  } else {
    writeTEXT("neg w0, w0");
  }
  writeTEXT("push x0");
}

void Arm64::writeUnaryNot() {
  writeTEXT("pop x0");
  writeTEXT("cmp w0, #0");
  writeTEXT("cset w0, eq");
  writeTEXT("push x0");
}

void Arm64::writeUnaryBitNotExpr() {
  writeTEXT("pop x0");
  writeTEXT("mvn w0, w0");
  writeTEXT("push x0");
}

// An integer literal keeps its low 32 bits, as in the interpreter: the one
// above INT_MAX, only reachable as the operand of a unary minus, becomes
// INT_MIN. A real comes as the 64 bits of its double in hexadecimal.
void Arm64::writeLiteralExpr(int type, const string &src) {
  if (type == TIPO_REAL) {
    writeTEXT(loadImm("x0", (long long)strtoull(src.c_str(), NULL, 16)));
  } else if (type == TIPO_LITERAL) {
    writeTEXT(src == "0" ? "mov x0, #0" : "la x0, " + src);
  } else {
    writeTEXT(loadImm("w0", (int)(unsigned)strtoll(src.c_str(), NULL, 10)));
  }
  writeTEXT("push x0");
}

// The message names only the subscript that failed, so for a matrix with
// several dimensions it shows "m[7]" where the interpreter shows "m[1][7]".
void Arm64::writeIndexCheck(const string &name, int line, int size) {
  map<string, string>::iterator lit = _nameLiterals.find(name);
  if (lit == _nameLiterals.end()) {
    lit = _nameLiterals.insert(make_pair(name, addGlobalLiteral(name))).first;
  }

  string ok = createLabel(true, "idx");
  if (size <= 4095) {
    stringstream s;
    s << "cmp w0, #" << size;
    writeTEXT(s.str());
  } else {
    writeTEXT(loadImm("w9", size));
    writeTEXT("cmp w0, w9");
  }
  writeTEXT("b.lo " + ok);
  writeTEXT("sxtw x0, w0");
  writeTEXT("push x0");
  writeTEXT("la x0, " + lit->second);
  writeTEXT("push x0");
  writeTEXT(loadImm("w0", line));
  writeTEXT("push x0");
  writeTEXT("bl index_error");
  writeTEXT(ok + ":");
}

void Arm64::writeLValueExpr(pair<pair<int, bool>, string> &lv) {
  writeElementAddress(lv.second);
  if (lv.first.second) { // the whole matrix, as an argument: its address
    writeTEXT("push x3");
  } else { // a variable or an element: its value
    writeTEXT("ldr x0, [x3]");
    writeTEXT("push x0");
  }
}
