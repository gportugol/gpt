/*
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 */

#include "Portugol2CTranslator.hpp"

using namespace std;

Portugol2CTranslator::Portugol2CTranslator(SymbolTable &st)
    : _stable(st), _indentLevel(0) {}

void Portugol2CTranslator::indent() { _indentLevel++; }

void Portugol2CTranslator::dedent() {
  if (_indentLevel > 0)
    _indentLevel--;
}

string Portugol2CTranslator::getIndent() {
  return string(_indentLevel * 2, ' ');
}

string Portugol2CTranslator::translateType(int type) {
  switch (type) {
  case TIPO_INTEIRO:
    return "int";
  case TIPO_REAL:
    return "double";
  case TIPO_CARACTERE:
    return "char";
  case TIPO_LITERAL:
    return "char*";
  case TIPO_LOGICO:
    return "int";
  default:
    return "void";
  }
}

int Portugol2CTranslator::getTypeFromTpPrim(
    PortugolParser::Tp_primContext *ctx) {
  if (ctx->T_KW_INTEIRO())
    return TIPO_INTEIRO;
  if (ctx->T_KW_REAL())
    return TIPO_REAL;
  if (ctx->T_KW_CARACTERE())
    return TIPO_CARACTERE;
  if (ctx->T_KW_LITERAL())
    return TIPO_LITERAL;
  if (ctx->T_KW_LOGICO())
    return TIPO_LOGICO;
  return TIPO_NULO;
}

int Portugol2CTranslator::getTypeFromTpPrimPl(
    PortugolParser::Tp_prim_plContext *ctx) {
  if (ctx->T_KW_INTEIROS())
    return TIPO_INTEIRO;
  if (ctx->T_KW_REAIS())
    return TIPO_REAL;
  if (ctx->T_KW_CARACTERES())
    return TIPO_CARACTERE;
  if (ctx->T_KW_LITERAIS())
    return TIPO_LITERAL;
  if (ctx->T_KW_LOGICOS())
    return TIPO_LOGICO;
  return TIPO_NULO;
}

string Portugol2CTranslator::translate(PortugolParser::AlgoritmoContext *tree) {
  if (tree) {
    visit(tree);
  }
  return _output.str();
}

antlrcpp::Any
Portugol2CTranslator::visitAlgoritmo(PortugolParser::AlgoritmoContext *ctx) {
  // Header
  _output << "/* Código gerado automaticamente por G-Portugol */\n";
  _output << "#include <stdio.h>\n";
  _output << "#include <stdlib.h>\n";
  _output << "#include <string.h>\n";
  _output << "#include <math.h>\n\n";

  // First pass: collect function declarations
  for (auto funcDecl : ctx->func_decls()) {
    string funcName = funcDecl->T_IDENTIFICADOR()->getText();
    auto retCtx = funcDecl->rettype();
    string retType = "void";

    if (auto rettypeComTipo =
            dynamic_cast<PortugolParser::RettypeComTipoContext *>(retCtx)) {
      retType = translateType(getTypeFromTpPrim(rettypeComTipo->tp_prim()));
    }

    _funcDecls << retType << " " << funcName << "(";

    auto fparams = funcDecl->fparams();
    if (fparams) {
      bool first = true;
      for (auto fparam : fparams->fparam()) {
        if (!first)
          _funcDecls << ", ";
        first = false;

        auto tipoDecl = fparam->tipo_decl();
        string paramName = fparam->T_IDENTIFICADOR()->getText();

        if (auto tipoPrim =
                dynamic_cast<PortugolParser::TipoPrimitivoContext *>(
                    tipoDecl)) {
          int type = getTypeFromTpPrim(tipoPrim->tp_prim());
          _funcDecls << translateType(type) << " " << paramName;
        } else if (auto tipoMatriz =
                       dynamic_cast<PortugolParser::TipoMatrizContext *>(
                           tipoDecl)) {
          int type = getTypeFromTpPrimPl(tipoMatriz->tp_matriz()->tp_prim_pl());
          _funcDecls << translateType(type) << " " << paramName << "[]";
        }
      }
    }
    _funcDecls << ");\n";
  }

  if (!ctx->func_decls().empty()) {
    _output << "/* Declarações de funções */\n";
    _output << _funcDecls.str() << "\n";
  }

  // Global variables
  auto varBlock = ctx->var_decl_block();
  if (varBlock) {
    _output << "/* Variáveis globais */\n";
    visit(varBlock);
    _output << "\n";
  }

  // Function definitions
  for (auto funcDecl : ctx->func_decls()) {
    visit(funcDecl);
  }

  // Main function
  _output << "int main(int argc, char *argv[]) {\n";
  indent();

  auto stmBlock = ctx->stm_block();
  if (stmBlock) {
    visit(stmBlock);
  }

  _output << getIndent() << "return 0;\n";
  dedent();
  _output << "}\n";

  return nullptr;
}

