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

#ifndef PORTUGOL2CTRANSLATOR_HPP
#define PORTUGOL2CTRANSLATOR_HPP

#include "PortugolParser.h"
#include "SymbolTable.hpp"

#include <list>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/*
 * G-Portugol to C translator.
 *
 * Port of the ANTLR2 tree-walker (pt2c.g) to the ANTLR4 parse tree. The
 * production structure and the generated text are kept identical to the
 * original walker: a fixed runtime prelude (imprima, leia_*, matrix_*,
 * str_comp, return_literal, collect/cleanup), function prototypes, global
 * variables, main() and the algorithm's functions.
 */
class Portugol2CTranslator {
public:
  Portugol2CTranslator(SymbolTable &st);

  std::string translate(PortugolParser::AlgoritmoContext *tree);

private:
  // pair<type, C text>, the "production" struct of pt2c.g
  typedef std::pair<int, std::string> Production;

  SymbolTable &_stable;

  int _currentScopeType;
  std::string _currentScope;
  std::string _indent;

  std::stringstream _scope_init_stms;
  std::stringstream _txt;
  std::stringstream _head;

  void indent();
  void unindent();
  void init(const std::string &name);
  void setScope(const std::string &scope);
  void addPrototype(const std::string &str);
  void writeln(const std::string &str, bool doIndent = true);
  void write(const std::string &str, bool doIndent = true);
  void writeInitStms();
  void addInitStm(const std::string &s);

  std::string translateFunctionName(const std::string &id, int type);
  std::string translateFunctionParams(const std::string &id,
                                      std::list<Production> &params);
  std::string translateType(int type);
  std::string translateBinExpr(const Production &left, const Production &right,
                               size_t optoken);

  void variaveis(const std::vector<PortugolParser::Var_declContext *> &decls);
  void principal(PortugolParser::Stm_blockContext *ctx);
  void funcDecls(PortugolParser::Func_declsContext *ctx);

  void stmBlock(PortugolParser::Stm_blockContext *ctx);
  void stmList(PortugolParser::Stm_listContext *ctx);
  void stm(PortugolParser::StmContext *ctx);
  void stmAttr(PortugolParser::Stm_attrContext *ctx);
  void stmRet(PortugolParser::Stm_retContext *ctx);
  void stmSe(PortugolParser::Stm_seContext *ctx);
  void stmEnquanto(PortugolParser::Stm_enquantoContext *ctx);
  void stmRepita(PortugolParser::Stm_repitaContext *ctx);
  void stmPara(PortugolParser::Stm_paraContext *ctx);

  Production lvalue(PortugolParser::LvalueContext *ctx);
  Production fcall(PortugolParser::FcallContext *ctx, int expct_type);
  Production expr(PortugolParser::ExprContext *ctx, int expct_type);
  Production exprE(PortugolParser::Expr_eContext *ctx, int expct_type);
  Production exprBitOu(PortugolParser::Expr_bit_ouContext *ctx, int expct_type);
  Production exprBitXou(PortugolParser::Expr_bit_xouContext *ctx,
                        int expct_type);
  Production exprBitE(PortugolParser::Expr_bit_eContext *ctx, int expct_type);
  Production exprIgual(PortugolParser::Expr_igualContext *ctx, int expct_type);
  Production exprRelacional(PortugolParser::Expr_relacionalContext *ctx,
                            int expct_type);
  Production exprAd(PortugolParser::Expr_adContext *ctx, int expct_type);
  Production exprMultip(PortugolParser::Expr_multipContext *ctx,
                        int expct_type);
  Production exprUnario(PortugolParser::Expr_unarioContext *ctx,
                        int expct_type);
  Production element(PortugolParser::Expr_elementoContext *ctx, int expct_type);
  Production literal(PortugolParser::LiteralContext *ctx);

  // Folds a flat "operand (op operand)*" chain left to right, like the
  // left-associative nested nodes of the ANTLR2 AST.
  template <typename Ctx, typename Fn>
  Production binaryChain(Ctx *ctx, int expct_type, Fn operand);

  Production combine(const Production &left, const Production &right,
                     size_t optoken);

  static std::string intLiteral(const std::string &text);
  static std::string unquote(const std::string &text);
  static std::vector<std::string>
  dimensoes(PortugolParser::Tipo_declContext *ctx);
  static int binaryType(int left, int right, size_t optoken);
  static char matrixTypeChar(int type);
};

#endif // PORTUGOL2CTRANSLATOR_HPP
