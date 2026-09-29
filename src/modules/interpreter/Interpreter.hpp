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

#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "InterpreterEval.hpp"
#include "PortugolParser.h"
#include "SymbolTable.hpp"

#include <list>
#include <map>
#include <string>

/*
 * Interpreter over the ANTLR4 parse tree.
 *
 * Port of the ANTLR2 InterpreterWalker (interpreter.g): walks the tree
 * statement by statement and delegates all evaluation (variables, scopes,
 * operators, conversions, built-in functions and the debugger) to
 * InterpreterEval, exactly as the original tree-walker did.
 */
class Interpreter {
public:
  Interpreter(SymbolTable &st, const std::string &host = "", int port = 0);

  // Returns the exit code ("retorne" of the main block, or 0).
  int run(PortugolParser::AlgoritmoContext *tree);

private:
  void inicio(PortugolParser::Stm_blockContext *ctx);
  void stm(PortugolParser::StmContext *ctx);
  void stmAttr(PortugolParser::Stm_attrContext *ctx);
  void stmRet(PortugolParser::Stm_retContext *ctx);
  void stmList(PortugolParser::Stm_listContext *ctx);
  void stmSe(PortugolParser::Stm_seContext *ctx);
  void stmEnquanto(PortugolParser::Stm_enquantoContext *ctx);
  void stmRepita(PortugolParser::Stm_repitaContext *ctx);
  void stmPara(PortugolParser::Stm_paraContext *ctx);
  int passo(PortugolParser::PassoContext *ctx);
  void funcDecls(PortugolParser::Func_declsContext *ctx,
                 std::list<ExprValue> &args, int line);

  ExprValue expr(PortugolParser::ExprContext *ctx);
  ExprValue exprE(PortugolParser::Expr_eContext *ctx);
  ExprValue exprBitOu(PortugolParser::Expr_bit_ouContext *ctx);
  ExprValue exprBitXou(PortugolParser::Expr_bit_xouContext *ctx);
  ExprValue exprBitE(PortugolParser::Expr_bit_eContext *ctx);
  ExprValue exprIgual(PortugolParser::Expr_igualContext *ctx);
  ExprValue exprRelacional(PortugolParser::Expr_relacionalContext *ctx);
  ExprValue exprAd(PortugolParser::Expr_adContext *ctx);
  ExprValue exprMultip(PortugolParser::Expr_multipContext *ctx);
  ExprValue exprUnario(PortugolParser::Expr_unarioContext *ctx);
  ExprValue element(PortugolParser::Expr_elementoContext *ctx);
  ExprValue literal(PortugolParser::LiteralContext *ctx);
  ExprValue fcall(PortugolParser::FcallContext *ctx);
  LValue lvalue(PortugolParser::LvalueContext *ctx);

  // Folds a flat "operand (op operand)*" chain left to right, like the
  // left-associative nested nodes of the ANTLR2 AST.
  template <typename Ctx, typename Fn>
  ExprValue binaryChain(Ctx *ctx, Fn operand);
  ExprValue binaryOp(size_t tokenType, ExprValue &left, ExprValue &right);

  static std::string parseLiteral(std::string str);
  static std::string parseChar(std::string str);
  static std::string intLiteral(const std::string &text);
  static std::string stripQuotes(const std::string &text);

  // Replaces getFilename()/getLine() of the ANTLR2 nodes for concatenated
  // input files.
  void mapLine(int line, std::string &file, int &localLine);
  void nextCmd(int line);
  int stmLine(PortugolParser::StmContext *ctx);

  InterpreterEval interpreter;
  bool _returning;

  std::map<std::string, PortugolParser::Func_declsContext *> _functions;
};

#endif // INTERPRETER_HPP