antlrcpp::Any Portugol2CTranslator::visitVar_decl_block(
    PortugolParser::Var_decl_blockContext *ctx) {
  for (auto varDecl : ctx->var_decl()) {
    visit(varDecl);
  }
  return nullptr;
}

antlrcpp::Any
Portugol2CTranslator::visitVar_decl(PortugolParser::Var_declContext *ctx) {
  auto tipoDecl = ctx->tipo_decl();

  if (auto tipoPrim =
          dynamic_cast<PortugolParser::TipoPrimitivoContext *>(tipoDecl)) {
    int type = getTypeFromTpPrim(tipoPrim->tp_prim());
    string cType = translateType(type);

    for (auto id : ctx->T_IDENTIFICADOR()) {
      _output << getIndent() << cType << " " << id->getText();
      if (type == TIPO_LITERAL) {
        _output << " = NULL";
      } else if (type == TIPO_CARACTERE) {
        _output << " = '\\0'";
      } else {
        _output << " = 0";
      }
      _output << ";\n";
    }
  } else if (auto tipoMatriz =
                 dynamic_cast<PortugolParser::TipoMatrizContext *>(tipoDecl)) {
    int type = getTypeFromTpPrimPl(tipoMatriz->tp_matriz()->tp_prim_pl());
    string cType = translateType(type);
    auto dims = tipoMatriz->tp_matriz()->dimensoes();

    for (auto id : ctx->T_IDENTIFICADOR()) {
      _output << getIndent() << cType << " " << id->getText();
      if (dims) {
        for (auto dimToken : dims->T_INT_LIT()) {
          _output << "[" << convertIntLiteral(dimToken->getText()) << "]";
        }
      }
      _output << ";\n";
    }
  }

  return nullptr;
}

antlrcpp::Any
Portugol2CTranslator::visitStm_block(PortugolParser::Stm_blockContext *ctx) {
  if (ctx->stm_list()) {
    for (auto stm : ctx->stm_list()->stm()) {
      visit(stm);
    }
  }
  return nullptr;
}

antlrcpp::Any Portugol2CTranslator::visitStmAtribuicao(
    PortugolParser::StmAtribuicaoContext *ctx) {
  auto stmAttr = ctx->stm_attr();
  string lvalue = translateLvalue(stmAttr->lvalue());
  string expr = translateExpr(stmAttr->expr());
  _output << getIndent() << lvalue << " = " << expr << ";\n";
  return nullptr;
}

antlrcpp::Any Portugol2CTranslator::visitStmChamadaFunc(
    PortugolParser::StmChamadaFuncContext *ctx) {
  _output << getIndent() << translateFcall(ctx->fcall()) << ";\n";
  return nullptr;
}

antlrcpp::Any
Portugol2CTranslator::visitStmRetorno(PortugolParser::StmRetornoContext *ctx) {
  auto stmRet = ctx->stm_ret();

  if (auto retComExpr =
          dynamic_cast<PortugolParser::RetorneComExprContext *>(stmRet)) {
    _output << getIndent() << "return " << translateExpr(retComExpr->expr())
            << ";\n";
  } else {
    _output << getIndent() << "return;\n";
  }
  return nullptr;
}

