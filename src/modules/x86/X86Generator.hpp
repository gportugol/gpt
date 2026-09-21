/***************************************************************************
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
 ***************************************************************************/

#ifndef X86GENERATOR_HPP
#define X86GENERATOR_HPP

#include "PortugolParser.h"
#include "SymbolTable.hpp"
#include "X86.hpp"

#include <list>
#include <string>
#include <utility>
#include <vector>

/*
 * x86 code generator over the ANTLR4 parse tree.
 *
 * Port of the ANTLR2 X86Walker (x86.g): each method mirrors a walker
 * production and calls the X86 helper exactly as the walker did, so the
 * generated assembly is the same. The evaluated type of expression nodes
 * (PortugolAST::getEvalType in ANTLR2) comes from SymbolTable::getEvalType(),
 * recorded by SemanticAnalyzer.
 */
class X86Generator {
public:
  X86Generator(SymbolTable &st);

  std::string generate(PortugolParser::AlgoritmoContext *tree);

private:
  // pair< pair<type, using_addr>, name>
  typedef std::pair<std::pair<int, bool>, std::string> LValue;

  void algoritmo(PortugolParser::AlgoritmoContext *ctx);
  void variaveis(const std::vector<PortugolParser::Var_declContext *> &decls,
                 int decl_type);
  void declaracao(PortugolParser::Tipo_declContext *tipo,
                  const std::vector<antlr4::tree::TerminalNode *> &ids,
                  int decl_type);
  void principal(PortugolParser::Stm_blockContext *ctx);
  void stmBlock(PortugolParser::Stm_blockContext *ctx);
  void stmList(PortugolParser::Stm_listContext *ctx);
  void stm(PortugolParser::StmContext *ctx);
  void stmAttr(PortugolParser::Stm_attrContext *ctx);
  LValue lvalue(PortugolParser::LvalueContext *ctx);
  int fcall(PortugolParser::FcallContext *ctx, int expct_type);
  void stmRet(PortugolParser::Stm_retContext *ctx);
  void stmSe(PortugolParser::Stm_seContext *ctx);
  void stmEnquanto(PortugolParser::Stm_enquantoContext *ctx);
  void stmRepita(PortugolParser::Stm_repitaContext *ctx);
  void stmPara(PortugolParser::Stm_paraContext *ctx);
  std::pair<int, std::string> passo(PortugolParser::PassoContext *ctx);

  int expr(PortugolParser::ExprContext *ctx, int expecting_type);
  int exprE(PortugolParser::Expr_eContext *ctx, int expecting_type);
  int exprBitOu(PortugolParser::Expr_bit_ouContext *ctx, int expecting_type);
  int exprBitXou(PortugolParser::Expr_bit_xouContext *ctx, int expecting_type);
  int exprBitE(PortugolParser::Expr_bit_eContext *ctx, int expecting_type);
  int exprIgual(PortugolParser::Expr_igualContext *ctx, int expecting_type);
  int exprRelacional(PortugolParser::Expr_relacionalContext *ctx,
                     int expecting_type);
  int exprAd(PortugolParser::Expr_adContext *ctx, int expecting_type);
  int exprMultip(PortugolParser::Expr_multipContext *ctx, int expecting_type);
  int exprUnario(PortugolParser::Expr_unarioContext *ctx, int expecting_type);
  int element(PortugolParser::Expr_elementoContext *ctx, int expecting_type);
  std::pair<int, std::string> literal(PortugolParser::LiteralContext *ctx);

  void funcDecls(PortugolParser::Func_declsContext *ctx);

  // Folds a flat "operand (op operand)*" chain left to right, like the
  // left-associative nested nodes of the ANTLR2 AST.
  template <typename Ctx, typename Fn>
  int binaryChain(Ctx *ctx, int expecting_type, Fn operand);

  void writeBinaryExpr(size_t op, int e1, int e2);
  static int evalType(int e1, int e2, size_t op);

  static int calcMatrixOffset(int c, std::list<int> &dims);
  static std::string intLiteral(const std::string &text);
  static std::string unquote(const std::string &text);

  SymbolTable &stable;
  X86 x86;
};

#endif // X86GENERATOR_HPP
