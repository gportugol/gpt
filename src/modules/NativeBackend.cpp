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

#include "NativeBackend.hpp"
#include "GPTDisplay.hpp"
#include "Symbol.hpp"
#include "SymbolTable.hpp"
#include "config.h"

#include <sstream>
#include <stdlib.h>
#include <string.h>

string NativeBackend::toChar(const string &str) {
  stringstream s;
  if (str.length() == 0) {
    return "0";
  } else if (str[0] != '\\') {
    s << (int)(unsigned char)str[0];
    return s.str();
  } else {
    string ret;
    switch (str[1]) {
    case '0':
      ret = "0";
      break;
    case 'n':
      ret = "10";
      break;
    case 'r':
      ret = "13";
      break;
    case 't':
      ret = "9";
      break;
    default:
      s << (int)(unsigned char)str[1];
      ret = s.str();
    }
    return ret;
  }
}

// The literal as the 64 bits of its double, in hexadecimal.
string NativeBackend::toReal(const string &str) {
  double value = atof(str.c_str());
  unsigned long long bits;
  memcpy(&bits, &value, sizeof bits);

  stringstream s;
  s << "0x" << hex << bits;

  return s.str();
}

string NativeBackend::translateFuncLeia(int type) {
  switch (type) {
  case TIPO_REAL:
    return "leia_real";
  case TIPO_LITERAL:
    return "leia_literal";
  case TIPO_CARACTERE:
    return "leia_caractere";
  case TIPO_LOGICO:
    return "leia_logico";
  case TIPO_INTEIRO:
    return "leia_inteiro";
  default:
    GPTDisplay::self()->showError(
        "Erro interno: tipo nao suportado (NativeBackend::translateFuncLeia).");
    exit(1);
  }
}

// The semantic analyzer keeps every dimension and their product between 1
// and 2^26, so anything else here is a bug and stops the generation.
long long NativeBackend::matrixElements(const list<string> &dims) {
  long long elements = 1;
  for (list<string>::const_iterator it = dims.begin(); it != dims.end(); ++it) {
    long long size = strtoll((*it).c_str(), NULL, 10);
    if (size < 1 || size > MaxVariableBytes ||
        (elements *= size) > MaxVariableBytes) {
      GPTDisplay::self()->showError("Erro interno: dimensao fora do limite "
                                    "(NativeBackend::matrixElements).");
      exit(1);
    }
  }
  return elements;
}

void NativeBackend::checkVariableBytes(long long bytes, const string &scope) {
  if (bytes <= MaxVariableBytes) {
    return;
  }
  stringstream s;
  s << PACKAGE << ": ";
  if (scope == SymbolTable::GlobalScope) {
    s << "as variáveis globais";
  } else {
    s << "as variáveis locais da função \"" << scope << "\"";
  }
  s << " ocupam mais de 1 GiB, o limite do executável nativo.";
  GPTDisplay::self()->showError(s.str());
  exit(1);
}