antlrcpp::Any Portugol2CTranslator::visitStmCondicional(
    PortugolParser::StmCondicionalContext *ctx) {
  auto stmSe = ctx->stm_se();
  _output << getIndent() << "if (" << translateExpr(stmSe->expr()) << ") {\n";
  indent();

  // True block
  if (stmSe->stm_list()) {
    for (auto stm : stmSe->stm_list()->stm()) {
      visit(stm);
    }
  }

  dedent();
  _output << getIndent() << "}";

  // Else block
  if (stmSe->senao_part()) {
    _output << " else {\n";
    indent();
    for (auto stm : stmSe->senao_part()->stm_list()->stm()) {
      visit(stm);
    }
    dedent();
    _output << getIndent() << "}";
  }
  _output << "\n";

  return nullptr;
}

antlrcpp::Any Portugol2CTranslator::visitStmEnquanto(
    PortugolParser::StmEnquantoContext *ctx) {
  auto stmEnq = ctx->stm_enquanto();
  _output << getIndent() << "while (" << translateExpr(stmEnq->expr())
          << ") {\n";
  indent();
  if (stmEnq->stm_list()) {
    for (auto stm : stmEnq->stm_list()->stm()) {
      visit(stm);
    }
  }
  dedent();
  _output << getIndent() << "}\n";
  return nullptr;
}

antlrcpp::Any
Portugol2CTranslator::visitStmRepita(PortugolParser::StmRepitaContext *ctx) {
  auto stmRep = ctx->stm_repita();
  _output << getIndent() << "do {\n";
  indent();
  if (stmRep->stm_list()) {
    for (auto stm : stmRep->stm_list()->stm()) {
      visit(stm);
    }
  }
  dedent();
  _output << getIndent() << "} while (!(" << translateExpr(stmRep->expr())
          << "));\n";
  return nullptr;
}

antlrcpp::Any
Portugol2CTranslator::visitStmPara(PortugolParser::StmParaContext *ctx) {
  auto stmPara = ctx->stm_para();
  string lvalue = translateLvalue(stmPara->lvalue());
  auto exprs = stmPara->expr();
  string inicio = translateExpr(exprs[0]);
  string fim = translateExpr(exprs[1]);

  // Determine step
  string passo = "1";
  string op = "<=";
  string incOp = "++";

  if (stmPara->passo()) {
    passo = convertIntLiteral(stmPara->passo()->T_INT_LIT()->getText());
    if (stmPara->passo()->T_MENOS()) {
      op = ">=";
      incOp = " -= " + passo;
    } else {
      incOp = " += " + passo;
    }
  }

  _output << getIndent() << "for (" << lvalue << " = " << inicio << "; "
          << lvalue << " " << op << " " << fim << "; " << lvalue << incOp
          << ") {\n";
  indent();
  if (stmPara->stm_list()) {
    for (auto stm : stmPara->stm_list()->stm()) {
      visit(stm);
    }
  }
  dedent();
  _output << getIndent() << "}\n";

  return nullptr;
}

