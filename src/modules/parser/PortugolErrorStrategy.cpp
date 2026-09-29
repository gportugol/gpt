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

#include "PortugolErrorStrategy.hpp"

#include "BasePortugolParser.hpp"
#include "GPTDisplay.hpp"
#include "PortugolParser.h"

#include <sstream>
#include <stdlib.h>

using namespace antlr4;
using namespace std;

static bool isKeyword(size_t type) {
  return type >= PortugolParser::T_KW_ALGORITMO &&
         type <= PortugolParser::T_KW_PASSO;
}

static bool isDatatype(size_t type) {
  switch (type) {
  case PortugolParser::T_KW_INTEIRO:
  case PortugolParser::T_KW_INTEIROS:
  case PortugolParser::T_KW_REAL:
  case PortugolParser::T_KW_REAIS:
  case PortugolParser::T_KW_CARACTERE:
  case PortugolParser::T_KW_CARACTERES:
  case PortugolParser::T_KW_LITERAL:
  case PortugolParser::T_KW_LITERAIS:
  case PortugolParser::T_KW_LOGICO:
  case PortugolParser::T_KW_LOGICOS:
    return true;
  }
  return false;
}

// ANTLR 4.10+ escapes non-ASCII characters of literal names as \uXXXX.
static string unescapeUnicode(const string &str) {
  string out;
  for (string::size_type i = 0; i < str.size(); ++i) {
    if (str[i] == '\\' && i + 5 < str.size() && str[i + 1] == 'u') {
      unsigned cp = (unsigned)strtoul(str.substr(i + 2, 4).c_str(), NULL, 16);
      if (cp < 0x80) {
        out += (char)cp;
      } else if (cp < 0x800) {
        out += (char)(0xC0 | (cp >> 6));
        out += (char)(0x80 | (cp & 0x3F));
      } else {
        out += (char)(0xE0 | (cp >> 12));
        out += (char)(0x80 | ((cp >> 6) & 0x3F));
        out += (char)(0x80 | (cp & 0x3F));
      }
      i += 5;
    } else {
      out += str[i];
    }
  }
  return out;
}

static string literalText(Parser *recognizer, size_t type) {
  string lit(recognizer->getVocabulary().getLiteralName(type));
  if (lit.size() >= 2 && lit[0] == '\'' && lit[lit.size() - 1] == '\'') {
    lit = lit.substr(1, lit.size() - 2);
  }
  return unescapeUnicode(lit);
}

string PortugolErrorStrategy::tokenName(Parser *recognizer, size_t type) {
  switch (type) {
  case Token::EOF:
    return "fim de arquivo (EOF)";
  case PortugolParser::T_IDENTIFICADOR:
    return "identificador";
  case PortugolParser::T_INT_LIT:
    return "número inteiro";
  case PortugolParser::T_REAL_LIT:
    return "número real";
  case PortugolParser::T_CARAC_LIT:
    return "caractere";
  case PortugolParser::T_STRING_LIT:
    return "literal";
  case PortugolParser::T_BIT_OU:
  case PortugolParser::T_BIT_XOU:
  case PortugolParser::T_BIT_E:
  case PortugolParser::T_BIT_NOT:
  case PortugolParser::T_IGUAL:
  case PortugolParser::T_DIFERENTE:
  case PortugolParser::T_MAIOR:
  case PortugolParser::T_MENOR:
  case PortugolParser::T_MAIOR_EQ:
  case PortugolParser::T_MENOR_EQ:
  case PortugolParser::T_MAIS:
  case PortugolParser::T_MENOS:
  case PortugolParser::T_DIV:
  case PortugolParser::T_MULTIP:
  case PortugolParser::T_MOD:
    return "operador '" + literalText(recognizer, type) + "'";
  }
  if (isKeyword(type)) {
    return "\"" + literalText(recognizer, type) + "\"";
  }
  return "'" + literalText(recognizer, type) + "'";
}

string PortugolErrorStrategy::tokenDescription(Parser *recognizer,
                                               Token *token) {
  if (token->getType() == PortugolParser::T_IDENTIFICADOR) {
    return "\"" + token->getText() + "\"";
  } else if (isKeyword(token->getType())) {
    return "a palavra-chave " + tokenName(recognizer, token->getType());
  } else if (token->getType() == Token::EOF) {
    return "fim de arquivo (EOF)";
  }
  return token->getText();
}

void PortugolErrorStrategy::reportError(Parser *recognizer,
                                        const RecognitionException &e) {
  if (inErrorRecoveryMode(recognizer)) {
    return; // já reportado; ignora até re-sincronizar
  }
  beginErrorCondition(recognizer);
  Token *offending = e.getOffendingToken();
  if (auto nva = dynamic_cast<const NoViableAltException *>(&e)) {
    if (nva->getStartToken()) {
      offending = nva->getStartToken();
    }
  }
  if (!offending) {
    offending = recognizer->getCurrentToken();
  }
  report(recognizer, offending);
}

