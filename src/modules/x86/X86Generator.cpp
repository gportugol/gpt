/*
 *   Copyright (C) 2003-2006 by Thiago Silva                               *
 *   thiago.silva@kdemal.net                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 */

#include "X86Generator.hpp"
#include "GPTDisplay.hpp"
#include <sstream>

using namespace std;

X86Generator::X86Generator(SymbolTable &st)
    : _stable(st), _x86(st), _declType(X86::VAR_GLOBAL) {}

std::string X86Generator::generate(PortugolParser::AlgoritmoContext *tree) {
  visit(tree);
  return _x86.source();
}

int X86Generator::getTypeFromTpPrim(PortugolParser::Tp_primContext *ctx) {
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

int X86Generator::getTypeFromTpPrimPl(PortugolParser::Tp_prim_plContext *ctx) {
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

std::list<std::string>
X86Generator::getDimensions(PortugolParser::DimensoesContext *ctx) {
  std::list<std::string> dims;
  if (ctx) {
    for (auto dimToken : ctx->T_INT_LIT()) {
      dims.push_back(dimToken->getText());
    }
  }
  return dims;
}

int X86Generator::calcMatrixOffset(int c, std::list<int> &dims) {
  int res = 1;
  auto it = dims.rbegin();
  while (--c) {
    res *= (*it);
    ++it;
  }
  return res;
}

antlrcpp::Any
X86Generator::visitAlgoritmo(PortugolParser::AlgoritmoContext *ctx) {
  // Initialize with algorithm name
  auto declAlg = ctx->declaracao_algoritmo();
  std::string algName = declAlg->T_IDENTIFICADOR()->getText();
  _x86.init(algName);

  // Process global variables
  _declType = X86::VAR_GLOBAL;
  if (ctx->var_decl_block()) {
    visit(ctx->var_decl_block());
  }

  // Process main block (principal)
  visit(ctx->stm_block());
  _x86.writeTEXT("mov ecx, 0");
  _x86.writeExit();

  // Process functions
  for (auto funcDecl : ctx->func_decls()) {
    visit(funcDecl);
  }

  return nullptr;
}

antlrcpp::Any
X86Generator::visitVar_decl_block(PortugolParser::Var_decl_blockContext *ctx) {
  for (auto varDecl : ctx->var_decl()) {
    visit(varDecl);
  }
  return nullptr;
}

antlrcpp::Any
X86Generator::visitVar_decl(PortugolParser::Var_declContext *ctx) {
  auto tipoDecl = ctx->tipo_decl();

  if (auto tipoPrim =
          dynamic_cast<PortugolParser::TipoPrimitivoContext *>(tipoDecl)) {
    int type = getTypeFromTpPrim(tipoPrim->tp_prim());
    for (auto id : ctx->T_IDENTIFICADOR()) {
      _x86.declarePrimitive(_declType, id->getText(), type);
    }
  } else if (auto tipoMatriz =
                 dynamic_cast<PortugolParser::TipoMatrizContext *>(tipoDecl)) {
    int type = getTypeFromTpPrimPl(tipoMatriz->tp_matriz()->tp_prim_pl());
    std::list<std::string> dims =
        getDimensions(tipoMatriz->tp_matriz()->dimensoes());
    for (auto id : ctx->T_IDENTIFICADOR()) {
      _x86.declareMatrix(_declType, type, id->getText(), dims);
    }
  }

  return nullptr;
}

antlrcpp::Any
X86Generator::visitStm_block(PortugolParser::Stm_blockContext *ctx) {
  for (auto stm : ctx->stm_list()->stm()) {
    visit(stm);
  }
  return nullptr;
}

antlrcpp::Any
X86Generator::visitStmAtribuicao(PortugolParser::StmAtribuicaoContext *ctx) {
  auto stmAttr = ctx->stm_attr();
  auto lv = evaluateLvalue(stmAttr->lvalue());

  Symbol symb = _stable.getSymbol(_x86.currentScope(), lv.second, true);
  int expectingType = symb.type.primitiveType();

  _x86.writeTEXT("push ecx");

  int etype = evaluateExpr(stmAttr->expr(), expectingType);

  _x86.writeAttribution(etype, expectingType, lv);

  return nullptr;
}

antlrcpp::Any
X86Generator::visitStmChamadaFunc(PortugolParser::StmChamadaFuncContext *ctx) {
  evaluateFcall(ctx->fcall(), TIPO_ALL);
  _x86.writeTEXT("pop eax");
  return nullptr;
}

antlrcpp::Any
X86Generator::visitStmRetorno(PortugolParser::StmRetornoContext *ctx) {
  auto stmRet = ctx->stm_ret();
  bool isGlobalScope = (_x86.currentScope() == SymbolTable::GlobalScope);

  int expectingType = TIPO_NULO;
  if (isGlobalScope) {
    expectingType = TIPO_INTEIRO;
  } else {
    expectingType =
        _stable.getSymbol(SymbolTable::GlobalScope, _x86.currentScope(), true)
            .type.primitiveType();
  }

  int etype = TIPO_NULO;
  if (auto retExpr =
          dynamic_cast<PortugolParser::RetorneComExprContext *>(stmRet)) {
    etype = evaluateExpr(retExpr->expr(), expectingType);
  }

  if (isGlobalScope) {
    _x86.writeTEXT("pop ecx");
    _x86.writeExit();
  } else {
    if (expectingType != TIPO_NULO) {
      _x86.writeTEXT("pop eax");
    }
    if (expectingType == TIPO_LITERAL) {
      _x86.writeTEXT("addarg eax");
      _x86.writeTEXT("call clone_literal");
      _x86.writeTEXT("clargs 1");
    } else {
      _x86.writeCast(etype, expectingType);
    }
    _x86.writeTEXT("return");
  }

  return nullptr;
}

antlrcpp::Any
X86Generator::visitStmCondicional(PortugolParser::StmCondicionalContext *ctx) {
  auto stmSe = ctx->stm_se();
  stringstream s;

  string lbnext = _x86.createLabel(true, "next_se");
  string lbfim = _x86.createLabel(true, "fim_se");

  _x86.writeTEXT("; se: expressao");
  evaluateExpr(stmSe->expr(), TIPO_LOGICO);

  _x86.writeTEXT("; se: resultado");
  _x86.writeTEXT("pop eax");
  _x86.writeTEXT("cmp eax, 0");
  s << "je near " << lbnext;
  _x86.writeTEXT(s.str());

  _x86.writeTEXT("; se: verdadeiro:");

  // Process statements in "then" block
  for (auto stm : stmSe->stm_list()->stm()) {
    visit(stm);
  }

  bool hasElse = (stmSe->senao_part() != nullptr);

  if (hasElse) {
    s.str("");
    s << "jmp " << lbfim;
    _x86.writeTEXT(s.str());

    _x86.writeTEXT("; se: falso:");
    s.str("");
    s << lbnext << ":";
    _x86.writeTEXT(s.str());

    // Process statements in "else" block
    for (auto stm : stmSe->senao_part()->stm_list()->stm()) {
      visit(stm);
    }
  }

  _x86.writeTEXT("; se: fim:");
  s.str("");
  if (hasElse) {
    s << lbfim << ":";
  } else {
    s << lbnext << ":";
  }
  _x86.writeTEXT(s.str());

  return nullptr;
}

antlrcpp::Any
X86Generator::visitStmEnquanto(PortugolParser::StmEnquantoContext *ctx) {
  auto stmEnq = ctx->stm_enquanto();
  stringstream s;

  string lbenq = _x86.createLabel(true, "enquanto");
  string lbfim = _x86.createLabel(true, "fim_enquanto");

  s << lbenq << ":";
  _x86.writeTEXT(s.str());

  _x86.writeTEXT("; while: expressao");
  evaluateExpr(stmEnq->expr(), TIPO_LOGICO);

  _x86.writeTEXT("; while: resultado");
  _x86.writeTEXT("pop eax");
  _x86.writeTEXT("cmp eax, 0");
  s.str("");
  s << "je near " << lbfim;
  _x86.writeTEXT(s.str());

  // Process statements
  for (auto stm : stmEnq->stm_list()->stm()) {
    visit(stm);
  }

  s.str("");
  s << "jmp " << lbenq;
  _x86.writeTEXT(s.str());

  s.str("");
  s << lbfim << ":";
  _x86.writeTEXT(s.str());

  return nullptr;
}

antlrcpp::Any
X86Generator::visitStmRepita(PortugolParser::StmRepitaContext *ctx) {
  auto stmRep = ctx->stm_repita();
  stringstream s;

  string lbrep = _x86.createLabel(true, "repita");
  string lbfim = _x86.createLabel(true, "ate");

  s << lbrep << ":";
  _x86.writeTEXT(s.str());

  _x86.writeTEXT("; repita");

  // Process statements
  for (auto stm : stmRep->stm_list()->stm()) {
    visit(stm);
  }

  _x86.writeTEXT("; until expressao");
  evaluateExpr(stmRep->expr(), TIPO_LOGICO);

  _x86.writeTEXT("; until resultado");
  _x86.writeTEXT("pop eax");
  _x86.writeTEXT("cmp eax, 0");
  s.str("");
  s << "je near " << lbrep;
  _x86.writeTEXT(s.str());

  return nullptr;
}

antlrcpp::Any X86Generator::visitStmPara(PortugolParser::StmParaContext *ctx) {
  auto stmPara = ctx->stm_para();
  stringstream s;

  string lbpara = _x86.createLabel(true, "para");
  string lbfim = _x86.createLabel(true, "fim_para");

  _x86.writeTEXT("; para: lvalue:");
  auto lv = evaluateLvalue(stmPara->lvalue());

  Symbol symb = _stable.getSymbol(_x86.currentScope(), lv.second, true);
  int expectingType = symb.type.primitiveType();

  _x86.writeTEXT("push ecx"); // lvalue's offset to be used later

  _x86.writeTEXT("; para: de:");
  int deType = evaluateExpr(stmPara->inicio, TIPO_INTEIRO);

  _x86.writeTEXT("; para: de attr:");
  _x86.writeAttribution(deType, expectingType, lv);

  _x86.writeTEXT("push ecx");
  _x86.writeTEXT("; para: ate:");

  int ateType = evaluateExpr(stmPara->fim, TIPO_INTEIRO);

  _x86.writeTEXT("pop eax");
  _x86.writeCast(ateType, lv.first.first);
  _x86.writeTEXT("push eax"); // top stack tem "ate"

  bool hasPasso = false;
  int passoDir = 0; // 0 = crescente, 1 = decrescente
  string passoVal = "1";

  if (stmPara->passo()) {
    hasPasso = true;
    if (stmPara->passo()->T_MENOS()) {
      passoDir = 1;
    }
    passoVal = stmPara->passo()->T_INT_LIT()->getText();
  }

  // Não entrar se condição falsa
  _x86.writeTEXT("mov ecx, dword [esp+4]");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  _x86.writeTEXT(s.str());
  _x86.writeTEXT("mov eax, dword [edx + ecx * SIZEOF_DWORD]");

  _x86.writeTEXT("mov ebx, dword [esp]");
  _x86.writeTEXT("cmp eax, ebx");

  s.str("");
  if (hasPasso && passoDir) {
    s << "jl " << lbfim;
  } else {
    s << "jg near " << lbfim;
  }
  _x86.writeTEXT(s.str());

  s.str("");
  s << lbpara << ":";
  _x86.writeTEXT(s.str());

  // Process statements
  for (auto stm : stmPara->stm_list()->stm()) {
    visit(stm);
  }

  // Calcular passo
  _x86.writeTEXT("mov ecx, dword [esp+4]");
  s.str("");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  _x86.writeTEXT(s.str());
  _x86.writeTEXT("mov eax, dword [edx + ecx * SIZEOF_DWORD]");

  s.str("");
  if (!hasPasso) {
    _x86.writeTEXT("inc eax");
  } else {
    if (passoDir) {
      s << "sub eax, " << passoVal;
    } else {
      s << "add eax, " << passoVal;
    }
    _x86.writeTEXT(s.str());
  }

  // Desviar controle
  _x86.writeTEXT("mov ebx, dword [esp]");
  _x86.writeTEXT("cmp eax, ebx");

  s.str("");
  if (hasPasso && passoDir) {
    s << "jl near " << lbfim;
  } else {
    s << "jg near " << lbfim;
  }
  _x86.writeTEXT(s.str());

  s.str("");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  _x86.writeTEXT(s.str());
  _x86.writeTEXT("mov ecx, dword [esp+4]");
  _x86.writeTEXT("lea edx, [edx + ecx * SIZEOF_DWORD]");
  _x86.writeTEXT("mov dword [edx], eax");

  s.str("");
  s << "jmp " << lbpara;
  _x86.writeTEXT(s.str());

  s.str("");
  s << lbfim << ":";
  _x86.writeTEXT(s.str());

  // lvalue = ate value
  _x86.writeTEXT("mov ebx, dword [esp]");
  s.str("");
  s << "lea edx, [" << X86::makeID(lv.second) << "]";
  _x86.writeTEXT(s.str());
  _x86.writeTEXT("mov ecx, dword [esp+4]");
  _x86.writeTEXT("lea edx, [edx + ecx * SIZEOF_DWORD]");
  _x86.writeTEXT("mov dword [edx], ebx");

  // pop ate, pop lvalue offset
  _x86.writeTEXT("pop eax");
  _x86.writeTEXT("pop ecx");
  _x86.writeTEXT("; fimpara");

  return nullptr;
}

antlrcpp::Any
X86Generator::visitFunc_decls(PortugolParser::Func_declsContext *ctx) {
  string funcName = ctx->T_IDENTIFICADOR()->getText();
  _x86.createScope(funcName);

  // Process parameters
  _declType = X86::VAR_PARAM;
  auto fparams = ctx->fparams();
  if (fparams) {
    for (auto fparam : fparams->fparam()) {
      auto tipoDecl = fparam->tipo_decl();
      string paramName = fparam->T_IDENTIFICADOR()->getText();

      if (auto tipoPrim =
              dynamic_cast<PortugolParser::TipoPrimitivoContext *>(tipoDecl)) {
        int type = getTypeFromTpPrim(tipoPrim->tp_prim());
        _x86.declarePrimitive(_declType, paramName, type);
      } else if (auto tipoMatriz =
                     dynamic_cast<PortugolParser::TipoMatrizContext *>(
                         tipoDecl)) {
        int type = getTypeFromTpPrimPl(tipoMatriz->tp_matriz()->tp_prim_pl());
        std::list<std::string> dims =
            getDimensions(tipoMatriz->tp_matriz()->dimensoes());
        _x86.declareMatrix(_declType, type, paramName, dims);
      }
    }
  }

  // Process local variables
  _declType = X86::VAR_LOCAL;
  if (auto fvarDecl = dynamic_cast<PortugolParser::FvarDeclComVarsContext *>(
          ctx->fvar_decl())) {
    for (auto varDecl : fvarDecl->var_decl()) {
      visit(varDecl);
    }
  }

  // Process function body
  visit(ctx->stm_block());

  _x86.writeTEXT("return");

  return nullptr;
}

// Expression evaluation methods
int X86Generator::evaluateExpr(PortugolParser::ExprContext *ctx,
                               int expectingType) {
  if (!ctx || ctx->expr_e().empty())
    return TIPO_NULO;

  int etype = evaluateExprE(ctx->expr_e(0), expectingType);

  for (size_t i = 1; i < ctx->expr_e().size(); i++) {
    evaluateExprE(ctx->expr_e(i), expectingType);
    _x86.writeOuExpr();
    etype = TIPO_LOGICO;
  }

  return etype;
}

int X86Generator::evaluateExprE(PortugolParser::Expr_eContext *ctx,
                                int expectingType) {
  if (!ctx || ctx->expr_bit_ou().empty())
    return TIPO_NULO;

  int etype = evaluateExprBitOu(ctx->expr_bit_ou(0), expectingType);

  for (size_t i = 1; i < ctx->expr_bit_ou().size(); i++) {
    evaluateExprBitOu(ctx->expr_bit_ou(i), expectingType);
    _x86.writeEExpr();
    etype = TIPO_LOGICO;
  }

  return etype;
}

int X86Generator::evaluateExprBitOu(PortugolParser::Expr_bit_ouContext *ctx,
                                    int expectingType) {
  if (!ctx || ctx->expr_bit_xou().empty())
    return TIPO_NULO;

  int etype = evaluateExprBitXou(ctx->expr_bit_xou(0), expectingType);

  for (size_t i = 1; i < ctx->expr_bit_xou().size(); i++) {
    evaluateExprBitXou(ctx->expr_bit_xou(i), expectingType);
    _x86.writeBitOuExpr();
    etype = TIPO_INTEIRO;
  }

  return etype;
}

int X86Generator::evaluateExprBitXou(PortugolParser::Expr_bit_xouContext *ctx,
                                     int expectingType) {
  if (!ctx || ctx->expr_bit_e().empty())
    return TIPO_NULO;

  int etype = evaluateExprBitE(ctx->expr_bit_e(0), expectingType);

  for (size_t i = 1; i < ctx->expr_bit_e().size(); i++) {
    evaluateExprBitE(ctx->expr_bit_e(i), expectingType);
    _x86.writeBitXouExpr();
    etype = TIPO_INTEIRO;
  }

  return etype;
}

int X86Generator::evaluateExprBitE(PortugolParser::Expr_bit_eContext *ctx,
                                   int expectingType) {
  if (!ctx || ctx->expr_igual().empty())
    return TIPO_NULO;

  int etype = evaluateExprIgual(ctx->expr_igual(0), expectingType);

  for (size_t i = 1; i < ctx->expr_igual().size(); i++) {
    evaluateExprIgual(ctx->expr_igual(i), expectingType);
    _x86.writeBitEExpr();
    etype = TIPO_INTEIRO;
  }

  return etype;
}

int X86Generator::evaluateExprIgual(PortugolParser::Expr_igualContext *ctx,
                                    int expectingType) {
  if (!ctx || ctx->expr_relacional().empty())
    return TIPO_NULO;

  int e1 = evaluateExprRelacional(ctx->expr_relacional(0), expectingType);

  auto igualOps = ctx->T_IGUAL();
  auto difOps = ctx->T_DIFERENTE();

  for (size_t i = 1; i < ctx->expr_relacional().size(); i++) {
    int e2 = evaluateExprRelacional(ctx->expr_relacional(i), expectingType);

    if (i - 1 < igualOps.size()) {
      _x86.writeIgualExpr(e1, e2);
    } else {
      _x86.writeDiferenteExpr(e1, e2);
    }
    e1 = TIPO_LOGICO;
  }

  return e1;
}

int X86Generator::evaluateExprRelacional(
    PortugolParser::Expr_relacionalContext *ctx, int expectingType) {
  if (!ctx || ctx->expr_ad().empty())
    return TIPO_NULO;

  int e1 = evaluateExprAd(ctx->expr_ad(0), expectingType);

  auto maiorOps = ctx->T_MAIOR();
  auto menorOps = ctx->T_MENOR();
  auto maiorEqOps = ctx->T_MAIOR_EQ();
  auto menorEqOps = ctx->T_MENOR_EQ();

  size_t opIdx = 0;
  for (size_t i = 1; i < ctx->expr_ad().size(); i++) {
    int e2 = evaluateExprAd(ctx->expr_ad(i), expectingType);

    // Determine which operator (in order of appearance in grammar)
    if (opIdx < maiorOps.size()) {
      _x86.writeMaiorExpr(e1, e2);
    } else if (opIdx < maiorOps.size() + maiorEqOps.size()) {
      _x86.writeMaiorEqExpr(e1, e2);
    } else if (opIdx < maiorOps.size() + maiorEqOps.size() + menorOps.size()) {
      _x86.writeMenorExpr(e1, e2);
    } else {
      _x86.writeMenorEqExpr(e1, e2);
    }
    opIdx++;
    e1 = TIPO_LOGICO;
  }

  return e1;
}

int X86Generator::evaluateExprAd(PortugolParser::Expr_adContext *ctx,
                                 int expectingType) {
  if (!ctx || ctx->expr_multip().empty())
    return TIPO_NULO;

  int e1 = evaluateExprMultip(ctx->expr_multip(0), expectingType);

  auto maisOps = ctx->T_MAIS();
  auto menosOps = ctx->T_MENOS();

  size_t maisIdx = 0, menosIdx = 0;
  for (size_t i = 1; i < ctx->expr_multip().size(); i++) {
    int e2 = evaluateExprMultip(ctx->expr_multip(i), expectingType);

    // We need to figure out which operator was used
    // This is tricky since we don't have positional info
    // For now, assume alternating or use children order
    if (maisIdx < maisOps.size()) {
      _x86.writeMaisExpr(e1, e2);
      maisIdx++;
    } else {
      _x86.writeMenosExpr(e1, e2);
      menosIdx++;
    }

    if (e1 == TIPO_REAL || e2 == TIPO_REAL) {
      e1 = TIPO_REAL;
    }
  }

  return e1;
}

int X86Generator::evaluateExprMultip(PortugolParser::Expr_multipContext *ctx,
                                     int expectingType) {
  if (!ctx || ctx->expr_unario().empty())
    return TIPO_NULO;

  int e1 = evaluateExprUnario(ctx->expr_unario(0), expectingType);

  auto divOps = ctx->T_DIV();
  auto multOps = ctx->T_MULTIP();
  auto modOps = ctx->T_MOD();

  size_t divIdx = 0, multIdx = 0, modIdx = 0;
  for (size_t i = 1; i < ctx->expr_unario().size(); i++) {
    int e2 = evaluateExprUnario(ctx->expr_unario(i), expectingType);

    if (divIdx < divOps.size()) {
      _x86.writeDivExpr(e1, e2);
      divIdx++;
      e1 = TIPO_REAL;
    } else if (multIdx < multOps.size()) {
      _x86.writeMultipExpr(e1, e2);
      multIdx++;
    } else {
      _x86.writeModExpr();
      modIdx++;
      e1 = TIPO_INTEIRO;
    }
  }

  return e1;
}

int X86Generator::evaluateExprUnario(PortugolParser::Expr_unarioContext *ctx,
                                     int expectingType) {
  if (auto negCtx = dynamic_cast<PortugolParser::ExprNegativaContext *>(ctx)) {
    int etype = evaluateExprElemento(negCtx->expr_elemento(), expectingType);
    _x86.writeUnaryNeg(etype);
    return etype;
  } else if (auto posCtx =
                 dynamic_cast<PortugolParser::ExprPositivaContext *>(ctx)) {
    return evaluateExprElemento(posCtx->expr_elemento(), expectingType);
  } else if (auto notCtx =
                 dynamic_cast<PortugolParser::ExprNotContext *>(ctx)) {
    evaluateExprElemento(notCtx->expr_elemento(), expectingType);
    _x86.writeUnaryNot();
    return TIPO_LOGICO;
  } else if (auto bnotCtx =
                 dynamic_cast<PortugolParser::ExprBitNotContext *>(ctx)) {
    int etype = evaluateExprElemento(bnotCtx->expr_elemento(), expectingType);
    _x86.writeUnaryBitNotExpr();
    return etype;
  } else if (auto elemCtx =
                 dynamic_cast<PortugolParser::ExprElementoContext *>(ctx)) {
    return evaluateExprElemento(elemCtx->expr_elemento(), expectingType);
  }

  return TIPO_NULO;
}

int X86Generator::evaluateExprElemento(
    PortugolParser::Expr_elementoContext *ctx, int expectingType) {
  if (auto fcallCtx = dynamic_cast<PortugolParser::ElemFcallContext *>(ctx)) {
    return evaluateFcall(fcallCtx->fcall(), expectingType);
  } else if (auto lvalueCtx =
                 dynamic_cast<PortugolParser::ElemLvalueContext *>(ctx)) {
    auto lv = evaluateLvalue(lvalueCtx->lvalue());
    _x86.writeLValueExpr(lv);
    return lv.first.first;
  } else if (auto litCtx =
                 dynamic_cast<PortugolParser::ElemLiteralContext *>(ctx)) {
    auto lit = evaluateLiteral(litCtx->literal());
    _x86.writeLiteralExpr(lit.second);
    return lit.first;
  } else if (auto parenCtx =
                 dynamic_cast<PortugolParser::ElemParentesesContext *>(ctx)) {
    return evaluateExpr(parenCtx->expr(), expectingType);
  }

  return TIPO_NULO;
}

int X86Generator::evaluateFcall(PortugolParser::FcallContext *ctx,
                                int expectingType) {
  string funcName = ctx->T_IDENTIFICADOR()->getText();
  stringstream s;

  Symbol f = _stable.getSymbol(SymbolTable::GlobalScope, funcName);
  string fname;
  int type;

  if (f.lexeme == "leia") {
    fname = _x86.translateFuncLeia(funcName, expectingType);
    type = expectingType;
  } else {
    fname = f.lexeme;
    type = f.type.primitiveType();
  }

  int count = 0;
  int args = 0;
  auto fargs = ctx->fargs();

  if (fargs) {
    for (auto argExpr : fargs->expr()) {
      int ptype = f.param.paramType(count++);
      int etype = evaluateExpr(argExpr, ptype);

      if (fname == "imprima") {
        switch (etype) {
        case TIPO_INTEIRO:
          _x86.writeTEXT("addarg 'i'");
          break;
        case TIPO_REAL:
          _x86.writeTEXT("addarg 'r'");
          break;
        case TIPO_CARACTERE:
          _x86.writeTEXT("addarg 'c'");
          break;
        case TIPO_LITERAL:
          _x86.writeTEXT("addarg 's'");
          break;
        case TIPO_LOGICO:
          _x86.writeTEXT("addarg 'l'");
          break;
        }
      } else {
        _x86.writeTEXT("pop eax");
        _x86.writeCast(etype, ptype);
        _x86.writeTEXT("addarg eax");
      }
      args++;
    }
  }

  if (fname == "imprima") {
    s << "addarg " << args;
    _x86.writeTEXT(s.str());
    _x86.writeTEXT("call imprima");
    s.str("");
    s << "clargs " << ((args * 2) + 1);
    _x86.writeTEXT(s.str());
    _x86.writeTEXT("print_lf"); // \n
  } else if (f.lexeme == "leia") {
    _x86.writeTEXT(string("call ") + fname);
  } else {
    _x86.writeTEXT(string("call ") + X86::makeID(fname));
    if (args) {
      s.str("");
      s << "clargs " << args;
      _x86.writeTEXT(s.str());
    }
  }

  _x86.writeTEXT("push eax");

  return type;
}

std::pair<int, std::string>
X86Generator::evaluateLiteral(PortugolParser::LiteralContext *ctx) {
  if (auto strCtx = dynamic_cast<PortugolParser::LitStringContext *>(ctx)) {
    string text = strCtx->T_STRING_LIT()->getText();
    if (text.length() > 2) { // Has content besides quotes
      // Remove surrounding quotes
      text = text.substr(1, text.length() - 2);
      return {TIPO_LITERAL, _x86.addGlobalLiteral(text)};
    } else {
      return {TIPO_LITERAL, "0"};
    }
  } else if (auto intCtx =
                 dynamic_cast<PortugolParser::LitInteiroContext *>(ctx)) {
    string intText = intCtx->T_INT_LIT()->getText();
    // Convert Portugol number formats to NASM format
    if (intText.length() > 2) {
      if (intText[0] == '0' && (intText[1] == 'c' || intText[1] == 'C')) {
        // Octal: 0c710 -> 0o710 (NASM format)
        intText = "0o" + intText.substr(2);
      } else if (intText[0] == '0' &&
                 (intText[1] == 'b' || intText[1] == 'B')) {
        // Binary: 0b1010 -> convert to decimal for NASM compatibility
        long val = std::stol(intText.substr(2), nullptr, 2);
        intText = std::to_string(val);
      }
      // Hex (0x) is already compatible with NASM
    }
    return {TIPO_INTEIRO, intText};
  } else if (auto caracCtx =
                 dynamic_cast<PortugolParser::LitCaractereContext *>(ctx)) {
    return {TIPO_CARACTERE, _x86.toChar(caracCtx->T_CARAC_LIT()->getText())};
  } else if (dynamic_cast<PortugolParser::LitVerdadeiroContext *>(ctx)) {
    return {TIPO_LOGICO, "1"};
  } else if (dynamic_cast<PortugolParser::LitFalsoContext *>(ctx)) {
    return {TIPO_LOGICO, "0"};
  } else if (auto realCtx =
                 dynamic_cast<PortugolParser::LitRealContext *>(ctx)) {
    return {TIPO_REAL, _x86.toReal(realCtx->T_REAL_LIT()->getText())};
  }

  return {TIPO_NULO, "0"};
}

std::pair<std::pair<int, bool>, std::string>
X86Generator::evaluateLvalue(PortugolParser::LvalueContext *ctx) {
  stringstream s;
  string varName = ctx->T_IDENTIFICADOR()->getText();

  Symbol symb = _stable.getSymbol(_x86.currentScope(), varName, true);
  int type = symb.type.primitiveType();
  bool isPrim = symb.type.isPrimitive();

  std::list<int> dims;
  for (int d : symb.type.dimensions()) {
    dims.push_back(d);
  }
  int c = dims.size();

  bool usingAddr;
  if (!isPrim) {
    usingAddr = true;
    _x86.writeTEXT("push 0");
  } else {
    usingAddr = false;
  }

  auto arraySub = ctx->array_sub();
  if (arraySub) {
    for (auto indexExpr : arraySub->expr()) {
      usingAddr = false;
      evaluateExpr(indexExpr, TIPO_INTEIRO);

      int multiplier = calcMatrixOffset(c, dims);
      _x86.writeTEXT("pop eax");

      s.str("");
      s << "mov ebx, " << multiplier;
      _x86.writeTEXT(s.str());
      _x86.writeTEXT("imul ebx");
      _x86.writeTEXT("pop ebx");
      _x86.writeTEXT("add eax, ebx");
      _x86.writeTEXT("push eax");
      c--;
    }
  }

  if (symb.type.isPrimitive()) {
    _x86.writeTEXT("mov ecx, 0");
  } else {
    _x86.writeTEXT("pop ecx");
  }

  return {{type, usingAddr}, varName};
}