antlrcpp::Any
Portugol2CTranslator::visitFunc_decls(PortugolParser::Func_declsContext *ctx) {
  string funcName = ctx->T_IDENTIFICADOR()->getText();
  auto retCtx = ctx->rettype();
  string retType = "void";

  if (auto rettypeComTipo =
          dynamic_cast<PortugolParser::RettypeComTipoContext *>(retCtx)) {
    retType = translateType(getTypeFromTpPrim(rettypeComTipo->tp_prim()));
  }

  _output << retType << " " << funcName << "(";

  auto fparams = ctx->fparams();
  if (fparams) {
    bool first = true;
    for (auto fparam : fparams->fparam()) {
      if (!first)
        _output << ", ";
      first = false;

      auto tipoDecl = fparam->tipo_decl();
      string paramName = fparam->T_IDENTIFICADOR()->getText();

      if (auto tipoPrim =
              dynamic_cast<PortugolParser::TipoPrimitivoContext *>(tipoDecl)) {
        int type = getTypeFromTpPrim(tipoPrim->tp_prim());
        _output << translateType(type) << " " << paramName;
      } else if (auto tipoMatriz =
                     dynamic_cast<PortugolParser::TipoMatrizContext *>(
                         tipoDecl)) {
        int type = getTypeFromTpPrimPl(tipoMatriz->tp_matriz()->tp_prim_pl());
        _output << translateType(type) << " " << paramName << "[]";
      }
    }
  }

  _output << ") {\n";
  indent();

  // Local variables
  auto fvarDecl = ctx->fvar_decl();
  if (auto fvarDeclComVars =
          dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(fvarDecl)) {
    for (auto varDecl : fvarDeclComVars->var_decl()) {
      visit(varDecl);
    }
  }

  // Function body
  if (ctx->stm_block()) {
    visit(ctx->stm_block());
  }

  dedent();
  _output << "}\n\n";

  return nullptr;
}

// Expression translation

string Portugol2CTranslator::translateExpr(PortugolParser::ExprContext *ctx) {
  if (!ctx)
    return "0";

  auto exprEList = ctx->expr_e();
  if (exprEList.empty())
    return "0";

  stringstream ss;
  ss << translateExprE(exprEList[0]);

  // Handle OU operations
  for (size_t i = 1; i < exprEList.size(); i++) {
    ss << " || " << translateExprE(exprEList[i]);
  }

  return ss.str();
}

string
Portugol2CTranslator::translateExprE(PortugolParser::Expr_eContext *ctx) {
  if (!ctx)
    return "0";

  auto list = ctx->expr_bit_ou();
  if (list.empty())
    return "0";

  stringstream ss;
  ss << translateExprBitOu(list[0]);

  // Handle E operations
  for (size_t i = 1; i < list.size(); i++) {
    ss << " && " << translateExprBitOu(list[i]);
  }

  return ss.str();
}

string Portugol2CTranslator::translateExprBitOu(
    PortugolParser::Expr_bit_ouContext *ctx) {
  if (!ctx)
    return "0";

  auto parts = ctx->expr_bit_xou();
  if (parts.empty())
    return "0";

  stringstream ss;
  ss << translateExprBitXou(parts[0]);

  for (size_t i = 1; i < parts.size(); ++i) {
    ss << " | " << translateExprBitXou(parts[i]);
  }
  return ss.str();
}

string Portugol2CTranslator::translateExprBitXou(
    PortugolParser::Expr_bit_xouContext *ctx) {
  if (!ctx)
    return "0";

  auto parts = ctx->expr_bit_e();
  if (parts.empty())
    return "0";

  stringstream ss;
  ss << translateExprBitE(parts[0]);

  for (size_t i = 1; i < parts.size(); ++i) {
    ss << " ^ " << translateExprBitE(parts[i]);
  }
  return ss.str();
}

string Portugol2CTranslator::translateExprBitE(
    PortugolParser::Expr_bit_eContext *ctx) {
  if (!ctx)
    return "0";

  auto parts = ctx->expr_igual();
  if (parts.empty())
    return "0";

  stringstream ss;
  ss << translateExprIgual(parts[0]);

  for (size_t i = 1; i < parts.size(); ++i) {
    ss << " & " << translateExprIgual(parts[i]);
  }
  return ss.str();
}

string Portugol2CTranslator::translateExprIgual(
    PortugolParser::Expr_igualContext *ctx) {
  if (!ctx)
    return "0";

  auto parts = ctx->expr_relacional();
  if (parts.empty())
    return "0";

  stringstream ss;
  ss << translateExprRelacional(parts[0]);

  // Get operators from children
  auto children = ctx->children;
  size_t operandIdx = 1;

  for (auto child : children) {
    if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
      int tokenType = terminal->getSymbol()->getType();
      if (operandIdx < parts.size()) {
        if (tokenType == PortugolParser::T_IGUAL) {
          ss << " == " << translateExprRelacional(parts[operandIdx]);
          operandIdx++;
        } else if (tokenType == PortugolParser::T_DIFERENTE) {
          ss << " != " << translateExprRelacional(parts[operandIdx]);
          operandIdx++;
        }
      }
    }
  }

  return ss.str();
}