void PortugolErrorStrategy::reportUnwantedToken(Parser *recognizer) {
  if (inErrorRecoveryMode(recognizer)) {
    return;
  }
  beginErrorCondition(recognizer);
  report(recognizer, recognizer->getCurrentToken());
}

void PortugolErrorStrategy::reportMissingToken(Parser *recognizer) {
  if (inErrorRecoveryMode(recognizer)) {
    return;
  }
  beginErrorCondition(recognizer);
  report(recognizer, recognizer->getCurrentToken());
}

static bool isBlockTerminator(size_t type) {
  switch (type) {
  case PortugolParser::T_KW_FIM:
  case PortugolParser::T_KW_FIM_SE:
  case PortugolParser::T_KW_FIM_ENQUANTO:
  case PortugolParser::T_KW_FIM_PARA:
  case PortugolParser::T_KW_ATE:
    return true;
  }
  return false;
}

static string enclosingBlockExpectation(ParserRuleContext *ctx) {
  for (; ctx; ctx = dynamic_cast<ParserRuleContext *>(ctx->parent)) {
    if (dynamic_cast<PortugolParser::Stm_seContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fimse;
    if (dynamic_cast<PortugolParser::Stm_enquantoContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fimenq;
    if (dynamic_cast<PortugolParser::Stm_paraContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fimpara;
    if (dynamic_cast<PortugolParser::Stm_repitaContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_ate;
    if (dynamic_cast<PortugolParser::Stm_blockContext *>(ctx))
      return BasePortugolParser::expecting_stm_or_fim;
  }
  return "";
}

string PortugolErrorStrategy::expectingText(Parser *recognizer,
                                            const misc::IntervalSet &expected,
                                            Token * /*offending*/) {
  ParserRuleContext *ctx = recognizer->getContext();
  size_t rule = ctx ? ctx->getRuleIndex() : (size_t)-1;

  bool hasIdent = expected.contains((size_t)PortugolParser::T_IDENTIFICADOR);

  // Inside a statement list the expected set carries the terminator of the
  // enclosing block, which is what the ANTLR2 messages named.
  if (expected.contains((size_t)PortugolParser::T_KW_FIM_SE))
    return BasePortugolParser::expecting_stm_or_fimse;
  if (expected.contains((size_t)PortugolParser::T_KW_FIM_ENQUANTO))
    return BasePortugolParser::expecting_stm_or_fimenq;
  if (expected.contains((size_t)PortugolParser::T_KW_FIM_PARA))
    return BasePortugolParser::expecting_stm_or_fimpara;
  if (expected.contains((size_t)PortugolParser::T_KW_ATE) &&
      !expected.contains((size_t)PortugolParser::T_KW_PASSO))
    return BasePortugolParser::expecting_stm_or_ate;
  if (expected.contains((size_t)PortugolParser::T_KW_FIM))
    return BasePortugolParser::expecting_stm_or_fim;

  if (expected.contains((size_t)PortugolParser::T_KW_SE) &&
      expected.contains((size_t)PortugolParser::T_KW_RETORNE)) {
    string s = enclosingBlockExpectation(ctx);
    if (!s.empty())
      return s;
  }

  if (expected.contains((size_t)PortugolParser::T_KW_VARIAVEIS) &&
      expected.contains((size_t)PortugolParser::T_KW_INICIO)) {
    return "\"variáveis\" ou \"início\" após declaração de algoritmo";
  }

  if (expected.contains((size_t)PortugolParser::T_KW_FUNCAO) &&
      expected.contains((size_t)Token::EOF)) {
    return BasePortugolParser::expecting_eof_or_function;
  }

  if (expected.contains((size_t)PortugolParser::T_KW_FIM_VARIAVEIS) &&
      hasIdent) {
    return BasePortugolParser::expecting_fimvar_or_var;
  }

  switch (rule) {
  case PortugolParser::RuleDeclaracao_algoritmo:
    if (hasIdent)
      return BasePortugolParser::expecting_algorithm_name;
    break;
  case PortugolParser::RuleFunc_decls:
    if (hasIdent)
      return BasePortugolParser::expecting_function_name;
    break;
  case PortugolParser::RuleFparams:
    return BasePortugolParser::expecting_param_or_fparen;
  case PortugolParser::RuleVar_decl_block:
  case PortugolParser::RuleVar_decl:
  case PortugolParser::RuleFvar_decl:
  case PortugolParser::RuleFparam:
  case PortugolParser::RuleLvalue:
  case PortugolParser::RuleStm_para:
    if (hasIdent && !expected.contains((size_t)PortugolParser::T_INT_LIT))
      return BasePortugolParser::expecting_variable;
    if (expected.contains((size_t)PortugolParser::T_COLON))
      return tokenName(recognizer, PortugolParser::T_COLON);
    break;
  case PortugolParser::RulePasso:
  case PortugolParser::RuleDimensoes:
    if (expected.contains((size_t)PortugolParser::T_INT_LIT))
      return tokenName(recognizer, PortugolParser::T_INT_LIT);
    break;
  case PortugolParser::RuleTp_prim_pl:
    return BasePortugolParser::expecting_datatype_pl;
  }

  if (expected.contains((size_t)PortugolParser::T_KW_INTEIROS)) {
    return BasePortugolParser::expecting_datatype_pl;
  }
  if (expected.contains((size_t)PortugolParser::T_KW_INTEIRO)) {
    return BasePortugolParser::expecting_datatype;
  }

  if (expected.contains((size_t)PortugolParser::T_INT_LIT) &&
      expected.contains((size_t)PortugolParser::T_STRING_LIT)) {
    return BasePortugolParser::expecting_expression;
  }

  vector<ptrdiff_t> types = expected.toList();
  string str;
  for (size_t i = 0; i < types.size(); ++i) {
    if (i > 0) {
      str += " ou ";
    }
    str += tokenName(recognizer, (size_t)types[i]);
  }
  return str;
}

void PortugolErrorStrategy::report(Parser *recognizer, Token *offending) {
  misc::IntervalSet expected = recognizer->getExpectedTokens();
  string expecting = expectingText(recognizer, expected, offending);

  Token *last = recognizer->getTokenStream()->LT(-1);
  ParserRuleContext *ctx0 = recognizer->getContext();
  size_t rule0 = ctx0 ? ctx0->getRuleIndex() : (size_t)-1;

  // The ANTLR2 handlers of these rules always reported "encontrado X" on the
  // line of the unexpected token, the others only when it shared the line
  // of the last consumed token.
  bool alwaysFound =
      expecting == BasePortugolParser::expecting_stm_or_fim ||
      expecting == BasePortugolParser::expecting_eof_or_function ||
      expecting.compare(0, 11, "\"variáveis\"") == 0 ||
      rule0 == PortugolParser::RuleAlgoritmo;
  bool alwaysAfter = last && (rule0 == PortugolParser::RulePasso ||
                              rule0 == PortugolParser::RuleDimensoes);

  int line;
  string found, after;
  if (alwaysAfter ||
      (!alwaysFound && last && last->getLine() != offending->getLine())) {
    line = (int)last->getLine();
    after = last->getText();
  } else {
    line = (int)offending->getLine();
    found = tokenDescription(recognizer, offending);
    if (expecting == BasePortugolParser::expecting_expression && last) {
      after = last->getText();
    }
  }

  int cd = BasePortugolParser::reportParserError(line, expecting, found, after);

  size_t rule = recognizer->getContext()
                    ? recognizer->getContext()->getRuleIndex()
                    : (size_t)-1;
  size_t ftype = offending->getType();

  if (expected.contains((size_t)PortugolParser::T_COLON) && last &&
      (rule == PortugolParser::RuleVar_decl ||
       rule == PortugolParser::RuleFparam) &&
      found.length()) {
    if (ftype == PortugolParser::T_IDENTIFICADOR) {
      BasePortugolParser::printTip(
          string("Coloque uma vírgula entre as variáveis \"") +
              last->getText() + "\" e \"" + offending->getText() + "\"",
          line, cd);
    } else if (isDatatype(ftype)) {
      BasePortugolParser::printTip(string("Coloque ':' entre \"") +
                                       last->getText() + "\" e " +
                                       tokenName(recognizer, ftype),
                                   line, cd);
    }
  } else if (expecting == BasePortugolParser::expecting_stm_or_fim &&
             ftype == PortugolParser::T_IDENTIFICADOR) {
    BasePortugolParser::printTip(
        string("Verifique o uso de \"[]\" (caso \"") + offending->getText() +
            "\" seja um conjunto/matriz), do operador \":=\" (caso seja um "
            "comando de atribuição) e do uso de parêntesis (caso \"" +
            offending->getText() + "\" seja uma chamada de função)",
        line, cd);
  } else if (expecting == BasePortugolParser::expecting_variable &&
             rule == PortugolParser::RuleVar_decl_block) {
    BasePortugolParser::printTip("Pelo menos uma variável deve ser declarada",
                                 line, cd);
  } else if ((rule == PortugolParser::RuleFcall ||
              rule == PortugolParser::RuleFargs) &&
             expected.contains((size_t)PortugolParser::T_FECHAP) &&
             (ftype == PortugolParser::T_IDENTIFICADOR ||
              ftype == PortugolParser::T_STRING_LIT ||
              ftype == PortugolParser::T_INT_LIT ||
              ftype == PortugolParser::T_REAL_LIT ||
              ftype == PortugolParser::T_CARAC_LIT)) {
    BasePortugolParser::printTip(string("Coloque uma vírgula antes de \"") +
                                     offending->getText() + "\"",
                                 line, cd);
  }
}

void PortugolLexerErrorListener::syntaxError(Recognizer * /*recognizer*/,
                                             Token * /*offendingSymbol*/,
                                             size_t line,
                                             size_t /*charPositionInLine*/,
                                             const string &msg,
                                             std::exception_ptr /*e*/) {
  // msg is "token recognition error at: '<text>'"
  string text;
  string::size_type pos = msg.find("at: '");
  if (pos != string::npos) {
    text = msg.substr(pos + 5);
    if (!text.empty() && text[text.size() - 1] == '\'') {
      text.erase(text.size() - 1);
    }
  }

  stringstream s;
  if (text.compare(0, 2, "/*") == 0) {
    int endline = (int)line;
    for (string::size_type i = 0; i < text.size(); ++i) {
      if (text[i] == '\n')
        endline++;
    }
    s << "AVISO: comentário iniciado na linha " << line
      << " não termina com \"*/\"";
    GPTDisplay::self()->add(s.str(), endline);
    return;
  }

  if (!text.empty() && (text[0] == '"' || text[0] == '\'')) {
    s << "Faltando fechar aspas";
  } else {
    string c;
    if (!text.empty()) {
      unsigned char b = (unsigned char)text[0];
      size_t len = (b < 0x80)        ? 1
                   : (b >> 5) == 0x6 ? 2
                   : (b >> 4) == 0xE ? 3
                                     : 4;
      c = text.substr(0, len);
    }
    s << "Caractere inválido: \"" << c << "\"";
  }
  GPTDisplay::self()->add(s.str(), (int)line);
}

void validateTokens(CommonTokenStream &tokens) {
  tokens.fill();
  const vector<Token *> &all = tokens.getTokens();
  for (size_t i = 0; i + 1 < all.size(); ++i) {
    if (all[i]->getType() == PortugolParser::T_DIV &&
        all[i + 1]->getType() == PortugolParser::T_MULTIP &&
        all[i + 1]->getStartIndex() == all[i]->getStopIndex() + 1) {
      // The grammar has no rule for an unterminated comment, so "/*" lexes
      // as '/' '*'. Report it like the ANTLR2 lexer did and hide the rest.
      stringstream s;
      s << "AVISO: comentário iniciado na linha " << all[i]->getLine()
        << " não termina com \"*/\"";
      GPTDisplay::self()->add(s.str(), (int)all.back()->getLine());
      for (size_t j = i; j < all.size(); ++j) {
        if (all[j]->getType() == Token::EOF)
          break;
        if (auto wt = dynamic_cast<WritableToken *>(all[j]))
          wt->setChannel(Token::HIDDEN_CHANNEL);
      }
      break;
    }
  }
  for (Token *t : all) {
    // The ANTLR2 lexer rewrote 0c17/0x1F/0b101 to decimal text, so every
    // consumer only ever sees base-10 integers.
    if (t->getType() == PortugolParser::T_INT_LIT) {
      const string &text = t->getText();
      if (text.size() > 2 && text[0] == '0') {
        int base = 0;
        switch (text[1]) {
        case 'c':
        case 'C':
          base = 8;
          break;
        case 'x':
        case 'X':
          base = 16;
          break;
        case 'b':
        case 'B':
          base = 2;
          break;
        }
        if (base) {
          unsigned long v = strtoul(text.c_str() + 2, NULL, base);
          stringstream s;
          s << v;
          if (auto wt = dynamic_cast<WritableToken *>(t))
            wt->setText(s.str());
        }
      }
      continue;
    }
    if (t->getType() != PortugolParser::T_IDENTIFICADOR) {
      continue;
    }
    const string &text = t->getText();
    bool hasLatim = false;
    for (string::size_type i = 0; i < text.size(); ++i) {
      if ((unsigned char)text[i] >= 0x80) {
        hasLatim = true;
        break;
      }
    }
    if (hasLatim) {
      stringstream s;
      s << "Identificador \"" << text << "\" não pode ter caracteres especiais";
      GPTDisplay::self()->add(s.str(), (int)t->getLine());
    }
  }
}
