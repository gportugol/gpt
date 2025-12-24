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
 * Base class for Portugol parser providing error handling and utilities
 * Adapted for ANTLR4 C++ runtime
 */
class BasePortugolParser {
public:
  BasePortugolParser();
  virtual ~BasePortugolParser();

  // Error handling
  int reportParserError(int line, const std::string &expecting,
                        const std::string &found,
                        const std::string &after = "");

  void printTip(const std::string &tip, int line, int code);

  const std::string &algorithmName() const { return _name; }

  // Error messages
  static const char *expecting_algorithm_name;
  static const char *expecting_variable;
  static const char *expecting_datatype;
  static const char *expecting_datatype_pl;
  static const char *expecting_fimvar_or_var;
  static const char *expecting_eof_or_function;
  static const char *expecting_expression;
  static const char *expecting_function_name;
  static const char *expecting_param_or_fparen;
  static const char *expecting_stm_or_fim;
  static const char *expecting_stm_or_fimse;
  static const char *expecting_stm_or_fimenq;
  static const char *expecting_stm_or_ate;
  static const char *expecting_stm_or_fimpara;

protected:
  std::string _name;
};

#endif // BASEPORTUGOLPARSER_HPP