string Portugol2CTranslator::translateExprRelacional(
    PortugolParser::Expr_relacionalContext *ctx) {
  if (!ctx)
    return "0";

  auto parts = ctx->expr_ad();
  if (parts.empty())
    return "0";

  stringstream ss;
  ss << translateExprAd(parts[0]);

  // Get operators from children
  auto children = ctx->children;
  size_t operandIdx = 1;

  for (auto child : children) {
    if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
      int tokenType = terminal->getSymbol()->getType();
      if (operandIdx < parts.size()) {
        if (tokenType == PortugolParser::T_MENOR) {
          ss << " < " << translateExprAd(parts[operandIdx]);
          operandIdx++;
        } else if (tokenType == PortugolParser::T_MAIOR) {
          ss << " > " << translateExprAd(parts[operandIdx]);
          operandIdx++;
        } else if (tokenType == PortugolParser::T_MENOR_EQ) {
          ss << " <= " << translateExprAd(parts[operandIdx]);
          operandIdx++;
        } else if (tokenType == PortugolParser::T_MAIOR_EQ) {
          ss << " >= " << translateExprAd(parts[operandIdx]);
          operandIdx++;
        }
      }
    }
  }

  return ss.str();
}

string
Portugol2CTranslator::translateExprAd(PortugolParser::Expr_adContext *ctx) {
  if (!ctx)
    return "0";

  auto parts = ctx->expr_multip();
  if (parts.empty())
    return "0";

  stringstream ss;
  ss << translateExprMultip(parts[0]);

  // Get operators from children
  auto children = ctx->children;
  size_t operandIdx = 1;

  for (auto child : children) {
    if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
      int tokenType = terminal->getSymbol()->getType();
      if (operandIdx < parts.size()) {
        if (tokenType == PortugolParser::T_MAIS) {
          ss << " + " << translateExprMultip(parts[operandIdx]);
          operandIdx++;
        } else if (tokenType == PortugolParser::T_MENOS) {
          ss << " - " << translateExprMultip(parts[operandIdx]);
          operandIdx++;
        }
      }
    }
  }

  return ss.str();
}

string Portugol2CTranslator::translateExprMultip(
    PortugolParser::Expr_multipContext *ctx) {
  if (!ctx)
    return "0";

  auto parts = ctx->expr_unario();
  if (parts.empty())
    return "0";

  stringstream ss;
  ss << translateExprUnario(parts[0]);

  // Get operators from children
  auto children = ctx->children;
  size_t operandIdx = 1;

  for (auto child : children) {
    if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode *>(child)) {
      int tokenType = terminal->getSymbol()->getType();
      if (operandIdx < parts.size()) {
        if (tokenType == PortugolParser::T_MULTIP) {
          ss << " * " << translateExprUnario(parts[operandIdx]);
          operandIdx++;
        } else if (tokenType == PortugolParser::T_DIV) {
          ss << " / " << translateExprUnario(parts[operandIdx]);
          operandIdx++;
        } else if (tokenType == PortugolParser::T_MOD) {
          ss << " % " << translateExprUnario(parts[operandIdx]);
          operandIdx++;
        }
      }
    }
  }

  return ss.str();
}

