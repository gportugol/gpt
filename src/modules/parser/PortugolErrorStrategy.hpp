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

#ifndef PORTUGOLERRORSTRATEGY_HPP
#define PORTUGOLERRORSTRATEGY_HPP

#include "antlr4-runtime.h"

#include <string>

/*
 * Reports syntax errors in the GPT format (Portuguese, "file:line" and
 * hints) instead of the default ANTLR4 messages, reproducing the exception
 * handlers of the ANTLR2 parser.g: "Esperando X, encontrado Y" when the
 * unexpected token is on the line of the last consumed token, otherwise
 * "Esperando X após "Z"" on the line of that token Z.
 */
class PortugolErrorStrategy : public antlr4::DefaultErrorStrategy {
public:
  void reportError(antlr4::Parser *recognizer,
                   const antlr4::RecognitionException &e) override;

  void reportUnwantedToken(antlr4::Parser *recognizer) override;
  void reportMissingToken(antlr4::Parser *recognizer) override;

  static std::string tokenName(antlr4::Parser *recognizer, size_t type);

  static std::string tokenDescription(antlr4::Parser *recognizer,
                                      antlr4::Token *token);

private:
  void report(antlr4::Parser *recognizer, antlr4::Token *offending);

  std::string expectingText(antlr4::Parser *recognizer,
                            const antlr4::misc::IntervalSet &expected,
                            antlr4::Token *offending);
};

/*
 * Lexical errors: invalid characters and unterminated quotes.
 */
class PortugolLexerErrorListener : public antlr4::BaseErrorListener {
public:
  void syntaxError(antlr4::Recognizer *recognizer,
                   antlr4::Token *offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string &msg,
                   std::exception_ptr e) override;
};

/*
 * Token checks the ANTLR2 lexer performed in its actions (identifiers with
 * non-ASCII characters, non-decimal integer literals, unterminated comments).
 * Must run over the token stream before parsing.
 */
void validateTokens(antlr4::CommonTokenStream &tokens);

#endif // PORTUGOLERRORSTRATEGY_HPP
