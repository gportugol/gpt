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

// Error message constants
const char *BasePortugolParser::expecting_algorithm_name = "nome do algoritmo";
const char *BasePortugolParser::expecting_variable = "variável";
const char *BasePortugolParser::expecting_datatype = "tipo de dado";
const char *BasePortugolParser::expecting_datatype_pl = "tipo de dado (plural)";
const char *BasePortugolParser::expecting_fimvar_or_var =
    "\"fim-variáveis\" ou declaração de variável";
const char *BasePortugolParser::expecting_eof_or_function =
    "fim do arquivo ou declaração de função";
const char *BasePortugolParser::expecting_expression = "expressão";
const char *BasePortugolParser::expecting_function_name = "nome da função";
const char *BasePortugolParser::expecting_param_or_fparen = "parâmetro ou ')'";
const char *BasePortugolParser::expecting_stm_or_fim = "comando ou \"fim\"";
const char *BasePortugolParser::expecting_stm_or_fimse =
    "comando ou \"fim-se\"";
const char *BasePortugolParser::expecting_stm_or_fimenq =
    "comando ou \"fim-enquanto\"";
const char *BasePortugolParser::expecting_stm_or_ate = "comando ou \"até\"";
const char *BasePortugolParser::expecting_stm_or_fimpara =
    "comando ou \"fim-para\"";

BasePortugolParser::BasePortugolParser() {}

BasePortugolParser::~BasePortugolParser() {}

int BasePortugolParser::reportParserError(int line,
                                          const std::string &expecting,
                                          const std::string &found,
                                          const std::string &after) {
  std::stringstream s;
  s << "Esperando " << expecting;

  if (!found.empty()) {
    s << ", encontrado: " << found;
  }

  if (!after.empty()) {
    s << " (após \"" << after << "\")";
  }

  return GPTDisplay::self()->add(s.str(), line);
}

void BasePortugolParser::printTip(const std::string &tip, int line, int code) {
  GPTDisplay::self()->addTip(tip, line, code);
}