string Portugol2CTranslator::translateExprUnario(
    PortugolParser::Expr_unarioContext *ctx) {
  if (!ctx)
    return "0";

  if (auto neg = dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    return "-" + translateExprElemento(neg->expr_elemento());
  } else if (auto pos =
                 dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    return translateExprElemento(pos->expr_elemento());
  } else if (auto notExpr =
                 dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    return "!" + translateExprElemento(notExpr->expr_elemento());
  } else if (auto bitNot =
                 dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    return "~" + translateExprElemento(bitNot->expr_elemento());
  } else if (auto elem =
                 dynamic_cast<PortugolParser::ExprElementoContext *>(ctx)) {
    return translateExprElemento(elem->expr_elemento());
  }

  return "0";
}

string Portugol2CTranslator::translateExprElemento(
    PortugolParser::Expr_elementoContext *ctx) {
  if (!ctx)
    return "0";

  if (auto fc = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    return translateFcall(fc->fcall());
  } else if (auto lv = dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    return translateLvalue(lv->lvalue());
  } else if (auto lit =
                 dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    return translateLiteral(lit->literal());
  } else if (auto paren =
                 dynamic_cast<PortugolParser::ElemParentesesContext *>(ctx)) {
    return "(" + translateExpr(paren->expr()) + ")";
  }

  return "0";
}

string Portugol2CTranslator::translateFcall(PortugolParser::FcallContext *ctx) {
  string funcName = ctx->T_IDENTIFICADOR()->getText();
  stringstream ss;

  // Built-in function mappings
  if (funcName == "leia") {
    // Read from stdin - needs special handling based on type
    auto args = ctx->fargs();
    if (args && !args->expr().empty()) {
      ss << "scanf(\"%d\", &" << translateExpr(args->expr()[0]) << ")";
    }
    return ss.str();
  } else if (funcName == "imprima") {
    auto args = ctx->fargs();
    if (args && !args->expr().empty()) {
      ss << "printf(\"%s\", " << translateExpr(args->expr()[0]) << ")";
    }
    return ss.str();
  }

  ss << funcName << "(";

  auto args = ctx->fargs();
  if (args) {
    bool first = true;
    for (auto expr : args->expr()) {
      if (!first)
        ss << ", ";
      first = false;
      ss << translateExpr(expr);
    }
  }

  ss << ")";
  return ss.str();
}

string
Portugol2CTranslator::translateLvalue(PortugolParser::LvalueContext *ctx) {
  stringstream ss;
  ss << ctx->T_IDENTIFICADOR()->getText();

  // Array indices
  auto arraySub = ctx->array_sub();
  if (arraySub) {
    for (auto expr : arraySub->expr()) {
      ss << "[" << translateExpr(expr) << "]";
    }
  }

  return ss.str();
}

string
Portugol2CTranslator::translateLiteral(PortugolParser::LiteralContext *ctx) {
  if (auto strCtx = dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    return strCtx->T_STRING_LIT()->getText();
  } else if (auto intCtx =
                 dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    return convertIntLiteral(intCtx->T_INT_LIT()->getText());
  } else if (auto caracCtx =
                 dynamic_cast<PortugolParser::LitCaractereContext *>(ctx)) {
    return caracCtx->T_CARAC_LIT()->getText();
  } else if (dynamic_cast<PortugolParser::LitVerdadeiroContext *>(ctx)) {
    return "1";
  } else if (dynamic_cast<PortugolParser::LitFalsoContext *>(ctx)) {
    return "0";
  } else if (auto realCtx =
                 dynamic_cast<PortugolParser::LitRealContext *>(ctx)) {
    return realCtx->T_REAL_LIT()->getText();
  }

  return "0";
}

string Portugol2CTranslator::convertIntLiteral(const string &lit) {
  if (lit.length() > 2) {
    if (lit[0] == '0' && (lit[1] == 'c' || lit[1] == 'C')) {
      // Octal: 0c710 -> 0710 (C format)
      return "0" + lit.substr(2);
    } else if (lit[0] == '0' && (lit[1] == 'b' || lit[1] == 'B')) {
      // Binary: 0b1010 -> convert to decimal
      long val = std::stol(lit.substr(2), nullptr, 2);
      return to_string(val);
    }
    // Hex (0x) is already C-compatible
  }
  return lit;
}
