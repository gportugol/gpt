/***************************************************************************
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
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

#include "X86.hpp"
#include "GPTDisplay.hpp"
#include "config.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

X86SubProgram::X86SubProgram()
    : SizeofSlot(8),
      _param_offset(16), // return address and saved rbp come first
      _local_offset(8) { // starts at -8
}

X86SubProgram::X86SubProgram(const X86SubProgram &other)
    : SizeofSlot(8), _param_offset(other._param_offset),
      _local_offset(other._local_offset), _name(other._name),
      _params(other._params), _locals(other._locals) {

  _head << other._head.str();
  _txt << other._txt.str();
  _init << other._init.str();
}

X86SubProgram::~X86SubProgram() {}

void X86SubProgram::writeTEXT(const string &str) { _txt << str << endl; }

void X86SubProgram::init(const string &name, int totalParams) {
  _name = name;
  _param_offset = 8 + (totalParams * SizeofSlot);
}

string X86SubProgram::name() { return _name; }

void X86SubProgram::declareLocal(const string &local_var, int msize,
                                 bool minit) {
  long long bytes = (long long)(msize == 0 ? 1 : msize) * SizeofSlot;
  NativeBackend::checkVariableBytes(_local_offset - SizeofSlot + bytes, _name);
  if (msize == 0) {
    _head << "%define " << X86::makeID(local_var) << " rbp-" << _local_offset
          << endl;
    _end << "%undef " << X86::makeID(local_var) << endl;
    if (minit) {
      _init << "mov qword [" << X86::makeID(local_var) << "], 0" << endl;
    }

    _local_offset += SizeofSlot;
  } else {
    _head << "%define " << X86::makeID(local_var) << " rbp-"
          << (_local_offset + (msize * SizeofSlot) - SizeofSlot) << endl;
    _end << "%undef " << X86::makeID(local_var) << endl;
    if (minit) {
      writeMatrixInitCode(local_var, msize);
    }
    _local_offset += (msize * SizeofSlot);
  }
}

void X86SubProgram::declareParam(const string &param, int type, int msize) {

  if (msize == 0) {
    _head << "%define " << X86::makeID(param) << " rbp+" << _param_offset
          << endl;
    _end << "%undef " << X86::makeID(param) << endl;
  } else {
    _head << "%define p$" << param << " rbp+" << _param_offset << endl;
    _end << "%undef p$" << param << endl;
    declareLocal(param, msize, false);
    writeMatrixCopyCode(param, type, msize);
  }
  _param_offset -= SizeofSlot;
}

void X86SubProgram::writeMatrixCopyCode(const string &param, int type,
                                        int msize) {
  _init << "lea rax, [" << X86::makeID(param) << "]" << endl;
  _init << "addarg qword [p$" << param << "]" << endl;
  _init << "addarg rax" << endl;
  _init << "addarg " << (type == TIPO_LITERAL) << endl;
  _init << "addarg " << (msize * SizeofSlot) << endl;
  _init << "call matrix_cpy" << endl;
  _init << "clargs 4" << endl;
}

void X86SubProgram::writeMatrixInitCode(const string &varname, int size) {
  _init << "lea rbx, [" << X86::makeID(varname) << "]" << endl;
  _init << "addarg rbx" << endl;
  _init << "addarg " << size << " * SIZEOF_SLOT" << endl;
  _init << "call matrix_init" << endl;
  _init << "clargs 2" << endl;
}

string X86SubProgram::source() {
  stringstream s;

  if (_name == SymbolTable::GlobalScope) {
    s << X86::EntryPoint << ":" << endl;
  } else {
    s << X86::makeFuncID(_name) << ":" << endl;
  }
  s << _head.str();

  if (_name != SymbolTable::GlobalScope) {
    s << "begin " << (_local_offset - SizeofSlot) << endl;
  }

  s << _init.str();

  s << _txt.str();
  s << _end.str();

  return s.str();
}

////////--------------------------------------------------------

string X86::EntryPoint = "start";

// A variable x of the program becomes v$x and a function f becomes f$f. An
// identifier cannot contain a $, so these names cannot meet each other, the
// runtime labels, the pointer slot p$x of a matrix parameter, the string
// labels or NASM's __LINE__-style macros, and the %define of a local hides
// a global of the same name but not a function.
string X86::makeID(const string &str) { return string("v$") + str; }

string X86::makeFuncID(const string &str) { return string("f$") + str; }

X86::X86(SymbolTable &st) : _stable(st), _globalBytes(0) {}

X86::~X86() {}

void X86::init(const string &name) {

  _head << "; algoritmo: " << name << "\n\n";

#ifdef WIN32
#include <asm_win32.h>
#else
#include <asm_elf.h>
#endif

  // Global variables start zeroed, so they live in .bss with the arena.
#ifdef WIN32
  // The PE template declares the sections and their alignment up front.
  _bss << "section .bss\n"
          "bss_start:\n"
          "    mem    resb  MEMORY_SIZE\n\n";
  _data << "section .data\n"
           "data_start:\n";
#else
  // align=8 puts .bss at the end of .data rounded up to 8 bytes, where the
  // ELF header's p_memsz expects it. The page alignment keeps the writable
  // data off the code's page. The PE template sets its own section
  // attributes, so this stays ELF only.
  _bss << "section .bss align=8\n"
          "    mem    resb  MEMORY_SIZE\n\n";
  _data << "section .data align=4096\n";
#endif
  _data << "              data_no equ $\n"
           "    mem_ptr         dq mem\n"
           "    mem_end         dq mem + MEMORY_SIZE\n"
           "    str_true        db 'verdadeiro',0\n"
           "    str_false       db 'falso',0\n"
           "    str_null        db '(nulo)',0\n"
           "    str_empty       db 0\n"
           "    str_zero        db '0',0\n"
           "    str_no_mem_left db 'Não foi possível alocar memória.',0\n\n";

  createScope(SymbolTable::GlobalScope);
}

void X86::writeBSS(const string &str) { _bss << str << endl; }

void X86::writeDATA(const string &str) { _data << str << endl; }

void X86::writeTEXT(const string &str) {
  _subprograms[_currentScope].writeTEXT(str);
}

void X86::createScope(const string &scope) {
  _currentScope = scope;
  if (scope == SymbolTable::GlobalScope) {
    _subprograms[scope].init(scope);
  } else {
    Symbol symb = _stable.getSymbol(SymbolTable::GlobalScope, scope, true);
    _subprograms[scope].init(scope, symb.param.symbolList().size());
  }
}

string X86::currentScope() { return _currentScope; }

void X86::writeExit() { writeTEXT("exit ecx"); }

void X86::writeMainExit() {
  writeTEXT("mov ecx, 0");
  writeExit();
}

void X86::writeMainReturn() {
  writeTEXT("pop rcx");
  writeExit();
}

void X86::writeFuncReturn(int expecting_type, int etype, bool hasRetExpr) {
  if (expecting_type != TIPO_NULO) {
    writeTEXT("pop rax");
  }
  if (expecting_type == TIPO_LITERAL) {
    writeTEXT("addarg rax");
    writeTEXT("call clone_literal");
    writeTEXT("clargs 1");
  } else if (hasRetExpr) {
    writeCast(etype, expecting_type);
  }
  writeTEXT("return");
}

// A function that ends without retorne returns all bits zero, which is 0,
// 0.0, falso or the null literal, as in the interpreter.
void X86::writeFuncEnd() {
  writeTEXT("xor eax, eax");
  writeTEXT("return");
}

void X86::writeComment(const string &text) { writeTEXT("; " + text); }

void X86::writeLabel(const string &label) { writeTEXT(label + ":"); }

void X86::writeJump(const string &label) { writeTEXT("jmp " + label); }

void X86::writeJumpIfFalse(const string &label) {
  writeTEXT("pop rax");
  writeTEXT("cmp eax, 0");
  writeTEXT("je near " + label);
}

void X86::writeDiscardResult() { writeTEXT("pop rax"); }

void X86::writePushResult() { writeTEXT("push rax"); }

void X86::writePushIndex() { writeTEXT("push rcx"); }

void X86::writeLValueBegin(bool isprim) {
  if (!isprim) {
    writeTEXT("push 0");
  }
}

void X86::writeLValueIndex(const string &name, int line, int size,
                           int multiplier) {
  stringstream s;
  writeTEXT("pop rax");
  writeIndexCheck(name, line, size);
  s << "mov ebx, " << multiplier;
  writeTEXT(s.str());
  writeTEXT("imul ebx");
  writeTEXT("pop rbx");
  writeTEXT("add eax, ebx");
  writeTEXT("push rax");
}

void X86::writeLValueEnd(bool isprim) {
  writeTEXT(isprim ? "mov ecx, 0" : "pop rcx");
}

void X86::writeImprimaTag(int etype) {
  switch (etype) {
  case TIPO_INTEIRO:
    writeTEXT("addarg 'i'");
    break;
  case TIPO_REAL:
    writeTEXT("addarg 'r'");
    break;
  case TIPO_CARACTERE:
    writeTEXT("addarg 'c'");
    break;
  case TIPO_LITERAL:
    writeTEXT("addarg 's'");
    break;
  case TIPO_LOGICO:
    writeTEXT("addarg 'l'");
    break;
  default:
    GPTDisplay::self()->showError(
        "Erro interno: tipo nao suportado (X86::writeImprimaTag).");
    exit(1);
  }
}

void X86::writeImprimaCall(int args) {
  stringstream s;
  s << "addarg " << args;
  writeTEXT(s.str());
  writeTEXT("call imprima");
  s.str("");
  s << "clargs " << ((args * 2) + 1);
  writeTEXT(s.str());
  writeTEXT("print_lf");
}

void X86::writeCallArg(int etype, int ptype) {
  writeTEXT("pop rax");
  writeCast(etype, ptype);
  writeTEXT("addarg rax");
}

void X86::writeLeiaCall(const string &fname) { writeTEXT("call " + fname); }

void X86::writeCall(const string &fname, int args) {
  writeTEXT("call " + X86::makeFuncID(fname));
  if (args) {
    stringstream s;
    s << "clargs " << args;
    writeTEXT(s.str());
  }
}

void X86::writeParaDe() {
  writeTEXT("push rcx");
  writeTEXT("; para: de:");
}

void X86::writeParaDeAttr(int de_type, int expecting_type,
                          pair<pair<int, bool>, string> &lv) {
  writeTEXT("; para: de attr:");
  writeAttribution(de_type, expecting_type, lv);
  writeTEXT("push rcx");
  writeTEXT("; para: ate:");
}

void X86::writeParaAte(int ate_type, int lvtype) {
  writeTEXT("pop rax");
  writeCast(ate_type, lvtype);
  writeTEXT("push rax");
}

// The loop variable's slot index sits at [rsp+8] and the final value at
// [rsp] while the body runs.
void X86::writeParaEntry(const string &lvname, bool decreasing,
                         const string &lbpara, const string &lbfim) {
  writeTEXT("mov rcx, [rsp+8]");
  writeTEXT("lea rdx, [" + X86::makeID(lvname) + "]");
  writeTEXT("mov eax, dword [rdx + rcx * SIZEOF_SLOT]");
  writeTEXT("mov ebx, dword [rsp]");
  writeTEXT("cmp eax, ebx");
  writeTEXT(decreasing ? "jl " + lbfim : "jg near " + lbfim);
  writeTEXT(lbpara + ":");
}

void X86::writeParaNext(const string &lvname, bool hasPasso, bool decreasing,
                        const string &step, const string &lbpara,
                        const string &lbfim) {
  writeTEXT("mov rcx, [rsp+8]");
  writeTEXT("lea rdx, [" + X86::makeID(lvname) + "]");
  writeTEXT("mov eax, dword [rdx + rcx * SIZEOF_SLOT]");
  if (!hasPasso) {
    writeTEXT("inc eax");
  } else if (decreasing) {
    writeTEXT("sub eax, " + step);
  } else {
    writeTEXT("add eax, " + step);
  }
  writeTEXT("mov ebx, dword [rsp]");
  writeTEXT("cmp eax, ebx");
  writeTEXT((hasPasso && decreasing) ? "jl near " + lbfim : "jg near " + lbfim);
  writeTEXT("lea rdx, [" + X86::makeID(lvname) + "]");
  writeTEXT("mov rcx, [rsp+8]");
  writeTEXT("lea rdx, [rdx + rcx * SIZEOF_SLOT]");
  writeTEXT("movsxd rax, eax");
  writeTEXT("mov [rdx], rax");
  writeTEXT("jmp " + lbpara);
  writeTEXT(lbfim + ":");

  writeTEXT("movsxd rbx, dword [rsp]");
  writeTEXT("lea rdx, [" + X86::makeID(lvname) + "]");
  writeTEXT("mov rcx, [rsp+8]");
  writeTEXT("lea rdx, [rdx + rcx * SIZEOF_SLOT]");
  writeTEXT("mov [rdx], rbx");
  writeTEXT("pop rax");
  writeTEXT("pop rcx");
  writeTEXT("; fimpara");
}

void X86::declarePrimitive(int decl_type, const string &name, int type) {
  stringstream s;
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
      s << X86::makeID(name) << " resq 1";
      writeBSS(s.str());
      break;
    default:
      GPTDisplay::self()->showError(
          "Erro interno: tipo nao suportado (X86::declarePrimitive).");
      exit(1);
    }
  } else if (decl_type == VAR_PARAM) {
    _subprograms[currentScope()].declareParam(name, type);
  } else if (decl_type == VAR_LOCAL) {
    _subprograms[currentScope()].declareLocal(name);
  } else {
    GPTDisplay::self()->showError("Erro interno: X86::declarePrimitive).");
    exit(1);
  }
}

void X86::declareMatrix(int decl_type, int type, string name,
                        list<string> dims) {
  int size = (int)matrixElements(dims);

  if (decl_type == VAR_GLOBAL) {
    stringstream s;
    switch (type) {
    case TIPO_INTEIRO:
    case TIPO_REAL:
    case TIPO_CARACTERE:
    case TIPO_LOGICO:
    case TIPO_LITERAL:
      _globalBytes += (long long)size * 8;
      checkVariableBytes(_globalBytes, SymbolTable::GlobalScope);
      s << X86::makeID(name) << " resq " << size;
      break;
    default:
      GPTDisplay::self()->showError(
          "Erro interno: tipo nao suportado (X86::declarePrimitive).");
      exit(1);
    }
    writeBSS(s.str());
  } else if (decl_type == VAR_PARAM) {
    _subprograms[currentScope()].declareParam(name, type, size);
  } else if (decl_type == VAR_LOCAL) {
    _subprograms[currentScope()].declareLocal(name, size);
  } else {
    GPTDisplay::self()->showError("Erro interno: X86::declareMatrix).");
    exit(1);
  }
}

// The escapes of a G-Portugol literal, then the quote and the control
// characters as numbers: a raw line break, or the 0x1A byte NASM also reads
// as one, would end the db line and start a new line of source.
string X86::toNasmString(string str) {
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
    if (c == '\'' || c < 32 || c == 127) {
      s << "'," << (int)c << ",'";
    } else {
      s << c;
    }
  }
  return s.str();
}

string X86::addGlobalLiteral(string str) {
  stringstream s;
  string lb = createLabel(false, "str");
  s << lb << " db '" << toNasmString(str) << "',0";
  writeDATA(s.str());
  return lb;
}

string X86::createLabel(bool local, string tmpl) {
  static int c = 0;
  stringstream s;
  if (local) {
    s << ".__" << tmpl << "_" << c;
  } else {
    s << "__" << tmpl << "$" << c;
  }
  c++;
  return s.str();
}

string X86::source() {
  stringstream str;

  //.data footer
  writeDATA("datasize   equ     $ - data_no");
#ifndef WIN32
  // Last in .bss, so bsssize also counts what writeBSS() appends.
  _bss << "    bsssize equ $ - $$\n";
#endif

  str << _head.str() << _bss.str() << _data.str();

  //.text header
  str << "section .text" << endl;
  str << "start_no equ $" << endl;
#ifdef WIN32
  str << "text_start:" << endl;
#endif

  for (map<string, X86SubProgram>::iterator it = _subprograms.begin();
       it != _subprograms.end(); ++it) {
    str << it->second.source();
  }

  str << _lib.str();

  return str.str();
}

// Reals travel as the 64 bits of a double in the integer registers and
// stack slots; the SSE registers are only used while operating on them.
void X86::writeCast(int e1, int e2) {
  if ((e2 == TIPO_LOGICO) &&
      ((e1 == TIPO_REAL) || (e1 == TIPO_INTEIRO) || (e1 == TIPO_CARACTERE))) {
    writeLogicalOf(e1);
  } else if ((e1 != TIPO_REAL) && (e2 == TIPO_REAL)) {
    writeTEXT("cvtsi2sd xmm0, eax");
    writeTEXT("movq rax, xmm0");
  } else if ((e1 == TIPO_REAL) && (e2 != TIPO_REAL)) {
    // truncates, like the C cast
    writeTEXT("movq xmm0, rax");
    writeTEXT("cvttsd2si eax, xmm0");
  }
}

// 1 in eax when the value in rax is not zero, else 0. A real is zero when
// no bit but the sign is set, so -0.0 is falso and a NaN is verdadeiro, as
// with x != 0 in C.
void X86::writeLogicalOf(int type) {
  if (type == TIPO_REAL) {
    writeTEXT("shl rax, 1");
  } else {
    writeTEXT("test eax, eax");
  }
  writeTEXT("setnz al");
  writeTEXT("movzx eax, al");
}

// Integer values are computed in 32 bits, so the slot gets the sign
// extension before being stored; literals and reals already fill 64 bits.
void X86::writeSignExtend(int type) {
  if ((type != TIPO_REAL) && (type != TIPO_LITERAL)) {
    writeTEXT("movsxd rax, eax");
  }
}

// Left operand from rax into xmm0 and right from rbx into xmm1, converting
// whichever side is an integer.
void X86::writeLoadReals(int e1, int e2) {
  writeTEXT(e1 == TIPO_REAL ? "movq xmm0, rax" : "cvtsi2sd xmm0, eax");
  writeTEXT(e2 == TIPO_REAL ? "movq xmm1, rbx" : "cvtsi2sd xmm1, ebx");
}

void X86::writeAttribution(int e1, int e2, pair<pair<int, bool>, string> &lv) {
  writeTEXT("pop rax");
  writeTEXT("pop rcx");

  writeCast(e1, e2);
  writeSignExtend(e2);

  writeTEXT("lea rdx, [" + X86::makeID(lv.second) + "]");
  writeTEXT("lea rdx, [rdx + rcx * SIZEOF_SLOT]");
  writeTEXT("mov [rdx], rax");
}

void X86::writeOuExpr() {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  writeTEXT("cmp eax, 0");
  writeTEXT("setne al");
  writeTEXT("and eax, 0xff");
  writeTEXT("cmp ebx, 0");
  writeTEXT("setne bl");
  writeTEXT("and ebx, 0xff");
  writeTEXT("or al, bl");

  writeTEXT("push rax");
}

void X86::writeEExpr() {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  writeTEXT("cmp eax, 0");
  writeTEXT("setne al");
  writeTEXT("and eax, 0xff");
  writeTEXT("cmp ebx, 0");
  writeTEXT("setne bl");
  writeTEXT("and ebx, 0xff");
  writeTEXT("and al, bl");

  writeTEXT("push rax");
}

void X86::writeBitOuExpr() {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  writeTEXT("or eax, ebx");

  writeTEXT("push rax");
}

void X86::writeBitXouExpr() {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  writeTEXT("xor eax, ebx");

  writeTEXT("push rax");
}

void X86::writeBitEExpr() {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  writeTEXT("and eax, ebx");

  writeTEXT("push rax");
}

// A literal compared with anything goes through strcmp, which returns 1
// when the strings are equal. Reals compare as doubles, so 3.0 = 3 holds
// and NaN is different from everything, including itself.
void X86::writeIgualExpr(int e1, int e2) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  if ((e1 == TIPO_LITERAL) || (e2 == TIPO_LITERAL)) {
    writeTEXT("addarg rax");
    writeTEXT("addarg rbx");
    writeTEXT("call strcmp");
    writeTEXT("clargs 2");
  } else if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeLoadReals(e1, e2);
    writeTEXT("ucomisd xmm0, xmm1");
    writeTEXT("sete al");
    writeTEXT("setnp bl");
    writeTEXT("and al, bl");
    writeTEXT("and eax, 0xff");
  } else {
    writeTEXT("cmp eax, ebx");
    writeTEXT("sete al");
    writeTEXT("and eax, 0xff");
  }

  writeTEXT("push rax");
}

void X86::writeDiferenteExpr(int e1, int e2) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  if ((e1 == TIPO_LITERAL) || (e2 == TIPO_LITERAL)) {
    writeTEXT("addarg rax");
    writeTEXT("addarg rbx");
    writeTEXT("call strcmp");
    writeTEXT("clargs 2");
    writeTEXT("cmp eax, 1");
    writeTEXT("setne al");
    writeTEXT("and eax, 0xff");
  } else if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeLoadReals(e1, e2);
    writeTEXT("ucomisd xmm0, xmm1");
    writeTEXT("setne al");
    writeTEXT("setp bl");
    writeTEXT("or al, bl");
    writeTEXT("and eax, 0xff");
  } else {
    writeTEXT("cmp eax, ebx");
    writeTEXT("setne al");
    writeTEXT("and eax, 0xff");
  }

  writeTEXT("push rax");
}

// Ordering of two literals compares their lengths, as the interpreter
// does. For reals only seta and setae are used: comisd sets CF and ZF on
// an unordered result, so a NaN operand gives falso, as in C. The
// less-than forms swap the operands to get there.
void X86::writeOrderExpr(int e1, int e2, const string &intSet,
                         const string &realSet, bool swapReals) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeLoadReals(e1, e2);
    writeTEXT(swapReals ? "comisd xmm1, xmm0" : "comisd xmm0, xmm1");
    writeTEXT(realSet + " al");
    writeTEXT("and eax, 0xff");
  } else if ((e1 == TIPO_LITERAL) && (e2 == TIPO_LITERAL)) {
    writeTEXT("push rbx");

    writeTEXT("addarg rax");
    writeTEXT("call strlen");
    writeTEXT("clargs 1");

    writeTEXT("pop rbx");
    writeTEXT("push rax");

    writeTEXT("addarg rbx");
    writeTEXT("call strlen");
    writeTEXT("clargs 1");

    writeTEXT("mov ebx, eax");
    writeTEXT("pop rax");

    writeTEXT("cmp eax, ebx");
    writeTEXT(intSet + " al");
    writeTEXT("and eax, 0xff");
  } else {
    writeTEXT("cmp eax, ebx");
    writeTEXT(intSet + " al");
    writeTEXT("and eax, 0xff");
  }

  writeTEXT("push rax");
}

void X86::writeMaiorExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "setg", "seta", false);
}

void X86::writeMenorExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "setl", "seta", true);
}

void X86::writeMaiorEqExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "setge", "setae", false);
}

void X86::writeMenorEqExpr(int e1, int e2) {
  writeOrderExpr(e1, e2, "setle", "setae", true);
}

void X86::writeRealArith(int e1, int e2, const string &op) {
  writeLoadReals(e1, e2);
  writeTEXT(op + " xmm0, xmm1");
  writeTEXT("movq rax, xmm0");
}

void X86::writeMaisExpr(int e1, int e2) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeRealArith(e1, e2, "addsd");
  } else {
    writeTEXT("add eax, ebx");
  }

  writeTEXT("push rax");
}

void X86::writeMenosExpr(int e1, int e2) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeRealArith(e1, e2, "subsd");
  } else {
    writeTEXT("sub eax, ebx");
  }

  writeTEXT("push rax");
}

// With the divisor in rbx, a zero ends the program with the interpreter's
// message. A real divisor is tested without its sign bit so that -0.0 also
// counts as zero.
void X86::writeDivisorCheck(int line, bool realDivisor) {
  string ok = createLabel(true, "div");
  stringstream s;

  if (realDivisor) {
    writeTEXT("mov rcx, rbx");
    writeTEXT("shl rcx, 1");
  } else {
    writeTEXT("cmp ebx, 0");
  }
  writeTEXT("jne " + ok);
  s << "addarg " << line;
  writeTEXT(s.str());
  writeTEXT("call div_zero_error");
  writeTEXT(ok + ":");
}

// idiv traps on INT_MIN / -1, so a divisor of -1 is handled without it:
// the quotient is the negation, which wraps to INT_MIN, and the remainder
// is 0.
void X86::writeSignedDivide(int line, bool remainder) {
  writeDivisorCheck(line, false);
  string divide = createLabel(true, remainder ? "imod" : "idiv");
  string done = createLabel(true, remainder ? "endmod" : "enddiv");
  writeTEXT("cmp ebx, -1");
  writeTEXT("jne " + divide);
  writeTEXT(remainder ? "xor eax, eax" : "neg eax");
  writeTEXT("jmp " + done);
  writeTEXT(divide + ":");
  writeTEXT("cdq");
  writeTEXT("idiv ebx");
  if (remainder) {
    writeTEXT("mov eax, edx");
  }
  writeTEXT(done + ":");
}

void X86::writeDivExpr(int e1, int e2, int line) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeDivisorCheck(line, e2 == TIPO_REAL);
    writeRealArith(e1, e2, "divsd");
  } else {
    writeSignedDivide(line, false);
  }

  writeTEXT("push rax");
}

void X86::writeMultipExpr(int e1, int e2) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  if ((e1 == TIPO_REAL) || (e2 == TIPO_REAL)) {
    writeRealArith(e1, e2, "mulsd");
  } else {
    writeTEXT("imul eax, ebx");
  }

  writeTEXT("push rax");
}

void X86::writeModExpr(int line) {
  writeTEXT("pop rbx");
  writeTEXT("pop rax");

  writeSignedDivide(line, true);

  writeTEXT("push rax");
}

void X86::writeUnaryNeg(int etype) {
  writeTEXT("pop rax");

  if (etype == TIPO_REAL) {
    writeTEXT("btc rax, 63");
  } else {
    writeTEXT("neg eax");
  }

  writeTEXT("push rax");
}

void X86::writeUnaryNot() {
  writeTEXT("pop rax");

  writeTEXT("mov ebx, eax");
  writeTEXT("xor eax, eax");
  writeTEXT("cmp ebx, 0");
  writeTEXT("sete al");

  writeTEXT("push rax");
}

void X86::writeRealToLogical() {
  writeTEXT("pop rax");
  writeLogicalOf(TIPO_REAL);
  writeTEXT("push rax");
}

void X86::writeUnaryBitNotExpr() {
  writeTEXT("pop rax");
  writeTEXT("not eax");

  writeTEXT("push rax");
}

// push sign-extends a 32-bit immediate, so a real and an integer literal
// above INT_MAX (only reachable as the operand of a unary minus) go through
// rax.
void X86::writeLiteralExpr(int type, const string &src) {
  if (type == TIPO_REAL) {
    writeTEXT("mov rax, " + src);
    writeTEXT("push rax");
  } else if (type == TIPO_INTEIRO && strtoll(src.c_str(), NULL, 10) > INT_MAX) {
    writeTEXT("mov eax, " + src);
    writeTEXT("push rax");
  } else {
    writeTEXT("push " + src);
  }
}

// The message names only the subscript that failed, so for a matrix with
// several dimensions it shows "m[7]" where the interpreter shows "m[1][7]".
void X86::writeIndexCheck(const string &name, int line, int size) {
  map<string, string>::iterator lit = _nameLiterals.find(name);
  if (lit == _nameLiterals.end()) {
    lit = _nameLiterals.insert(make_pair(name, addGlobalLiteral(name))).first;
  }
  string ok = createLabel(true, "idx");

  stringstream s;
  s << "cmp eax, " << size;
  writeTEXT(s.str());
  writeTEXT("jb " + ok);
  writeTEXT("movsxd rax, eax");
  writeTEXT("addarg rax");
  writeTEXT("addarg " + lit->second);
  s.str("");
  s << "addarg " << line;
  writeTEXT(s.str());
  writeTEXT("call index_error");
  writeTEXT(ok + ":");
}

void X86::writeLValueExpr(pair<pair<int, bool>, string> &lv) {
  writeTEXT("lea rdx, [" + X86::makeID(lv.second) + "]");
  writeTEXT("lea rdx, [rdx + rcx * SIZEOF_SLOT]");

  if (lv.first.second) { // using matrix (ie mat), push matrix address
                         //(probably passing mat to a function f(mm[])
    writeTEXT("push rdx");
  } else { // not using matrix (ie. mat[1] or x), push the value of var/index
    writeTEXT("push qword [rdx]");
  }
}
