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

#include "BasePortugolParser.hpp"
#include "GPTDisplay.hpp"

#include <sstream>

using namespace std;

const string BasePortugolParser::expecting_algorithm_name = "nome do algoritmo";
const string BasePortugolParser::expecting_variable = "uma variável";
const string BasePortugolParser::expecting_datatype =
    "um tipo (inteiro, literal,...)";
const string BasePortugolParser::expecting_datatype_pl =
    "um tipo de conjunto/matriz (inteiros, literais,...)";
const string BasePortugolParser::expecting_identifier = "identificador";
const string BasePortugolParser::expecting_expression = "expressão";
const string BasePortugolParser::expecting_valid_sentence = "sentença válida";
const string BasePortugolParser::expecting_attr_op = "operador \":=\"";
const string BasePortugolParser::expecting_fimse = "\"fim-se\"";
const string BasePortugolParser::expecting_fimvar_or_var =
    "\"fim-variáveis\" ou declaração de variável";
const string BasePortugolParser::expecting_stm_or_fim =
    "\"fim\" ou comando válido";
const string BasePortugolParser::expecting_stm_or_fimse =
    "\"fim-se\" ou comando válido";
const string BasePortugolParser::expecting_stm_or_fimenq =
    "\"fim-enquanto\" ou comando válido";
const string BasePortugolParser::expecting_stm_or_fimpara =
    "\"fim-para\" ou comando válido";
const string BasePortugolParser::expecting_stm_or_ate =
    "\"até\" ou comando válido";
const string BasePortugolParser::expecting_eof_or_function =
    "fim de arquivo (EOF) ou \"função\"";
const string BasePortugolParser::expecting_function_name = "nome da função";
const string BasePortugolParser::expecting_param_or_fparen =
    "variável ou \")\"";

int BasePortugolParser::reportParserError(int line, const string &expecting,
                                          const string &found,
                                          const string &after) {
  string str;
  if (found.length()) {
    str = ", encontrado ";
    str += found;
  }
  if (after.length()) {
    str += " após \"";
    str += after;
    str += "\"";
  }

  stringstream s;
  s << "Esperando " << expecting << str;
  return GPTDisplay::self()->add(s.str(), line);
}

void BasePortugolParser::printTip(const string &msg, int line, int cd) {
  GPTDisplay::self()->addTip(msg, line, cd);
}
