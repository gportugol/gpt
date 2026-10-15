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

#ifndef BASEPORTUGOLPARSER_HPP
#define BASEPORTUGOLPARSER_HPP

#include <string>

/**
 * Parser error messages and reporting helpers inherited from the ANTLR2
 * parser.
 */
class BasePortugolParser {
public:
  static int reportParserError(int line, const std::string &expecting,
                               const std::string &found = "",
                               const std::string &after = "");

  static void printTip(const std::string &tip, int line, int code);

  static const std::string expecting_algorithm_name;
  static const std::string expecting_variable;
  static const std::string expecting_datatype;
  static const std::string expecting_datatype_pl;
  static const std::string expecting_identifier;
  static const std::string expecting_expression;
  static const std::string expecting_valid_sentence;
  static const std::string expecting_attr_op;
  static const std::string expecting_fimse;
  static const std::string expecting_fimvar_or_var;
  static const std::string expecting_stm_or_fim;
  static const std::string expecting_stm_or_fimse;
  static const std::string expecting_stm_or_fimenq;
  static const std::string expecting_stm_or_fimpara;
  static const std::string expecting_stm_or_ate;
  static const std::string expecting_eof_or_function;
  static const std::string expecting_function_name;
  static const std::string expecting_param_or_fparen;
};

#endif // BASEPORTUGOLPARSER_HPP
